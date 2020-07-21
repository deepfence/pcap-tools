//--------------------------------------------------------------------------
// Copyright (C) 2017-2020 Cisco and/or its affiliates. All rights reserved.
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

// appid_app_descriptor.h author davis mcpherson <davmcphe@cisco.com>

#ifndef APPID_APP_DESCRIPTOR_H
#define APPID_APP_DESCRIPTOR_H

// The ApplicationDescriptor class and its subclasses contain the state info for
// detected applications.  It provides and API for detectors to call when an
// application is detected to set, get, update, or reset this information.
// When the application is first detected or when it is updated to a different
// application than the current setting the PegCount statistic for that application
// is incremented.

#include <string>

#include "protocols/packet.h"
#include "pub_sub/appid_events.h"

#include "appid_types.h"
#include "application_ids.h"

class AppIdDetector;
class AppIdSession;
class OdpContext;

class ApplicationDescriptor
{
public:
    ApplicationDescriptor() = default;
    virtual ~ApplicationDescriptor() = default;

    virtual void reset()
    {
        my_id = APP_ID_NONE;
        my_vendor.clear();
        my_version.clear();
    }

    virtual void update(AppId id, AppidChangeBits& change_bits, char* version)
    {
        set_id(id);
        set_version(version, change_bits);
    }

    virtual void update_stats(AppId id) = 0;

    AppId get_id() const
    {
        return my_id;
    }

    virtual void set_id(AppId app_id);

    virtual void set_id(const snort::Packet& p, AppIdSession& asd, AppidSessionDirection dir, AppId app_id, AppidChangeBits& change_bits);

    const char* get_vendor() const
    {
        return my_vendor.empty() ? nullptr : my_vendor.c_str();
    }

    void set_vendor(const char* vendor)
    {
        if ( vendor )
            my_vendor = vendor;
    }

    const char* get_version() const
    {
        return my_version.empty() ? nullptr : my_version.c_str();
    }

    void set_version(const char* version, AppidChangeBits& change_bits)
    {
        if ( version )
        {
            my_version = version;
            change_bits.set(APPID_VERSION_BIT);
        }
    }

private:
    AppId my_id = APP_ID_NONE;
    std::string my_vendor;
    std::string my_version;
};

class ServiceAppDescriptor : public ApplicationDescriptor
{
public:
    ServiceAppDescriptor() = default;

    void set_id(AppId app_id, OdpContext& odp_ctxt);

    void reset() override
    {
        ApplicationDescriptor::reset();
        port_service_id = APP_ID_NONE;
    }

    void update_stats(AppId id) override;

    AppId get_port_service_id() const
    {
        return port_service_id;
    }

    void set_port_service_id(AppId id);

    bool get_deferred() const
    {
        return deferred;
    }

private:
    AppId port_service_id = APP_ID_NONE;
    bool deferred = false;
    using ApplicationDescriptor::set_id;
};

class ClientAppDescriptor : public ApplicationDescriptor
{
public:
    ClientAppDescriptor() = default;

    void reset() override
    {
        ApplicationDescriptor::reset();
        my_username.clear();
        my_user_id = APP_ID_NONE;
    }

    void update_user(AppId app_id, const char* username);

    AppId get_user_id() const
    {
        return my_user_id;
    }

    const char* get_username() const
    {
        return my_username.empty() ? nullptr : my_username.c_str();
    }

    void update_stats(AppId id) override;

private:
    std::string my_username;
    AppId my_user_id = APP_ID_NONE;
};

class PayloadAppDescriptor : public ApplicationDescriptor
{
public:
    PayloadAppDescriptor() = default;

    void reset() override
    {
        ApplicationDescriptor::reset();
    }

    void update_stats(AppId id) override;
};

#endif
