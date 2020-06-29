// See the file "COPYING" in the main distribution directory for copyright.

#include "zeek-config.h"

#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "SQLite.h"
#include "sqlite.bif.h"
#include "logging/writers/sqlite/sqlite.bif.h"
#include "logging/writers/ascii/ascii.bif.h"

#include "threading/SerialTypes.h"

using namespace input::reader;
using threading::Value;
using threading::Field;

SQLite::SQLite(ReaderFrontend *frontend)
	: ReaderBackend(frontend),
	  fields(), num_fields(), mode(), started(), query(), db(), st()
	{
	set_separator.assign(
			(const char*) zeek::BifConst::LogSQLite::set_separator->Bytes(),
			zeek::BifConst::InputSQLite::set_separator->Len()
			);

	unset_field.assign(
			(const char*) zeek::BifConst::LogSQLite::unset_field->Bytes(),
			zeek::BifConst::InputSQLite::unset_field->Len()
			);

	empty_field.assign(
			(const char*) zeek::BifConst::LogAscii::empty_field->Bytes(),
			zeek::BifConst::InputSQLite::empty_field->Len()
			);

	io = new threading::formatter::Ascii(this, threading::formatter::Ascii::SeparatorInfo(std::string(), set_separator, unset_field, empty_field));
	}

SQLite::~SQLite()
	{
	DoClose();
	delete io;
	}

void SQLite::DoClose()
	{
	sqlite3_finalize(st);
	st = nullptr;

	if ( db != 0 )
		{
		sqlite3_close(db);
		db = 0;
		}
	}

bool SQLite::checkError(int code)
	{
	if ( code != SQLITE_OK && code != SQLITE_DONE )
		{
		Error(Fmt("SQLite call failed: %s", sqlite3_errmsg(db)));
		return true;
		}

	return false;
	}

bool SQLite::DoInit(const ReaderInfo& info, int arg_num_fields, const threading::Field* const* arg_fields)
	{
	if ( sqlite3_threadsafe() == 0 )
		{
		Error("SQLite reports that it is not threadsafe. Zeek needs a threadsafe version of SQLite. Aborting");
		return false;
		}

	// Allow connections to same DB to use single data/schema cache. Also
	// allows simultaneous writes to one file.
	sqlite3_enable_shared_cache(1);

	if ( Info().mode != MODE_MANUAL )
		{
		Error("SQLite only supports manual reading mode.");
		return false;
		}

	started = false;

	std::string fullpath(info.source);
	fullpath.append(".sqlite");

	std::string query;
	ReaderInfo::config_map::const_iterator it = info.config.find("query");
	if ( it == info.config.end() )
		{
		Error(Fmt("No query specified when setting up SQLite data source %s. Aborting.", info.source));
		return false;
		}
	else
		query = it->second;

	if ( checkError(sqlite3_open_v2(
					fullpath.c_str(),
					&db,
					SQLITE_OPEN_READWRITE |
					SQLITE_OPEN_NOMUTEX
					,
					NULL)) )
		return false;

	num_fields = arg_num_fields;
	fields = arg_fields;

	// create the prepared select statement that we will re-use forever...
	if ( checkError(sqlite3_prepare_v2( db, query.c_str(), query.size()+1, &st, NULL )) )
		{
		return false;
		}

	DoUpdate();

	return true;
	}

