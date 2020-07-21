//--------------------------------------------------------------------------
// Copyright (C) 2014-2020 Cisco and/or its affiliates. All rights reserved.
// Copyright (C) 2012-2013 Sourcefire, Inc.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 as published
// by the Free Software Foundation.  You may not use, modify or distribute
// this program under any other version of the GNU General Public License.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//--------------------------------------------------------------------------
/*
 ** Author(s):  Hui Cao <huica@cisco.com>
 **
 ** NOTES
 ** 8.15.15 - Initial Source Code. Hui Cao
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "file_flows.h"

#include "detection/detection_engine.h"
#include "log/messages.h"
#include "main/snort_config.h"
#include "managers/inspector_manager.h"
#include "protocols/packet.h"

#include "file_cache.h"
#include "file_config.h"
#include "file_lib.h"
#include "file_module.h"
#include "file_service.h"
#include "file_stats.h"

using namespace snort;

unsigned FileFlows::file_flow_data_id = 0;
static THREAD_LOCAL uint32_t max_file_id = 0;

namespace snort
{
    FilePosition get_file_position(Packet* pkt)
    {
        FilePosition position = SNORT_FILE_POSITION_UNKNOWN;
        Packet* p = (Packet*)pkt;

        if (p->is_full_pdu())
            position = SNORT_FILE_FULL;
        else if (p->is_pdu_start())
            position = SNORT_FILE_START;
        else if (p->packet_flags & PKT_PDU_TAIL)
            position = SNORT_FILE_END;
        else if (get_file_processed_size(p->flow))
            position = SNORT_FILE_MIDDLE;

        return position;
    }
}

void FileFlows::handle_retransmit(Packet* p)
{
    if (file_policy == nullptr)
        return;

    FileContext* file = get_file_context(pending_file_id, false);
    if ((file == nullptr) or (file->verdict != FILE_VERDICT_PENDING))
        return;

    FileVerdict verdict = file_policy->signature_lookup(p, file);
    FileCache* file_cache = FileService::get_file_cache();
    if (file_cache)
        file_cache->apply_verdict(p, file, verdict, false, file_policy);
    file->log_file_event(flow, file_policy);
}

FileFlows* FileFlows::get_file_flows(Flow* flow)
{

    FileFlows* fd = (FileFlows*)flow->get_flow_data(FileFlows::file_flow_data_id);

    if (fd)
        return fd;

    FileInspect* fi = (FileInspect*)InspectorManager::get_inspector(FILE_ID_NAME, true);

    if (FileService::is_file_service_enabled() and fi)
    {
        fd = new FileFlows(flow, fi);
        flow->set_flow_data(fd);
        if (fi->config)
            fd->set_file_policy(&(fi->config->get_file_policy()));
    }

    return fd;
}

FilePolicyBase* FileFlows::get_file_policy(Flow* flow)
{
    FileFlows* fd = (FileFlows*)flow->get_flow_data(FileFlows::file_flow_data_id);

    if (fd)
        return fd->get_file_policy();

    return nullptr;
}

void FileFlows::set_current_file_context(FileContext* ctx)
{
    // If we finished processing a file context object last time, delete it
    if (current_context_delete_pending)
    {
        delete current_context;
        current_context_delete_pending = false;
    }
    current_context = ctx;
    // Not using current_file_id so clear it
    current_file_id = 0;
}

FileContext* FileFlows::get_current_file_context()
{
    if (current_file_id)
    {
        return get_file_context(current_file_id, false);
    }
    return current_context;
}

uint64_t FileFlows::get_new_file_instance()
{
    uint64_t thread_id = get_instance_id();
    return ((thread_id << 32) | max_file_id++);
}

FileFlows::~FileFlows()
{
    delete(main_context);
    if (current_context_delete_pending)
        delete(current_context);

    // Delete any remaining FileContexts stored on the flow
    for (auto const& elem : partially_processed_contexts)
    {
        delete elem.second;
    }
}

FileContext* FileFlows::find_main_file_context(FilePosition pos, FileDirection dir, size_t index)
{
    /* Attempt to get a previously allocated context. */
    FileContext* context = main_context;

    if (context)
    {
        if ((pos == SNORT_FILE_MIDDLE)or (pos == SNORT_FILE_END))
            return context;
        else
            delete context;
    }

    context = new FileContext;
    main_context = context;
    context->check_policy(flow, dir, file_policy);

    if (!index)
        context->set_file_id(get_new_file_instance());
    else
        context->set_file_id(index);

    return context;
}

