/* Copyright (C) 2014-2020 Open Information Security Foundation
 *
 * You can copy, redistribute or modify this Program under the terms of
 * the GNU General Public License version 2 as published by the Free
 * Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/**
 * \file
 *
 * \author Victor Julien <victor@inliniac.net>
 *
 * Implements Unidirectiontal NetFlow JSON logging portion of the engine.
 */

#include "suricata-common.h"
#include "debug.h"
#include "detect.h"
#include "pkt-var.h"
#include "conf.h"

#include "threads.h"
#include "threadvars.h"
#include "tm-threads.h"

#include "util-print.h"
#include "util-unittest.h"

#include "util-debug.h"

#include "output.h"
#include "util-privs.h"
#include "util-buffer.h"
#include "util-proto-name.h"
#include "util-logopenfile.h"
#include "util-time.h"
#include "output-json.h"
#include "output-json-netflow.h"

#include "stream-tcp-private.h"

typedef struct LogJsonFileCtx_ {
    LogFileCtx *file_ctx;
    OutputJsonCommonSettings cfg;
} LogJsonFileCtx;

typedef struct JsonNetFlowLogThread_ {
    LogJsonFileCtx *flowlog_ctx;
    /** LogFileCtx has the pointer to the file and a mutex to allow multithreading */

    MemBuffer *buffer;
} JsonNetFlowLogThread;

static JsonBuilder *CreateEveHeaderFromFlow(const Flow *f, const char *event_type, int dir)
{
    char timebuf[64];
    char srcip[46] = {0}, dstip[46] = {0};
    Port sp, dp;

    JsonBuilder *js = jb_new_object();
    if (unlikely(js == NULL))
        return NULL;

    struct timeval tv;
    memset(&tv, 0x00, sizeof(tv));
    TimeGet(&tv);

    CreateIsoTimeString(&tv, timebuf, sizeof(timebuf));

    /* reverse header direction if the flow started out wrong */
    dir ^= ((f->flags & FLOW_DIR_REVERSED) != 0);

    if (FLOW_IS_IPV4(f)) {
        if (dir == 0) {
            PrintInet(AF_INET, (const void *)&(f->src.addr_data32[0]), srcip, sizeof(srcip));
            PrintInet(AF_INET, (const void *)&(f->dst.addr_data32[0]), dstip, sizeof(dstip));
        } else {
            PrintInet(AF_INET, (const void *)&(f->dst.addr_data32[0]), srcip, sizeof(srcip));
            PrintInet(AF_INET, (const void *)&(f->src.addr_data32[0]), dstip, sizeof(dstip));
        }
    } else if (FLOW_IS_IPV6(f)) {
        if (dir == 0) {
            PrintInet(AF_INET6, (const void *)&(f->src.address), srcip, sizeof(srcip));
            PrintInet(AF_INET6, (const void *)&(f->dst.address), dstip, sizeof(dstip));
        } else {
            PrintInet(AF_INET6, (const void *)&(f->dst.address), srcip, sizeof(srcip));
            PrintInet(AF_INET6, (const void *)&(f->src.address), dstip, sizeof(dstip));
        }
    }

    if (dir == 0) {
        sp = f->sp;
        dp = f->dp;
    } else {
        sp = f->dp;
        dp = f->sp;
    }

    /* time */
    jb_set_string(js, "timestamp", timebuf);

    CreateEveFlowId(js, (const Flow *)f);

#if 0 // TODO
    /* sensor id */
    if (sensor_id >= 0)
        json_object_set_new(js, "sensor_id", json_integer(sensor_id));
#endif

    /* input interface */
    if (f->livedev) {
        jb_set_string(js, "in_iface", f->livedev->dev);
    }

    if (event_type) {
        jb_set_string(js, "event_type", event_type);
    }

    /* vlan */
    if (f->vlan_idx > 0) {
        jb_open_array(js, "vlan");
        jb_append_uint(js, f->vlan_id[0]);
        if (f->vlan_idx > 1) {
            jb_append_uint(js, f->vlan_id[1]);
        }
        jb_close(js);
    }

    /* tuple */
    jb_set_string(js, "src_ip", srcip);
    switch(f->proto) {
        case IPPROTO_ICMP:
            break;
        case IPPROTO_UDP:
        case IPPROTO_TCP:
        case IPPROTO_SCTP:
            jb_set_uint(js, "src_port", sp);
            break;
    }
    jb_set_string(js, "dest_ip", dstip);
    switch(f->proto) {
        case IPPROTO_ICMP:
            break;
        case IPPROTO_UDP:
        case IPPROTO_TCP:
        case IPPROTO_SCTP:
            jb_set_uint(js, "dest_port", dp);
            break;
    }

    if (SCProtoNameValid(f->proto)) {
        jb_set_string(js, "proto", known_proto[f->proto]);
    } else {
        char proto[4];
        snprintf(proto, sizeof(proto), "%"PRIu8"", f->proto);
        jb_set_string(js, "proto", proto);
    }

    switch (f->proto) {
        case IPPROTO_ICMP:
        case IPPROTO_ICMPV6: {
            uint8_t type = f->icmp_s.type;
            uint8_t code = f->icmp_s.code;
            if (dir == 1) {
                type = f->icmp_d.type;
                code = f->icmp_d.code;

            }
            jb_set_uint(js, "icmp_type", type);
            jb_set_uint(js, "icmp_code", code);
            break;
        }
    }
    return js;
}

