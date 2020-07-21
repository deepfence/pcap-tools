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

// service_bootp.h author Sourcefire Inc.

#ifndef SERVICE_BOOTP_H
#define SERVICE_BOOTP_H

#include "service_detector.h"

#define DHCP_OP55_MAX_SIZE  64
#define DHCP_OP60_MAX_SIZE  64

class AppIdSession;
class ServiceDiscovery;

struct DHCPData
{
    DHCPData* next;
    unsigned op55_len;
    unsigned op60_len;
    uint8_t op55[DHCP_OP55_MAX_SIZE];
    uint8_t op60[DHCP_OP60_MAX_SIZE];
    uint8_t eth_addr[6];
};

struct DHCPInfo
{
    DHCPInfo* next;
    uint32_t ipAddr;
    uint8_t eth_addr[6];
    uint32_t subnetmask;
    uint32_t leaseSecs;
    uint32_t router;
};

class BootpServiceDetector : public ServiceDetector
{
public:
    BootpServiceDetector(ServiceDiscovery*);

    int validate(AppIdDiscoveryArgs&) override;

    // FIXIT-L - move to service discovery class
    static void AppIdFreeDhcpData(DHCPData*);
    static void AppIdFreeDhcpInfo(DHCPInfo*);

private:
    int add_dhcp_info(AppIdSession&, unsigned op55_len, const uint8_t* op55, unsigned
        op60_len, const uint8_t* op60, const uint8_t* mac);
    void add_new_dhcp_lease(AppIdSession&, const uint8_t* mac, uint32_t ip, int32_t zone,
        uint32_t subnetmask, uint32_t leaseSecs, uint32_t router);
};
#endif