FileContext* FileFlows::get_partially_processed_context(uint64_t file_id)
{
    auto elem = partially_processed_contexts.find(file_id);
    if (elem != partially_processed_contexts.end())
        return elem->second;
    return nullptr;
}

FileContext* FileFlows::get_file_context(
    uint64_t file_id, bool to_create, uint64_t multi_file_processing_id)
{
    // First check if this file is currently being processed
    if (!multi_file_processing_id)
        multi_file_processing_id = file_id;
    FileContext *context = get_partially_processed_context(multi_file_processing_id);

    // Otherwise check if it has been fully processed and is in the file cache. If the file is not
    // in the cache, don't add it.
    if (!context)
    {
        FileCache* file_cache = FileService::get_file_cache();
        assert(file_cache);
        context = file_cache->get_file(flow, file_id, false);
    }

    // If we haven't found the context, create it and store it on the file flows object
    if (!context and to_create)
    {
        // If we have reached the max file per flow limit, alert and increment the peg count
        FileConfig* fc = get_file_config(SnortConfig::get_conf());
        if (partially_processed_contexts.size() == fc->max_files_per_flow)
        {
            file_counts.files_over_flow_limit_not_processed++;
            events.create_event(EVENT_FILE_DROPPED_OVER_LIMIT);
        }
        else
        {
            context = new FileContext;
            partially_processed_contexts[multi_file_processing_id] = context;
            if (partially_processed_contexts.size() > file_counts.max_concurrent_files_per_flow)
                file_counts.max_concurrent_files_per_flow = partially_processed_contexts.size();
        }
    }

    return context;
}

// Remove a file context from the flow's partially processed store. Don't delete the context
// yet because detection needs access; pointer is stored in current_context. The file context will
// be deleted when the next file is processed
void FileFlows::remove_processed_file_context(uint64_t file_id)
{
    FileContext *context = get_partially_processed_context(file_id);
    partially_processed_contexts.erase(file_id);
    if (context)
        current_context_delete_pending = true;
}

/* This function is used to process file that is sent in pieces
 *
 * Return:
 *    true: continue processing/log/block this file
 *    false: ignore this file
 */
bool FileFlows::file_process(Packet* p, uint64_t file_id, const uint8_t* file_data,
    int data_size, uint64_t offset, FileDirection dir, uint64_t multi_file_processing_id,
    FilePosition position)
{
    int64_t file_depth = FileService::get_max_file_depth();
    bool continue_processing;
    if (!multi_file_processing_id)
        multi_file_processing_id = file_id;

    if ((file_depth < 0) or (offset > (uint64_t)file_depth))
    {
        return false;
    }

    FileContext* context = get_file_context(file_id, true, multi_file_processing_id);

    if (!context)
        return false;

    set_current_file_context(context);

    if (!context->get_processed_bytes())
    {
        context->check_policy(flow, dir, file_policy);
        context->set_file_id(file_id);
    }

    if ( offset != 0 and
        (FileService::get_file_cache()->cached_verdict_lookup(p, context, file_policy) !=
            FILE_VERDICT_UNKNOWN) )
    {
        context->processing_complete = true;
        remove_processed_file_context(multi_file_processing_id);
        return false;
    }

    if (context->processing_complete and context->verdict != FILE_VERDICT_UNKNOWN)
    {
        /*A new file session, but policy might be different*/
        context->check_policy(flow, dir, file_policy);

        if ((context->get_file_sig_sha256()) || !context->is_file_signature_enabled())
        {
            /* Just check file type and signature */
            continue_processing = context->process(p, file_data, data_size, SNORT_FILE_FULL,
                    file_policy);
            if (context->processing_complete)
                remove_processed_file_context(multi_file_processing_id);
            return continue_processing;
        }
    }

    continue_processing = context->process(p, file_data, data_size, offset, file_policy, position);
    if (context->processing_complete)
        remove_processed_file_context(multi_file_processing_id);
    return continue_processing;
}

/*
 * Return:
 *    true: continue processing/log/block this file
 *    false: ignore this file
 */