/* JSON format logging */
static void NetFlowLogEveToServer(JsonNetFlowLogThread *aft, JsonBuilder *js, Flow *f)
{
    jb_set_string(js, "app_proto",
            AppProtoToString(f->alproto_ts ? f->alproto_ts : f->alproto));

    jb_open_object(js, "netflow");

    jb_set_uint(js, "pkts", f->todstpktcnt);
    jb_set_uint(js, "bytes", f->todstbytecnt);

    char timebuf1[64], timebuf2[64];

    CreateIsoTimeString(&f->startts, timebuf1, sizeof(timebuf1));
    CreateIsoTimeString(&f->lastts, timebuf2, sizeof(timebuf2));

    jb_set_string(js, "start", timebuf1);
    jb_set_string(js, "end", timebuf2);

    int32_t age = f->lastts.tv_sec - f->startts.tv_sec;
    jb_set_uint(js, "age", age);

    jb_set_uint(js, "min_ttl", f->min_ttl_toserver);
    jb_set_uint(js, "max_ttl", f->max_ttl_toserver);

    /* Close netflow. */
    jb_close(js);

    /* TCP */
    if (f->proto == IPPROTO_TCP) {
        jb_open_object(js, "tcp");

        TcpSession *ssn = f->protoctx;

        char hexflags[3];
        snprintf(hexflags, sizeof(hexflags), "%02x",
                ssn ? ssn->client.tcp_flags : 0);
        jb_set_string(js, "tcp_flags", hexflags);

        EveTcpFlags(ssn ? ssn->client.tcp_flags : 0, js);

        jb_close(js);
    }
}

static void NetFlowLogEveToClient(JsonNetFlowLogThread *aft, JsonBuilder *js, Flow *f)
{
    jb_set_string(js, "app_proto",
            AppProtoToString(f->alproto_tc ? f->alproto_tc : f->alproto));

    jb_open_object(js, "netflow");

    jb_set_uint(js, "pkts", f->tosrcpktcnt);
    jb_set_uint(js, "bytes", f->tosrcbytecnt);

    char timebuf1[64], timebuf2[64];

    CreateIsoTimeString(&f->startts, timebuf1, sizeof(timebuf1));
    CreateIsoTimeString(&f->lastts, timebuf2, sizeof(timebuf2));

    jb_set_string(js, "start", timebuf1);
    jb_set_string(js, "end", timebuf2);

    int32_t age = f->lastts.tv_sec - f->startts.tv_sec;
    jb_set_uint(js, "age", age);

    /* To client is zero if we did not see any packet */
    if (f->tosrcpktcnt) {
        jb_set_uint(js, "min_ttl", f->min_ttl_toclient);
        jb_set_uint(js, "max_ttl", f->max_ttl_toclient);
    }

    /* Close netflow. */
    jb_close(js);

    /* TCP */
    if (f->proto == IPPROTO_TCP) {
        jb_open_object(js, "tcp");

        TcpSession *ssn = f->protoctx;

        char hexflags[3];
        snprintf(hexflags, sizeof(hexflags), "%02x",
                ssn ? ssn->server.tcp_flags : 0);
        jb_set_string(js, "tcp_flags", hexflags);

        EveTcpFlags(ssn ? ssn->server.tcp_flags : 0, js);

        jb_close(js);
    }
}

static int JsonNetFlowLogger(ThreadVars *tv, void *thread_data, Flow *f)
{
    SCEnter();
    JsonNetFlowLogThread *jhl = (JsonNetFlowLogThread *)thread_data;
    LogJsonFileCtx *netflow_ctx = jhl->flowlog_ctx;

    /* reset */
    MemBufferReset(jhl->buffer);
    JsonBuilder *jb = CreateEveHeaderFromFlow(f, "netflow", 0);
    if (unlikely(jb == NULL))
        return TM_ECODE_OK;
    NetFlowLogEveToServer(jhl, jb, f);
    EveAddCommonOptions(&netflow_ctx->cfg, NULL, f, jb);
    OutputJsonBuilderBuffer(jb, jhl->flowlog_ctx->file_ctx, &jhl->buffer);
    jb_free(jb);

    /* only log a response record if we actually have seen response packets */
    if (f->tosrcpktcnt) {
        /* reset */
        MemBufferReset(jhl->buffer);
        jb = CreateEveHeaderFromFlow(f, "netflow", 1);
        if (unlikely(jb == NULL))
            return TM_ECODE_OK;
        NetFlowLogEveToClient(jhl, jb, f);
        EveAddCommonOptions(&netflow_ctx->cfg, NULL, f, jb);
        OutputJsonBuilderBuffer(jb, jhl->flowlog_ctx->file_ctx, &jhl->buffer);
        jb_free(jb);
    }
    SCReturnInt(TM_ECODE_OK);
}

