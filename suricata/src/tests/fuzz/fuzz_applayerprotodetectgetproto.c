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


#define HEADER_LEN 6

//rule of thumb constant, so as not to timeout target
#define PROTO_DETECT_MAX_LEN 1024

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

AppLayerProtoDetectThreadCtx *alpd_tctx = NULL;

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    Flow *f;
    TcpSession ssn;
    bool reverse;
    AppProto alproto;
    AppProto alproto2;

    if (size < HEADER_LEN) {
        return 0;
    }

    if (alpd_tctx == NULL) {
        //global init
        InitGlobal();
        run_mode = RUNMODE_UNITTEST;
        MpmTableSetup();
        SpmTableSetup();
        AppLayerProtoDetectSetup();
        AppLayerParserSetup();
        AppLayerParserRegisterProtocolParsers();
        alpd_tctx = AppLayerProtoDetectGetCtxThread();
    }

    f = TestHelperBuildFlow(AF_INET, "1.2.3.4", "5.6.7.8", (data[2] << 8) | data[3], (data[4] << 8) | data[5]);
    if (f == NULL) {
        return 0;
    }
    f->proto = data[1];
    memset(&ssn, 0, sizeof(TcpSession));
    f->protoctx = &ssn;
    f->protomap = FlowGetProtoMapping(f->proto);

    alproto = AppLayerProtoDetectGetProto(alpd_tctx, f, data+HEADER_LEN, size-HEADER_LEN, f->proto, data[0], &reverse);
    if (alproto != ALPROTO_UNKNOWN && alproto != ALPROTO_FAILED && f->proto == IPPROTO_TCP) {
        /* If we find a valid protocol :
         * check that with smaller input
         * we find the same protocol or ALPROTO_UNKNOWN.
         * Otherwise, we have evasion with TCP splitting
         */
        for (size_t i = 0; i < size-HEADER_LEN && i < PROTO_DETECT_MAX_LEN; i++) {
            alproto2 = AppLayerProtoDetectGetProto(alpd_tctx, f, data+HEADER_LEN, i, f->proto, data[0], &reverse);
            if (alproto2 != ALPROTO_UNKNOWN && alproto2 != alproto) {
                printf("Assertion failure : With input length %"PRIuMAX", found %s instead of %s\n", (uintmax_t) i, AppProtoToString(alproto2), AppProtoToString(alproto));
                abort();
            }
        }
    }
    FlowFree(f);

    return 0;
}