bool FileFlows::file_process(Packet* p, const uint8_t* file_data, int data_size,
    FilePosition position, bool upload, size_t file_index)
{
    FileContext* context;
    FileDirection direction = upload ? FILE_UPLOAD : FILE_DOWNLOAD;
    /* if both disabled, return immediately*/
    if (!FileService::is_file_service_enabled())
        return false;

    if (position == SNORT_FILE_POSITION_UNKNOWN)
        return false;

    context = find_main_file_context(position, direction, file_index);

    set_current_file_context(context);

    context->set_signature_state(gen_signature);
    return context->process(p, file_data, data_size, position, file_policy);
}

void FileFlows::set_file_name(const uint8_t* fname, uint32_t name_size, uint64_t file_id)
{
    FileContext* context;
    if (file_id)
        context = get_file_context(file_id, false);
    else
        context = get_current_file_context();
    if ( !context )
        return;

    if ( !context->is_file_name_set() )
    {
        context->set_file_name((const char*)fname, name_size);
        context->log_file_event(flow, file_policy);
    }
}

void FileFlows::add_pending_file(uint64_t file_id)
{
    current_file_id = pending_file_id = file_id;
}

FileInspect::FileInspect(FileIdModule* fm)
{
    fm->load_config(config);
}

FileInspect:: ~FileInspect()
{
    if (config)
        delete config;
}

bool FileInspect::configure(SnortConfig*)
{
    if (!config)
        return true;

    FileCache* file_cache = FileService::get_file_cache();
    if (file_cache)
    {
        file_cache->set_block_timeout(config->file_block_timeout);
        file_cache->set_lookup_timeout(config->file_lookup_timeout);
        file_cache->set_max_files(config->max_files_cached);
    }

    return true;
}

static void file_config_show(const FileConfig* fc)
{
    const FilePolicy& fp = fc->get_file_policy();

    if ( ConfigLogger::log_flag("enable_type", fp.get_file_type()) )
        ConfigLogger::log_value("type_depth", fc->file_type_depth);

    if ( ConfigLogger::log_flag("enable_signature", fp.get_file_signature()) )
        ConfigLogger::log_value("signature_depth", fc->file_signature_depth);

    if ( ConfigLogger::log_flag("block_timeout_lookup", fc->block_timeout_lookup) )
        ConfigLogger::log_value("block_timeout", fc->file_block_timeout);

    if ( ConfigLogger::log_flag("enable_capture", fp.get_file_capture()) )
    {
        ConfigLogger::log_value("capture_memcap", fc->capture_memcap);
        ConfigLogger::log_value("capture_max_size", fc->capture_max_size);
        ConfigLogger::log_value("capture_min_size", fc->capture_min_size);
        ConfigLogger::log_value("capture_block_size", fc->capture_block_size);
    }

    ConfigLogger::log_value("lookup_timeout", fc->file_lookup_timeout);
    ConfigLogger::log_value("max_files_cached", fc->max_files_cached);
    ConfigLogger::log_value("max_files_per_flow", fc->max_files_per_flow);
    ConfigLogger::log_value("show_data_depth", fc->show_data_depth);

    ConfigLogger::log_flag("trace_type", fc->trace_type);
    ConfigLogger::log_flag("trace_signature", fc->trace_signature);
    ConfigLogger::log_flag("trace_stream", fc->trace_stream);
    ConfigLogger::log_value("verdict_delay", fc->verdict_delay);
}

void FileInspect::show(const SnortConfig*) const
{
    if ( config )
        file_config_show(config);
}

static Module* mod_ctor()
{ return new FileIdModule; }

static void mod_dtor(Module* m)
{ delete m; }

static void file_init()
{
    FileFlows::init();
}

static void file_term()
{
}

static Inspector* file_ctor(Module* m)
{
    FileIdModule* mod = (FileIdModule*)m;
    return new FileInspect(mod);
}

static void file_dtor(Inspector* p)
{
    delete p;
}

static const InspectApi file_inspect_api =
{
    {
        PT_INSPECTOR,
        sizeof(InspectApi),
        INSAPI_VERSION,
        0,
        API_RESERVED,
        API_OPTIONS,
        FILE_ID_NAME,
        FILE_ID_HELP,
        mod_ctor,
        mod_dtor
    },
    IT_PASSIVE,
    PROTO_BIT__NONE,
    nullptr,
    "file",
    file_init,
    file_term,
    nullptr, // tinit
    nullptr, // tterm
    file_ctor,
    file_dtor,
    nullptr, // ssn
    nullptr  // reset
};

const BaseApi* sin_file_flow = &file_inspect_api.base;
