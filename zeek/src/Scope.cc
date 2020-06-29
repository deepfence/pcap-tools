// See the file "COPYING" in the main distribution directory for copyright.

#include "zeek-config.h"

#include "Scope.h"
#include "Desc.h"
#include "ID.h"
#include "IntrusivePtr.h"
#include "Val.h"
#include "Reporter.h"
#include "module_util.h"

typedef PList<Scope> scope_list;

static scope_list scopes;
static Scope* top_scope;

Scope::Scope(IntrusivePtr<zeek::detail::ID> id,
             std::unique_ptr<std::vector<IntrusivePtr<zeek::detail::Attr>>> al)
	: scope_id(std::move(id)), attrs(std::move(al))
	{
	return_type = nullptr;

	if ( id )
		{
		const auto& id_type = scope_id->GetType();

		if ( id_type->Tag() == zeek::TYPE_ERROR )
			return;
		else if ( id_type->Tag() != zeek::TYPE_FUNC )
			reporter->InternalError("bad scope id");

		zeek::FuncType* ft = id->GetType()->AsFuncType();
		return_type = ft->Yield();
		}
	}

const IntrusivePtr<zeek::detail::ID>& Scope::Find(std::string_view name) const
	{
	auto entry = local.find(name);

	if ( entry != local.end() )
		return entry->second;

	return zeek::detail::ID::nil;
	}

IntrusivePtr<zeek::detail::ID> Scope::Remove(std::string_view name)
	{
	auto entry = local.find(name);

	if ( entry != local.end() )
		{
		auto id = std::move(entry->second);
		local.erase(entry);
		return id;
		}

	return nullptr;
	}

IntrusivePtr<zeek::detail::ID> Scope::GenerateTemporary(const char* name)
	{
	return make_intrusive<zeek::detail::ID>(name, zeek::detail::SCOPE_FUNCTION, false);
	}

std::vector<IntrusivePtr<zeek::detail::ID>> Scope::GetInits()
	{
	auto rval = std::move(inits);
	inits = {};
	return rval;
	}

void Scope::Describe(ODesc* d) const
	{
	if ( d->IsReadable() )
		d->AddSP("scope");

	else
		{
		d->Add(scope_id != nullptr);
		d->SP();
		d->Add(return_type != nullptr);
		d->SP();
		d->Add(static_cast<uint64_t>(local.size()));
		d->SP();
		}

	if ( scope_id )
		{
		scope_id->Describe(d);
		d->NL();
		}

	if ( return_type )
		{
		return_type->Describe(d);
		d->NL();
		}

	for ( const auto& entry : local )
		{
		zeek::detail::ID* id = entry.second.get();
		id->Describe(d);
		d->NL();
		}
	}

TraversalCode Scope::Traverse(TraversalCallback* cb) const
	{
	for ( const auto& entry : local )
		{
		zeek::detail::ID* id = entry.second.get();
		TraversalCode tc = id->Traverse(cb);
		HANDLE_TC_STMT_PRE(tc);
		}

	return TC_CONTINUE;
	}


const IntrusivePtr<zeek::detail::ID>& lookup_ID(const char* name, const char* curr_module,
                                                bool no_global, bool same_module_only,
                                                bool check_export)
	{
	std::string fullname = make_full_var_name(curr_module, name);

	std::string ID_module = extract_module_name(fullname.c_str());
	bool need_export = check_export && (ID_module != GLOBAL_MODULE_NAME &&
	                                    ID_module != curr_module);

	for ( int i = scopes.length() - 1; i >= 0; --i )
		{
		const auto& id = scopes[i]->Find(fullname);

		if ( id )
			{
			if ( need_export && ! id->IsExport() && ! in_debug )
				reporter->Error("identifier is not exported: %s",
				      fullname.c_str());

			return id;
			}
		}

	if ( ! no_global && (strcmp(GLOBAL_MODULE_NAME, curr_module) == 0 ||
	     ! same_module_only) )
		{
		std::string globalname = make_full_var_name(GLOBAL_MODULE_NAME, name);
		return global_scope()->Find(globalname);
		}

	return zeek::detail::ID::nil;
	}

IntrusivePtr<zeek::detail::ID> install_ID(const char* name, const char* module_name,
                            bool is_global, bool is_export)
	{
	if ( scopes.empty() && ! is_global )
		reporter->InternalError("local identifier in global scope");

	zeek::detail::IDScope scope;
	if ( is_export || ! module_name ||
	     (is_global &&
	      normalized_module_name(module_name) == GLOBAL_MODULE_NAME) )
		scope = zeek::detail::SCOPE_GLOBAL;
	else if ( is_global )
		scope = zeek::detail::SCOPE_MODULE;
	else
		scope = zeek::detail::SCOPE_FUNCTION;

	std::string full_name = make_full_var_name(module_name, name);

	auto id = make_intrusive<zeek::detail::ID>(full_name.data(), scope, is_export);

	if ( zeek::detail::SCOPE_FUNCTION != scope )
		global_scope()->Insert(std::move(full_name), id);
	else
		{
		id->SetOffset(top_scope->Length());
		top_scope->Insert(std::move(full_name), id);
		}

	return id;
	}

void push_existing_scope(Scope* scope)
	{
	scopes.push_back(scope);
	}

void push_scope(IntrusivePtr<zeek::detail::ID> id,
                std::unique_ptr<std::vector<IntrusivePtr<zeek::detail::Attr>>> attrs)
	{
	top_scope = new Scope(std::move(id), std::move(attrs));
	scopes.push_back(top_scope);
	}

IntrusivePtr<Scope> pop_scope()
	{
	if ( scopes.empty() )
		reporter->InternalError("scope underflow");
	scopes.pop_back();

	Scope* old_top = top_scope;

	top_scope = scopes.empty() ? nullptr : scopes.back();

	return {AdoptRef{}, old_top};
	}

Scope* current_scope()
	{
	return top_scope;
	}

Scope* global_scope()
	{
	return scopes.empty() ? 0 : scopes.front();
	}
