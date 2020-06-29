// See the file "COPYING" in the main distribution directory for copyright.

#include "zeek-config.h"

#include <string>
#include <errno.h>
#include <vector>

#include "threading/SerialTypes.h"

#include "SQLite.h"
#include "sqlite.bif.h"

using namespace std;
using namespace logging;
using namespace writer;
using threading::Value;
using threading::Field;

SQLite::SQLite(WriterFrontend* frontend)
	: WriterBackend(frontend),
	  fields(), num_fields(), db(), st()
	{
	set_separator.assign(
			(const char*) zeek::BifConst::LogSQLite::set_separator->Bytes(),
			zeek::BifConst::LogSQLite::set_separator->Len()
			);

	unset_field.assign(
			(const char*) zeek::BifConst::LogSQLite::unset_field->Bytes(),
			zeek::BifConst::LogSQLite::unset_field->Len()
			);

	empty_field.assign(
			(const char*) zeek::BifConst::LogSQLite::empty_field->Bytes(),
			zeek::BifConst::LogSQLite::empty_field->Len()
			);

	threading::formatter::Ascii::SeparatorInfo sep_info(string(), set_separator, unset_field, empty_field);
	io = new threading::formatter::Ascii(this, sep_info);
	}

SQLite::~SQLite()
	{
	if ( db != 0 )
		{
		sqlite3_finalize(st);
		if ( ! sqlite3_close(db) )
			Error("Sqlite could not close connection");

		db = 0;
		}

	delete io;
	}

string SQLite::GetTableType(int arg_type, int arg_subtype) {
	string type;

	switch ( arg_type ) {
	case zeek::TYPE_BOOL:
		type = "boolean";
		break;

	case zeek::TYPE_INT:
	case zeek::TYPE_COUNT:
	case zeek::TYPE_COUNTER:
	case zeek::TYPE_PORT: // note that we do not save the protocol at the moment. Just like in the case of the ascii-writer
		type = "integer";
		break;

	case zeek::TYPE_SUBNET:
	case zeek::TYPE_ADDR:
		type = "text"; // sqlite3 does not have a type for internet addresses
		break;

	case zeek::TYPE_TIME:
	case zeek::TYPE_INTERVAL:
	case zeek::TYPE_DOUBLE:
		type = "double precision";
		break;

	case zeek::TYPE_ENUM:
	case zeek::TYPE_STRING:
	case zeek::TYPE_FILE:
	case zeek::TYPE_FUNC:
		type = "text";
		break;

	case zeek::TYPE_TABLE:
	case zeek::TYPE_VECTOR:
		type = "text"; // dirty - but sqlite does not directly support arrays. so - we just roll it into a ","-separated string.
		break;

	default:
		Error(Fmt("unsupported field format %d ", arg_type));
		return ""; // not the cleanest way to abort. But sqlite will complain on create table...
	}

	return type;
}

// returns true true in case of error
bool SQLite::checkError(int code)
	{
	if ( code != SQLITE_OK && code != SQLITE_DONE )
		{
		Error(Fmt("SQLite call failed: %s", sqlite3_errmsg(db)));
		return true;
		}

	return false;
	}

bool SQLite::DoInit(const WriterInfo& info, int arg_num_fields,
			    const Field* const * arg_fields)
	{
	if ( sqlite3_threadsafe() == 0 )
		{
		Error("SQLite reports that it is not threadsafe. Zeek needs a threadsafe version of SQLite. Aborting");
		return false;
		}

	// Allow connections to same DB to use single data/schema cache. Also
	// allows simultaneous writes to one file.
	sqlite3_enable_shared_cache(1);

	num_fields = arg_num_fields;
	fields = arg_fields;

	string fullpath(info.path);
	fullpath.append(".sqlite");
	string tablename;

	WriterInfo::config_map::const_iterator it = info.config.find("tablename");
	if ( it == info.config.end() )
		{
		MsgThread::Info(Fmt("tablename configuration option not found. Defaulting to path %s", info.path));
		tablename = info.path;
		}
	else
		tablename = it->second;

	if ( checkError(sqlite3_open_v2(
					fullpath.c_str(),
					&db,
					SQLITE_OPEN_READWRITE |
					SQLITE_OPEN_CREATE |
					SQLITE_OPEN_NOMUTEX
					,
					NULL)) )
		return false;

	string create = "CREATE TABLE IF NOT EXISTS " + tablename + " (\n";
		//"id SERIAL UNIQUE NOT NULL"; // SQLite has rowids, we do not need a counter here.

	for ( unsigned int i = 0; i < num_fields; ++i )
		{
		const Field* field = fields[i];

		if ( i != 0 )
			create += ",\n";

		// sadly sqlite3 has no other method for escaping stuff. That I know of.
		char* fieldname = sqlite3_mprintf("%Q", fields[i]->name);
		if ( fieldname == 0 )
			{
			InternalError("Could not malloc memory");
			return false;
			}

		create += fieldname;
		sqlite3_free(fieldname);

		string type = GetTableType(field->type, field->subtype);
		if ( type == "" )
			{
			InternalError(Fmt("Could not determine type for field %u:%s", i, fieldname));
			return false;
			}

		create += " " + type;

		/* if ( !field->optional ) {
		 create += " NOT NULL";
		 } */
		}

	create += "\n);";

	char *errorMsg = 0;
	int res = sqlite3_exec(db, create.c_str(), NULL, NULL, &errorMsg);
	if ( res != SQLITE_OK )
		{
		Error(Fmt("Error executing table creation statement: %s", errorMsg));
		sqlite3_free(errorMsg);
		return false;
		}

	// create the prepared statement that will be re-used forever...
	string insert = "VALUES (";
	string names = "INSERT INTO " + tablename + " ( ";

	for ( unsigned int i = 0; i < num_fields; i++ )
		{
		bool ac = true;

		if ( i == 0 )
			ac = false;
		else
			{
			names += ", ";
			insert += ", ";
			}

		insert += "?";

		char* fieldname = sqlite3_mprintf("%Q", fields[i]->name);
		if ( fieldname == 0 )
			{
			InternalError("Could not malloc memory");
			return false;
			}

		names.append(fieldname);
		sqlite3_free(fieldname);
		}

	insert += ");";
	names += ") ";

	insert = names + insert;

	if ( checkError(sqlite3_prepare_v2(db, insert.c_str(), insert.size()+1, &st, NULL)) )
		return false;

	return true;
	}

