//--------------------------------------------------------------------------
// Copyright (C) 2014-2020 Cisco and/or its affiliates. All rights reserved.
// Copyright (C) 2005-2013 Sourcefire, Inc.
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

// appid_api.h author Sourcefire Inc.

#ifndef APPID_API_H
#define APPID_API_H

#include "flow/flow.h"
#include "sfip/sf_ip.h"
#include "appid_session_api.h"
#include "application_ids.h"

enum class IpProtocol : uint8_t;

class AppIdContext;
class AppIdSession;

namespace snort
{

#define APPID_HA_SESSION_APP_NUM_MAX 8    // maximum number of appIds replicated for a flow/session

struct AppIdSessionHA
{
    uint16_t flags;
    AppId appId[APPID_HA_SESSION_APP_NUM_MAX];
};

// -----------------------------------------------------------------------------
// AppId API
// -----------------------------------------------------------------------------

class SO_PUBLIC AppIdApi
{
public:
    SO_PRIVATE AppIdApi() = default;

    AppIdSession* get_appid_session(const Flow& flow);
    const char* get_application_name(AppId app_id, const AppIdContext& ctxt);
    const char* get_application_name(const Flow& flow, bool from_client);
    AppId get_application_id(const char* appName, const AppIdContext& ctxt);
    uint32_t produce_ha_state(const Flow& flow, uint8_t* buf);
    uint32_t consume_ha_state(Flow& flow, const uint8_t* buf, uint8_t length, IpProtocol,
        SfIp*, uint16_t initiatorPort);
    bool ssl_app_group_id_lookup(Flow* flow, const char*, const char*, const char*,
        const char*, bool, AppId& service_id, AppId& client_id, AppId& payload_id);
    const AppIdSessionApi* get_appid_session_api(const Flow& flow) const;
    bool is_inspection_needed(const Inspector& g) const;
};

SO_PUBLIC extern AppIdApi appid_api;
}
#endif

