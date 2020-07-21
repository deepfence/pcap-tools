/**
 * @file
 * @author Philippe Antoine <contact@catenacyber.fr>
 * fuzz target for AppLayerProtoDetectGetProto
 */


#include "suricata-common.h"
#include "app-layer-detect-proto.h"
#include "flow-util.h"
#include "app-layer-parser.h"
#include "util-unittest-helper.h"
#include "util-byte.h"

#define HEADER_LEN 6

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

AppLayerParserThreadCtx *alp_tctx = NULL;

/* input buffer is structured this way :
 * 6 bytes header,
 * then sequence of buffers separated by magic bytes 01 D5 CA 7A */

/* The 6 bytes header is
 * alproto
 * proto
 * source port (uint16_t)
 * destination port (uint16_t) */

const uint8_t separator[] = {0x01, 0xD5, 0xCA, 0x7A};
SCInstance surifuzz;
uint64_t forceLayer = 0;

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    Flow * f;
    TcpSession ssn;
    const uint8_t * albuffer;
    uint8_t * alnext;
    size_t alsize;
    // used to find under and overflows
    // otherwise overflows do not fail as they read the next packet
    uint8_t * isolatedBuffer;

    if (size < HEADER_LEN) {
        return 0;
    }

    if (alp_tctx == NULL) {
        //Redirects logs to /dev/null
        setenv("SC_LOG_OP_IFACE", "file", 0);
        setenv("SC_LOG_FILE", "/dev/null", 0);

        InitGlobal();
        run_mode = RUNMODE_PCAP_FILE;
        GlobalsInitPreConfig();

        //redirect logs to /tmp
        ConfigSetLogDirectory("/tmp/");

        PostConfLoadedSetup(&surifuzz);
        alp_tctx = AppLayerParserThreadCtxAlloc();
        const char* forceLayerStr = getenv("FUZZ_APPLAYER");
        if (forceLayerStr) {
            if (ByteExtractStringUint64(&forceLayer, 10, 0, forceLayerStr) < 0) {
                forceLayer = 0;
                printf("Invalid numeric value for FUZZ_APPLAYER environment variable");
            }
        }
    }

    if (data[0] >= ALPROTO_MAX) {
        return 0;
    }
    //no UTHBuildFlow to have storage
    f = FlowAlloc();
    if (f == NULL) {
        return 0;
    }
    f->flags |= FLOW_IPV4;
    f->src.addr_data32[0] = 0x01020304;
    f->dst.addr_data32[0] = 0x05060708;
    f->sp = (data[2] << 8) | data[3];
    f->dp = (data[4] << 8) | data[5];
    f->proto = data[1];
    memset(&ssn, 0, sizeof(TcpSession));
    f->protoctx = &ssn;
    f->protomap = FlowGetProtoMapping(f->proto);
    if (forceLayer > 0) {
        f->alproto = forceLayer;
    } else {
        f->alproto = data[0];
    }

    /*
     * We want to fuzz multiple calls to AppLayerParserParse
     * because some parts of the code are only reached after
     * multiple packets (in SMTP for example).
     * So we treat our input as a list of buffers with magic separator.
     */
    albuffer = data + HEADER_LEN;
    alsize = size - HEADER_LEN;
    uint8_t flags = STREAM_START;
    int flip = 0;
    alnext = memmem(albuffer, alsize, separator, 4);
    while (alnext) {
        if (flip) {
            flags |= STREAM_TOCLIENT;
            flags &= ~(STREAM_TOSERVER);
            flip = 0;
        } else {
            flags |= STREAM_TOSERVER;
            flags &= ~(STREAM_TOCLIENT);
            flip = 1;
        }

        if (alnext != albuffer) {
            // only if we have some data
            isolatedBuffer = malloc(alnext - albuffer);
            if (isolatedBuffer == NULL) {
                return 0;
            }
            memcpy(isolatedBuffer, albuffer, alnext - albuffer);
            (void) AppLayerParserParse(NULL, alp_tctx, f, f->alproto, flags, isolatedBuffer, alnext - albuffer);
            free(isolatedBuffer);
            flags &= ~(STREAM_START);
            if (f->alparser && AppLayerParserStateIssetFlag(f->alparser, APP_LAYER_PARSER_EOF)) {
                //no final chunk
                alsize = 0;
                break;
            }
        }
        alsize -= alnext - albuffer + 4;
        albuffer = alnext + 4;
        if (alsize == 0) {
            break;
        }
        alnext = memmem(albuffer, alsize, separator, 4);
    }
    if (alsize > 0 ) {
        if (flip) {
            flags |= STREAM_TOCLIENT;
            flags &= ~(STREAM_TOSERVER);
            flip = 0;
        } else {
            flags |= STREAM_TOSERVER;
            flags &= ~(STREAM_TOCLIENT);
            flip = 1;
        }
        flags |= STREAM_EOF;
        isolatedBuffer = malloc(alsize);
        if (isolatedBuffer == NULL) {
            return 0;
        }
        memcpy(isolatedBuffer, albuffer, alsize);
        (void) AppLayerParserParse(NULL, alp_tctx, f, f->alproto, flags, isolatedBuffer, alsize);
        free(isolatedBuffer);
    }

    FlowFree(f);

    return 0;
}