static void OutputNetFlowLogDeinit(OutputCtx *output_ctx)
{
    LogJsonFileCtx *flow_ctx = output_ctx->data;
    LogFileCtx *logfile_ctx = flow_ctx->file_ctx;
    LogFileFreeCtx(logfile_ctx);
    SCFree(flow_ctx);
    SCFree(output_ctx);
}

#define DEFAULT_LOG_FILENAME "netflow.json"
static OutputInitResult OutputNetFlowLogInit(ConfNode *conf)
{
    OutputInitResult result = { NULL, false };
    LogFileCtx *file_ctx = LogFileNewCtx();
    if(file_ctx == NULL) {
        SCLogError(SC_ERR_NETFLOW_LOG_GENERIC, "couldn't create new file_ctx");
        return result;
    }

    if (SCConfLogOpenGeneric(conf, file_ctx, DEFAULT_LOG_FILENAME, 1) < 0) {
        LogFileFreeCtx(file_ctx);
        return result;
    }

    LogJsonFileCtx *flow_ctx = SCMalloc(sizeof(LogJsonFileCtx));
    if (unlikely(flow_ctx == NULL)) {
        LogFileFreeCtx(file_ctx);
        return result;
    }

    OutputCtx *output_ctx = SCCalloc(1, sizeof(OutputCtx));
    if (unlikely(output_ctx == NULL)) {
        LogFileFreeCtx(file_ctx);
        SCFree(flow_ctx);
        return result;
    }

    flow_ctx->file_ctx = file_ctx;
    output_ctx->data = flow_ctx;
    output_ctx->DeInit = OutputNetFlowLogDeinit;

    result.ctx = output_ctx;
    result.ok = true;
    return result;
}

static void OutputNetFlowLogDeinitSub(OutputCtx *output_ctx)
{
    LogJsonFileCtx *flow_ctx = output_ctx->data;
    SCFree(flow_ctx);
    SCFree(output_ctx);
}

static OutputInitResult OutputNetFlowLogInitSub(ConfNode *conf, OutputCtx *parent_ctx)
{
    OutputInitResult result = { NULL, false };
    OutputJsonCtx *ojc = parent_ctx->data;

    LogJsonFileCtx *flow_ctx = SCMalloc(sizeof(LogJsonFileCtx));
    if (unlikely(flow_ctx == NULL))
        return result;

    OutputCtx *output_ctx = SCCalloc(1, sizeof(OutputCtx));
    if (unlikely(output_ctx == NULL)) {
        SCFree(flow_ctx);
        return result;
    }

    flow_ctx->file_ctx = ojc->file_ctx;
    flow_ctx->cfg = ojc->cfg;

    output_ctx->data = flow_ctx;
    output_ctx->DeInit = OutputNetFlowLogDeinitSub;

    result.ctx = output_ctx;
    result.ok = true;
    return result;
}

static TmEcode JsonNetFlowLogThreadInit(ThreadVars *t, const void *initdata, void **data)
{
    JsonNetFlowLogThread *aft = SCMalloc(sizeof(JsonNetFlowLogThread));
    if (unlikely(aft == NULL))
        return TM_ECODE_FAILED;
    memset(aft, 0, sizeof(JsonNetFlowLogThread));

    if(initdata == NULL)
    {
        SCLogDebug("Error getting context for EveLogNetflow.  \"initdata\" argument NULL");
        SCFree(aft);
        return TM_ECODE_FAILED;
    }

    /* Use the Ouptut Context (file pointer and mutex) */
    aft->flowlog_ctx = ((OutputCtx *)initdata)->data; //TODO

    aft->buffer = MemBufferCreateNew(JSON_OUTPUT_BUFFER_SIZE);
    if (aft->buffer == NULL) {
        SCFree(aft);
        return TM_ECODE_FAILED;
    }

    *data = (void *)aft;
    return TM_ECODE_OK;
}

static TmEcode JsonNetFlowLogThreadDeinit(ThreadVars *t, void *data)
{
    JsonNetFlowLogThread *aft = (JsonNetFlowLogThread *)data;
    if (aft == NULL) {
        return TM_ECODE_OK;
    }

    MemBufferFree(aft->buffer);
    /* clear memory */
    memset(aft, 0, sizeof(JsonNetFlowLogThread));

    SCFree(aft);
    return TM_ECODE_OK;
}

void JsonNetFlowLogRegister(void)
{
    /* register as separate module */
    OutputRegisterFlowModule(LOGGER_JSON_NETFLOW, "JsonNetFlowLog",
        "netflow-json-log", OutputNetFlowLogInit, JsonNetFlowLogger,
        JsonNetFlowLogThreadInit, JsonNetFlowLogThreadDeinit, NULL);

    /* also register as child of eve-log */
    OutputRegisterFlowSubModule(LOGGER_JSON_NETFLOW, "eve-log", "JsonNetFlowLog",
        "eve-log.netflow", OutputNetFlowLogInitSub, JsonNetFlowLogger,
        JsonNetFlowLogThreadInit, JsonNetFlowLogThreadDeinit, NULL);
}