// pos = field position
// subpos = subfield position, only used for port-field
Value* SQLite::EntryToVal(sqlite3_stmt *st, const threading::Field *field, int pos, int subpos)
	{
	if ( sqlite3_column_type(st, pos ) == SQLITE_NULL )
		return new Value(field->type, field->subtype, false);

	Value* val = new Value(field->type, true);

	switch ( field->type ) {
	case zeek::TYPE_ENUM:
	case zeek::TYPE_STRING:
		{
		const char *text = (const char*) sqlite3_column_text(st, pos);
		int length = sqlite3_column_bytes(st, pos);

		char *out = new char[length];
		memcpy(out, text, length);

		val->val.string_val.length = length;
		val->val.string_val.data = out;
		break;
		}

	case zeek::TYPE_BOOL:
		{
		if ( sqlite3_column_type(st, pos) != SQLITE_INTEGER )
			{
			Error("Invalid data type for boolean - expected Integer");
			delete val;
			return nullptr;
			}

		int res = sqlite3_column_int(st, pos);

		if ( res == 0 || res == 1 )
			val->val.int_val = res;
		else
			{
			Error(Fmt("Invalid value for boolean: %d", res));
			delete val;
			return nullptr;
			}
		break;
		}

	case zeek::TYPE_INT:
		val->val.int_val = sqlite3_column_int64(st, pos);
		break;

	case zeek::TYPE_DOUBLE:
	case zeek::TYPE_TIME:
	case zeek::TYPE_INTERVAL:
		val->val.double_val = sqlite3_column_double(st, pos);
		break;

	case zeek::TYPE_COUNT:
	case zeek::TYPE_COUNTER:
		val->val.uint_val = sqlite3_column_int64(st, pos);
		break;

	case zeek::TYPE_PORT:
		{
		val->val.port_val.port = sqlite3_column_int(st, pos);
		val->val.port_val.proto = TRANSPORT_UNKNOWN;
		if ( subpos != -1 )
			{
			const char *text = (const char*) sqlite3_column_text(st, subpos);

			if ( text == 0 )
				Error("Port protocol definition did not contain text");
			else
				{
				std::string s(text, sqlite3_column_bytes(st, subpos));
				val->val.port_val.proto = io->ParseProto(s);
				}
			}
		break;
		}

	case zeek::TYPE_SUBNET:
		{
		const char *text = (const char*) sqlite3_column_text(st, pos);
		std::string s(text, sqlite3_column_bytes(st, pos));
		int pos = s.find('/');
		int width = atoi(s.substr(pos+1).c_str());
		std::string addr = s.substr(0, pos);

		val->val.subnet_val.prefix = io->ParseAddr(addr);
		val->val.subnet_val.length = width;
		break;
		}

	case zeek::TYPE_ADDR:
		{
		const char *text = (const char*) sqlite3_column_text(st, pos);
		std::string s(text, sqlite3_column_bytes(st, pos));
		val->val.addr_val = io->ParseAddr(s);
		break;
		}

	case zeek::TYPE_TABLE:
	case zeek::TYPE_VECTOR:
		{
		const char *text = (const char*) sqlite3_column_text(st, pos);
		std::string s(text, sqlite3_column_bytes(st, pos));
		delete val;
		val = io->ParseValue(s, "", field->type, field->subtype);
		break;
		}

	default:
		Error(Fmt("unsupported field format %d", field->type));
		delete val;
		return nullptr;
	}

	return val;

	}

bool SQLite::DoUpdate()
	{
	int numcolumns = sqlite3_column_count(st);
	int *mapping = new int [num_fields];
	int *submapping = new int [num_fields];

	// first set them all to -1
	for ( unsigned int i = 0; i < num_fields; ++i )
		{
		mapping[i] = -1;
		submapping[i] = -1;
		}

	for ( int i = 0; i < numcolumns; ++i )
		{
		const char *name = sqlite3_column_name(st, i);

		for ( unsigned j = 0; j < num_fields; j++ )
			{
			if ( strcmp(fields[j]->name, name) == 0 )
				{
				if ( mapping[j] != -1 )
					{
					Error(Fmt("SQLite statement returns several columns with name %s! Cannot decide which to choose, aborting", name));
					delete [] mapping;
					delete [] submapping;
					return false;
					}

				mapping[j] = i;
				}

			if ( fields[j]->secondary_name != nullptr && strcmp(fields[j]->secondary_name, name) == 0 )
				{
				assert(fields[j]->type == zeek::TYPE_PORT);
				if ( submapping[j] != -1 )
					{
					Error(Fmt("SQLite statement returns several columns with name %s! Cannot decide which to choose, aborting", name));
					delete [] mapping;
					delete [] submapping;
					return false;
					}

				submapping[j] = i;
				}
			}
		}

	for ( unsigned int i = 0; i < num_fields; ++i )
		{
		if ( mapping[i] == -1 )
			{
			Error(Fmt("Required field %s not found after SQLite statement", fields[i]->name));
			delete [] mapping;
			delete [] submapping;
			return false;
			}
		}

	int errorcode;
	while ( ( errorcode = sqlite3_step(st)) == SQLITE_ROW )
		{
		Value** ofields = new Value*[num_fields];

		for ( unsigned int j = 0; j < num_fields; ++j)
			{
			ofields[j] = EntryToVal(st, fields[j], mapping[j], submapping[j]);
			if ( ! ofields[j] )
				{
				for ( unsigned int k = 0; k < j; ++k )
					delete ofields[k];

				delete [] ofields;
				delete [] mapping;
				delete [] submapping;
				return false;
				}
			}

		SendEntry(ofields);
		}

	delete [] mapping;
	delete [] submapping;

	if ( checkError(errorcode) ) // check the last error code returned by sqlite
		return false;

	EndCurrentSend();

	if ( checkError(sqlite3_reset(st)) )
		return false;

	return true;
	}
