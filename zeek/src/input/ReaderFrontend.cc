// See the file "COPYING" in the main distribution directory for copyright.

#include "Manager.h"
#include "ReaderFrontend.h"
#include "ReaderBackend.h"

namespace input {

class InitMessage final : public threading::InputMessage<ReaderBackend>
{
public:
	InitMessage(ReaderBackend* backend,
		    const int num_fields, const threading::Field* const* fields)
		: threading::InputMessage<ReaderBackend>("Init", backend),
		num_fields(num_fields), fields(fields) { }

	bool Process() override
		{
		return Object()->Init(num_fields, fields);
		}

private:
	const int num_fields;
	const threading::Field* const* fields;
};

class UpdateMessage final : public threading::InputMessage<ReaderBackend>
{
public:
	UpdateMessage(ReaderBackend* backend)
		: threading::InputMessage<ReaderBackend>("Update", backend)
		 { }

	bool Process() override { return Object()->Update(); }
};

ReaderFrontend::ReaderFrontend(const ReaderBackend::ReaderInfo& arg_info, EnumVal* type)
	{
	disabled = initialized = false;
	info = new ReaderBackend::ReaderInfo(arg_info);

	const char* t = type->GetType()->AsEnumType()->Lookup(type->InternalInt());
	name = copy_string(fmt("%s/%s", arg_info.source, t));

	backend = input_mgr->CreateBackend(this, type);
	assert(backend);
	backend->Start();
	}

void ReaderFrontend::Stop()
	{
	if ( backend )
		{
		backend->SignalStop();
		backend = nullptr; // Thread manager will clean it up once it finishes.
		}
	}

ReaderFrontend::~ReaderFrontend()
	{
	delete [] name;
	delete info;
	}

void ReaderFrontend::Init(const int arg_num_fields,
		          const threading::Field* const* arg_fields)
	{
	if ( disabled )
		return;

	if ( initialized )
		reporter->InternalError("reader initialize twice");

	num_fields = arg_num_fields;
	fields = arg_fields;
	initialized = true;

	backend->SendIn(new InitMessage(backend, num_fields, fields));
	}

void ReaderFrontend::Update()
	{
	if ( disabled )
		return;

	if ( ! initialized )
		{
		reporter->Error("Tried to call update on uninitialized reader");
		return;
		}

	backend->SendIn(new UpdateMessage(backend));
	}

const char* ReaderFrontend::Name() const
	{
	return name;
	}

}