int SQLite::AddParams(Value* val, int pos)
	{
	if ( ! val->present )
		return sqlite3_bind_null(st, pos);

	switch ( val->type ) {
	case zeek::TYPE_BOOL:
		return sqlite3_bind_int(st, pos, val->val.int_val != 0 ? 1 : 0 );

	case zeek::TYPE_INT:
		return sqlite3_bind_int(st, pos, val->val.int_val);

	case zeek::TYPE_COUNT:
	case zeek::TYPE_COUNTER:
		return sqlite3_bind_int(st, pos, val->val.uint_val);

	case zeek::TYPE_PORT:
		return sqlite3_bind_int(st, pos, val->val.port_val.port);

	case zeek::TYPE_SUBNET:
		{
		string out = io->Render(val->val.subnet_val);
		return sqlite3_bind_text(st, pos, out.data(), out.size(), SQLITE_TRANSIENT);
		}

	case zeek::TYPE_ADDR:
		{
		string out = io->Render(val->val.addr_val);
		return sqlite3_bind_text(st, pos, out.data(), out.size(), SQLITE_TRANSIENT);
		}

	case zeek::TYPE_TIME:
	case zeek::TYPE_INTERVAL:
	case zeek::TYPE_DOUBLE:
		return sqlite3_bind_double(st, pos, val->val.double_val);

	case zeek::TYPE_ENUM:
	case zeek::TYPE_STRING:
	case zeek::TYPE_FILE:
	case zeek::TYPE_FUNC:
		{
		if ( ! val->val.string_val.length || val->val.string_val.length == 0 )
			return sqlite3_bind_null(st, pos);

		return sqlite3_bind_text(st, pos, val->val.string_val.data, val->val.string_val.length, SQLITE_TRANSIENT);
		}

	case zeek::TYPE_TABLE:
		{
		ODesc desc;
		desc.Clear();
		desc.EnableEscaping();
		desc.AddEscapeSequence(set_separator);

		if ( ! val->val.set_val.size )
			desc.Add(empty_field);
		else
			for ( int j = 0; j < val->val.set_val.size; j++ )
				{
				if ( j > 0 )
					desc.AddRaw(set_separator);

				io->Describe(&desc, val->val.set_val.vals[j], fields[pos-1]->name);
				}

		desc.RemoveEscapeSequence(set_separator);
		return sqlite3_bind_text(st, pos, (const char*) desc.Bytes(), desc.Len(), SQLITE_TRANSIENT);
		}

	case zeek::TYPE_VECTOR:
		{
		ODesc desc;
		desc.Clear();
		desc.EnableEscaping();
		desc.AddEscapeSequence(set_separator);

		if ( ! val->val.vector_val.size )
			desc.Add(empty_field);
		else
			for ( int j = 0; j < val->val.vector_val.size; j++ )
				{
				if ( j > 0 )
					desc.AddRaw(set_separator);

				io->Describe(&desc, val->val.vector_val.vals[j], fields[pos-1]->name);
				}

		desc.RemoveEscapeSequence(set_separator);
		return sqlite3_bind_text(st, pos, (const char*) desc.Bytes(), desc.Len(), SQLITE_TRANSIENT);
		}

	default:
		Error(Fmt("unsupported field format %d", val->type));
		return 0;
	}
	}

bool SQLite::DoWrite(int num_fields, const Field* const * fields, Value** vals)
	{
	// bind parameters
	for ( int i = 0; i < num_fields; i++ )
		{
		if ( checkError(AddParams(vals[i], i+1)) )
			return false;
		}

	// execute query
	if ( checkError(sqlite3_step(st)) )
		return false;

	// clean up and make ready for next query execution
	if ( checkError(sqlite3_clear_bindings(st)) )
		return false;

	if ( checkError(sqlite3_reset(st)) )
		return false;

	return true;
	}

bool SQLite::DoRotate(const char* rotated_path, double open, double close, bool terminating)
	{
	if ( ! FinishedRotation("/dev/null", Info().path, open, close, terminating))
		{
		Error(Fmt("error rotating %s", Info().path));
		return false;
		}

	return true;
        }
