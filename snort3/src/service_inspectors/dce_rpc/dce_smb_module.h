//--------------------------------------------------------------------------
// Copyright (C) 2016-2020 Cisco and/or its affiliates. All rights reserved.
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
//
// dce_smb_module.h author Rashmi Pitre <rrp@cisco.com>

#ifndef DCE_SMB_MODULE_H
#define DCE_SMB_MODULE_H

#include "dce_common.h"
#include "framework/module.h"
#include "dce_list.h"

namespace snort
{
class Trace;
struct SnortConfig;
}

extern THREAD_LOCAL const snort::Trace* dce_smb_trace;

#define DCE2_VALID_SMB_VERSION_FLAG_V1 1
#define DCE2_VALID_SMB_VERSION_FLAG_V2 2

enum dce2SmbFingerprintPolicy
{
    DCE2_SMB_FINGERPRINT_POLICY_NONE = 0,
    DCE2_SMB_FINGERPRINT_POLICY_CLIENT,
    DCE2_SMB_FINGERPRINT_POLICY_SERVER,
    DCE2_SMB_FINGERPRINT_POLICY_BOTH,
};

struct dce2SmbShare
{
    char* unicode_str;
    unsigned int unicode_str_len;
    char* ascii_str;
    unsigned int ascii_str_len;
};

struct dce2SmbProtoConf
{
    dce2CoProtoConf common; // This member must be first
    dce2SmbFingerprintPolicy smb_fingerprint_policy;
    uint8_t smb_max_chain;
    uint8_t smb_max_compound;
    uint16_t smb_valid_versions_mask;
    int16_t smb_file_depth;
    DCE2_List* smb_invalid_shares;
    bool legacy_mode;
    uint16_t smb_max_credit;
    size_t memcap;
};

class Dce2SmbModule : public snort::Module
{
public:
    Dce2SmbModule();
    ~Dce2SmbModule() override;

    bool set(const char*, snort::Value&, snort::SnortConfig*) override;

    unsigned get_gid() const override
    { return GID_DCE2; }

    const snort::RuleMap* get_rules() const override;
    const PegInfo* get_pegs() const override;
    PegCount* get_counts() const override;
    snort::ProfileStats* get_profile() const override;
    void get_data(dce2SmbProtoConf&);

    Usage get_usage() const override
    { return INSPECT; }

    void set_trace(const snort::Trace*) const override;
    const snort::TraceOption* get_trace_options() const override;

private:
    dce2SmbProtoConf config;
};

void print_dce2_smb_conf(const dce2SmbProtoConf&);

inline int64_t DCE2_ScSmbFileDepth(const dce2SmbProtoConf* sc)
{
    return sc->smb_file_depth;
}

inline uint8_t DCE2_ScSmbMaxChain(const dce2SmbProtoConf* sc)
{
    if (sc == nullptr)
        return 0;
    return sc->smb_max_chain;
}

inline DCE2_List* DCE2_ScSmbInvalidShares(const dce2SmbProtoConf* sc)
{
    if (sc == nullptr)
        return nullptr;
    return sc->smb_invalid_shares;
}

inline uint16_t DCE2_ScSmbMaxCredit(const dce2SmbProtoConf* sc)
{
    if (sc == nullptr)
        return 8192;
    return sc->smb_max_credit;
}

inline size_t DCE2_ScSmbMemcap(const dce2SmbProtoConf* sc)
{
    if (sc == nullptr)
        return 8388608;
    return sc->memcap;
}

inline uint16_t DCE2_ScSmbMaxCompound(const dce2SmbProtoConf* sc)
{
    if (sc == nullptr)
        return 3;
    return sc->smb_max_compound;
}

inline bool DCE2_GcSmbFingerprintClient(const dce2SmbProtoConf* sc)
{
    if (sc == nullptr)
        return false;
    return (sc->smb_fingerprint_policy
           & DCE2_SMB_FINGERPRINT_POLICY_CLIENT) ? true : false;
}

inline bool DCE2_GcSmbFingerprintServer(const dce2SmbProtoConf* sc)
{
    if (sc == nullptr)
        return false;
    return (sc->smb_fingerprint_policy
           & DCE2_SMB_FINGERPRINT_POLICY_SERVER) ? true : false;
}

inline bool DCE2_GcIsLegacyMode(const dce2SmbProtoConf* sc)
{
    if (sc == nullptr)
        return false;
    return sc->legacy_mode;
}

#endif

