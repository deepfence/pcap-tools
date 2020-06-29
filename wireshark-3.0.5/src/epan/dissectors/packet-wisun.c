/* packet-wisun.c
 *
 * Wi-SUN IE Dissectors for Wireshark
 * By Owen Kirby <osk@exegin.com>
 * Copyright 2007 Exegin Technologies Limited
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *------------------------------------------------------------
 */
#include "config.h"
#include <epan/packet.h>
#include <epan/expert.h>
#include <epan/proto_data.h>
#include <wsutil/pint.h>
#include <epan/reassemble.h>

#include "packet-ieee802154.h"

void proto_register_wisun(void);
void proto_reg_handoff_wisun(void);

/* EDFE support */
typedef struct {
    ieee802154_map_rec initiator;
    ieee802154_map_rec target;
} edfe_exchange_t;

// for each exchange, maps both source address and destination address to a
// (distinct) tree that uses the frame number of the first exchange frame to
// map to the shared edfe_exchange_t
static wmem_map_t* edfe_byaddr;

static dissector_handle_t eapol_handle;  // for the eapol relay

static dissector_handle_t ieee802154_nofcs_handle;  // for Netricity Segment Control

static reassembly_table netricity_reassembly_table;


/* Wi-SUN Header IE Sub-ID Values. */
#define WISUN_SUBID_UTT     1
#define WISUN_SUBID_BT      2
#define WISUN_SUBID_FC      3
#define WISUN_SUBID_RSL     4
#define WISUN_SUBID_MHDS    5
#define WISUN_SUBID_VH      6
#define WISUN_SUBID_N_NFT   7
#define WISUN_SUBID_N_LQI   8
#define WISUN_SUBID_EA      9

/* Wi-SUN Payload/Nested ID values. */
#define WISUN_PIE_SUBID_US      1
#define WISUN_PIE_SUBID_BS      2
#define WISUN_PIE_SUBID_VP      3
#define WISUN_PIE_SUBID_PAN     4
#define WISUN_PIE_SUBID_NETNAME 5
#define WISUN_PIE_SUBID_PENVER  6
#define WISUN_PIE_SUBID_GTKHASH 7

#define WISUN_CHANNEL_PLAN      0x07
#define WISUN_CHANNEL_FUNCTION  0x38
#define WISUN_CHANNEL_EXCLUDE   0xc0

#define WISUN_CHANNEL_PLAN_REGULATORY   0
#define WISUN_CHANNEL_PLAN_EXPLICIT     1
#define WISUN_CHANNEL_FUNCTION_FIXED    0
#define WISUN_CHANNEL_FUNCTION_TR51CF   1
#define WISUN_CHANNEL_FUNCTION_DH1CF    2
#define WISUN_CHANNEL_FUNCTION_VENDOR   3
#define WISUN_CHANNEL_EXCLUDE_NONE      0
#define WISUN_CHANNEL_EXCLUDE_RANGE     1
#define WISUN_CHANNEL_EXCLUDE_MASK      2

#define WISUN_CH_PLAN_EXPLICIT_FREQ     0x00ffffff
#define WISUN_CH_PLAN_EXPLICIT_RESERVED 0xf0000000
#define WISUN_CH_PLAN_EXPLICIT_SPACING  0x0f000000

#define WISUN_EAPOL_RELAY_UDP_PORT 10253

static int proto_wisun = -1;
static int hf_wisun_subid = -1;
static int hf_wisun_unknown_ie = -1;
static int hf_wisun_uttie = -1;
static int hf_wisun_uttie_type = -1;
static int hf_wisun_uttie_ufsi = -1;
static int hf_wisun_btie = -1;
static int hf_wisun_btie_slot = -1;
static int hf_wisun_btie_bio = -1;
static int hf_wisun_fcie = -1;
static int hf_wisun_fcie_tx = -1;
static int hf_wisun_fcie_rx = -1;
static int hf_wisun_fcie_src = -1;
static int hf_wisun_fcie_initial_frame = -1;
static int hf_wisun_rslie = -1;
static int hf_wisun_rslie_rsl = -1;
static int hf_wisun_vhie = -1;
static int hf_wisun_vhie_vid = -1;
static int hf_wisun_eaie = -1;
static int hf_wisun_eaie_eui = -1;
static int hf_wisun_pie = -1;
static int hf_wisun_wsie = -1;
static int hf_wisun_wsie_type = -1;
static int hf_wisun_wsie_id = -1;
static int hf_wisun_wsie_length = -1;
static int hf_wisun_wsie_id_short = -1;
static int hf_wisun_wsie_length_short = -1;
static int hf_wisun_usie = -1;
static int hf_wisun_usie_dwell_interval = -1;
static int hf_wisun_usie_clock_drift = -1;
static int hf_wisun_usie_timing_accuracy = -1;
static int hf_wisun_usie_channel_control = -1;
static int hf_wisun_usie_channel_plan = -1;
static int hf_wisun_usie_channel_function = -1;
static int hf_wisun_usie_channel_exclude = -1;
static int hf_wisun_usie_regulatory_domain = -1;
static int hf_wisun_usie_operating_class = -1;
static int hf_wisun_usie_explicit = -1;
static int hf_wisun_usie_explicit_frequency = -1;
static int hf_wisun_usie_explicit_reserved = -1;
static int hf_wisun_usie_explicit_spacing = -1;
static int hf_wisun_usie_number_channels = -1;
static int hf_wisun_usie_fixed_channel = -1;
static int hf_wisun_usie_hop_count = -1;
static int hf_wisun_usie_hop_list = -1;
static int hf_wisun_usie_number_ranges = -1;
static int hf_wisun_usie_exclude_range = -1;
static int hf_wisun_usie_exclude_mask = -1;
static int hf_wisun_bsie = -1;
static int hf_wisun_bsie_bcast_interval = -1;
static int hf_wisun_bsie_bcast_schedule_id = -1;
static int hf_wisun_vpie = -1;
static int hf_wisun_vpie_vid = -1;
static int hf_wisun_panie = -1;
static int hf_wisun_panie_size = -1;
static int hf_wisun_panie_cost = -1;
static int hf_wisun_panie_flags = -1;
static int hf_wisun_panie_flag_parent_bsie = -1;
static int hf_wisun_panie_flag_routing_method = -1;
static int hf_wisun_panie_flag_version = -1;
static int hf_wisun_netnameie = -1;
static int hf_wisun_netnameie_name = -1;
static int hf_wisun_panverie = -1;
static int hf_wisun_panverie_version = -1;
static int hf_wisun_gtkhashie = -1;
static int hf_wisun_gtkhashie_gtk0 = -1;
static int hf_wisun_gtkhashie_gtk1 = -1;
static int hf_wisun_gtkhashie_gtk2 = -1;
static int hf_wisun_gtkhashie_gtk3 = -1;

static int proto_wisun_sec = -1;
static int hf_wisun_sec_function = -1;
static int hf_wisun_sec_error_type = -1;
static int hf_wisun_sec_error_nonce = -1;

// EAPOL Relay
static dissector_handle_t wisun_eapol_relay_handle;
static int proto_wisun_eapol_relay = -1;
static int hf_wisun_eapol_relay_sup = -1;
static int hf_wisun_eapol_relay_kmp_id = -1;
static int hf_wisun_eapol_relay_direction = -1;

// Netricity
static int proto_wisun_netricity_sc;
static int hf_wisun_netricity_nftie = -1;
static int hf_wisun_netricity_nftie_type = -1;
static int hf_wisun_netricity_lqiie = -1;
static int hf_wisun_netricity_lqiie_lqi = -1;
static int hf_wisun_netricity_sc_flags = -1;
static int hf_wisun_netricity_sc_reserved = -1;
static int hf_wisun_netricity_sc_tone_map_request = -1;
static int hf_wisun_netricity_sc_contention_control = -1;
static int hf_wisun_netricity_sc_channel_access_priority = -1;
static int hf_wisun_netricity_sc_last_segment = -1;
static int hf_wisun_netricity_sc_segment_count = -1;
static int hf_wisun_netricity_sc_segment_length = -1;
// Reassembly
static int hf_wisun_netricity_scr_segments = -1;
static int hf_wisun_netricity_scr_segment = -1;
static int hf_wisun_netricity_scr_segment_overlap = -1;
static int hf_wisun_netricity_scr_segment_overlap_conflicts = -1;
static int hf_wisun_netricity_scr_segment_multiple_tails = -1;
static int hf_wisun_netricity_scr_segment_too_long_segment = -1;
static int hf_wisun_netricity_scr_segment_error = -1;
static int hf_wisun_netricity_scr_segment_count = -1;
static int hf_wisun_netricity_scr_reassembled_in = -1;
static int hf_wisun_netricity_scr_reassembled_length = -1;

static gint ett_wisun_unknown_ie = -1;
static gint ett_wisun_uttie = -1;
static gint ett_wisun_btie = -1;
static gint ett_wisun_fcie = -1;
static gint ett_wisun_rslie = -1;
static gint ett_wisun_vhie = -1;
static gint ett_wisun_eaie = -1;
static gint ett_wisun_pie = -1;
static gint ett_wisun_wsie_bitmap = -1;
static gint ett_wisun_usie = -1;
static gint ett_wisun_bsie = -1;
static gint ett_wisun_vpie = -1;
static gint ett_wisun_usie_channel_control;
static gint ett_wisun_usie_explicit;
static gint ett_wisun_panie = -1;
static gint ett_wisun_panie_flags = -1;
static gint ett_wisun_netnameie = -1;
static gint ett_wisun_panverie = -1;
static gint ett_wisun_gtkhashie = -1;
static gint ett_wisun_sec = -1;
static gint ett_wisun_eapol_relay = -1;
// Netricity
static gint ett_wisun_netricity_nftie = -1;
static gint ett_wisun_netricity_lqiie = -1;
static gint ett_wisun_netricity_sc = -1;
static gint ett_wisun_netricity_sc_bitmask = -1;
static gint ett_wisun_netricity_scr_segment = -1;
static gint ett_wisun_netricity_scr_segments = -1;

static const fragment_items netricity_scr_frag_items = {
        /* Fragment subtrees */
        &ett_wisun_netricity_scr_segment,
        &ett_wisun_netricity_scr_segments,

        /* Fragment fields */
        &hf_wisun_netricity_scr_segments,
        &hf_wisun_netricity_scr_segment,
        &hf_wisun_netricity_scr_segment_overlap,
        &hf_wisun_netricity_scr_segment_overlap_conflicts,
        &hf_wisun_netricity_scr_segment_multiple_tails,
        &hf_wisun_netricity_scr_segment_too_long_segment,
        &hf_wisun_netricity_scr_segment_error,
        &hf_wisun_netricity_scr_segment_count,

        /* Reassembled in field */
        &hf_wisun_netricity_scr_reassembled_in,

        /* Reassembled length field */
        &hf_wisun_netricity_scr_reassembled_length,
        /* Reassembled data field */
        NULL,

        /* Tag */
        "Netricity Segments"
};

static const value_string wisun_wsie_types[] = {
    { 0, "Short" },
    { 1, "Long" },
    { 0, NULL }
};

static const value_string wisun_subid_vals[] = {
    { WISUN_SUBID_UTT,      "Unicast Timing IE" },
    { WISUN_SUBID_BT,       "Broadcast Timing IE" },
    { WISUN_SUBID_FC,       "Flow Control IE" },
    { WISUN_SUBID_RSL,      "Received Signal Level IE" },
    { WISUN_SUBID_MHDS,     "Multi-Hop Delivery Service IE" },
    { WISUN_SUBID_VH,       "Vendor Header IE" },
    { WISUN_SUBID_N_NFT,    "Netricity Frame Type IE" },
    { WISUN_SUBID_N_LQI,    "Link Quality Index IE" },
    { WISUN_SUBID_EA,       "EAPOL Authenticator EUI-64 IE" },
    { 0, NULL }
};

static const value_string wisun_wsie_names[] = {
    { WISUN_PIE_SUBID_US,       "Unicast Schedule IE" },
    { WISUN_PIE_SUBID_BS,       "Broadcast Schedule IE" },
    { WISUN_PIE_SUBID_VP,       "Vendor Payload IE" },
    { WISUN_PIE_SUBID_PAN,      "PAN Information IE" },
    { WISUN_PIE_SUBID_NETNAME,  "Network Name IE" },
    { WISUN_PIE_SUBID_PENVER,   "PAN Version IE" },
    { WISUN_PIE_SUBID_GTKHASH,  "GTK Hash IE" },
    { 0, NULL }
};

static const value_string wisun_frame_type_vals[] = {
    { 0, "PAN Advertisement" },
    { 1, "PAN Advertisement Solicit" },
    { 2, "PAN Configuration" },
    { 3, "PAN Configuration Solicit" },
    { 4, "Data" },
    { 5, "Acknowledgment" },
    { 6, "EAPOL" },
    { 0, NULL }
};

static const value_string wisun_usie_clock_drift_names[] = {
    { 0,   "Better than 1ppm" },
    { 255, "Not provided" },
    { 0, NULL }
};

static const value_string wisun_channel_plan_names[] = {
    { WISUN_CHANNEL_PLAN_REGULATORY,    "Regulatory Domain and Operating Class" },
    { WISUN_CHANNEL_PLAN_EXPLICIT,      "Explicit Spacing and Number" },
    { 0, NULL }
};

static const value_string wisun_channel_function_names[] = {
    { WISUN_CHANNEL_FUNCTION_FIXED,     "Fixed Channel" },
    { WISUN_CHANNEL_FUNCTION_TR51CF,    "TR51 Channel Function" },
    { WISUN_CHANNEL_FUNCTION_DH1CF,     "Direct Hash Channel Function" },
    { WISUN_CHANNEL_FUNCTION_VENDOR,    "Vendor Defined Channel Function" },
    { 0, NULL }
};

static const value_string wisun_channel_exclude_names[] = {
    { WISUN_CHANNEL_EXCLUDE_NONE,       "None" },
    { WISUN_CHANNEL_EXCLUDE_RANGE,      "Ranges" },
    { WISUN_CHANNEL_EXCLUDE_MASK,       "Bitmask" },
    { 0, NULL }
};

static const value_string wisun_channel_regulatory_domains_names[] = {
    {  0, "World"          },
    {  1, "North America"  },
    {  2, "Japan"          },
    {  3, "Europe"         },
    {  4, "China"          },
    {  5, "India"          },
    {  6, "Mexico"         },
    {  7, "Brazil"         },
    {  8, "Australia / NZ" },
    {  9, "Korea"          },
    { 10, "Philippines"    },
    { 11, "Malaysia"       },
    { 12, "Hong Kong"      },
    { 13, "Singapore"      },
    { 14, "Thailand"       },
    { 15, "Vietnam"        },
    {  0, NULL             }
};

static const value_string wisun_channel_spacing_names[] = {
    { 0, "200 kHz" },
    { 1, "400 kHz" },
    { 2, "600 kHz" },
    { 3, "100 kHz" },
    { 0, NULL }
};

static const value_string wisun_routing_methods[] = {
    { 0, "MHDS" },
    { 1, "RPL" },
    { 0, NULL }
};

static const value_string wisun_sec_functions[] = {
    { 0x01, "SM Error" },
    { 0, NULL }
};

static const value_string wisun_sec_sm_errors[] = {
    { 0x01, "No Session" },
    { 0x02, "Unavailable Key" },
    { 0x03, "Unsupported Security" },
    { 0x04, "Invalid Parameter" },
    { 0x06, "Unsupported Security" },
    { 0, NULL }
};

static const true_false_string wisun_netricity_sc_contention_control_tfs = {
    "Contention-free access",
    "Contention allowed in next contention state"
};

static const int * wisun_format_nested_ie[] = {
    &hf_wisun_wsie_type,
    &hf_wisun_wsie_id,
    &hf_wisun_wsie_length,
    NULL
};

static const int * wisun_format_nested_ie_short[] = {
    &hf_wisun_wsie_type,
    &hf_wisun_wsie_id_short,
    &hf_wisun_wsie_length_short,
    NULL
};

static expert_field ei_wisun_subid_unsupported = EI_INIT;
static expert_field ei_wisun_wsie_unsupported = EI_INIT;
static expert_field ei_wisun_usie_channel_plan_invalid = EI_INIT;
static expert_field ei_wisun_edfe_start_not_found = EI_INIT;
static expert_field ei_wisun_usie_explicit_reserved_bits_not_zero = EI_INIT;

static guint
wisun_add_wbxml_uint(tvbuff_t *tvb, proto_tree *tree, int hf, guint offset)
{
    guint val = 0;
    guint len = 0;
    guint8 b;
    do {
        b = tvb_get_guint8(tvb, offset + len++);
        val = (val << 7) | (b & 0x7f);
    } while (b & 0x80);
    proto_tree_add_uint(tree, hf, tvb, offset, len, val);
    return len;
}

/*-----------------------------------------------
 * Wi-SUN Header IE Dissection
 *---------------------------------------------*/
static proto_tree *
wisun_create_hie_tree(tvbuff_t *tvb, proto_tree *tree, int hf, gint ett)
{
    proto_tree *subtree = ieee802154_create_hie_tree(tvb, tree, hf, ett);
    proto_tree_add_item(subtree, hf_wisun_subid, tvb, 2, 1, ENC_LITTLE_ENDIAN);
    return subtree;
}

static int
dissect_wisun_uttie(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint offset)
{
    guint8 frame_type = tvb_get_guint8(tvb, offset);
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "Wi-SUN");
    col_set_str(pinfo->cinfo, COL_INFO, val_to_str_const(frame_type, wisun_frame_type_vals, "Unknown Wi-SUN Frame"));
    proto_tree_add_item(tree, hf_wisun_uttie_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(tree, hf_wisun_uttie_ufsi, tvb, offset+1, 3, ENC_LITTLE_ENDIAN);
    return 4;
}

static int
dissect_wisun_btie(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, guint offset)
{
    proto_tree_add_item(tree, hf_wisun_btie_slot, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    /* as of FAN TPS 1v14, this is 3 bytes instead of 4 */
    proto_tree_add_item(tree, hf_wisun_btie_bio, tvb, offset+2, 3, ENC_LITTLE_ENDIAN);
    return 5;
}

static void
edfe_insert_exchange(guint64* addr, edfe_exchange_t* exchange)
{
    wmem_tree_t* byframe = (wmem_tree_t *)wmem_map_lookup(edfe_byaddr, addr);
    if (!byframe) {
        byframe = wmem_tree_new(wmem_file_scope());
        wmem_map_insert(edfe_byaddr, addr, byframe);
    }
    wmem_tree_insert32(byframe, exchange->initiator.start_fnum, exchange);
}

static int
dissect_wisun_fcie(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint offset, ieee802154_packet *packet)
{
    guint32 tx;
    proto_tree_add_item_ret_uint(tree, hf_wisun_fcie_tx, tvb, offset, 1, ENC_LITTLE_ENDIAN, &tx);
    guint32 rx;
    proto_tree_add_item_ret_uint(tree, hf_wisun_fcie_rx, tvb, offset+1, 1, ENC_LITTLE_ENDIAN, &rx);

    // EDFE processing
    ieee802154_hints_t* hints = (ieee802154_hints_t *)p_get_proto_data(wmem_file_scope(), pinfo,
                                                                       proto_get_id_by_filter_name(IEEE802154_PROTOABBREV_WPAN), 0);
    if (packet && hints && packet->dst_addr_mode == IEEE802154_FCF_ADDR_EXT) {
        // first packet has source address
        if (!hints->map_rec && packet->src_addr_mode == IEEE802154_FCF_ADDR_EXT) {
            edfe_exchange_t* ex = wmem_new(wmem_file_scope(), edfe_exchange_t);
            ex->initiator.proto = "Wi-SUN";
            ex->target.proto = "Wi-SUN";
            ex->initiator.start_fnum = pinfo->num;
            ex->target.start_fnum = pinfo->num;
            ex->initiator.end_fnum = ~(guint)0;
            ex->target.end_fnum = ~(guint)0;

            ex->initiator.addr64 = packet->src64;
            ex->target.addr64 = packet->dst64;
            edfe_insert_exchange(&ex->initiator.addr64, ex);
            edfe_insert_exchange(&ex->target.addr64, ex);
        } else if (packet->src_addr_mode == IEEE802154_FCF_ADDR_NONE) {
            if (!hints->map_rec) {
                wmem_tree_t* byframe = (wmem_tree_t *)wmem_map_lookup(edfe_byaddr, &packet->dst64);
                if (byframe) {
                    edfe_exchange_t *ex = (edfe_exchange_t *)wmem_tree_lookup32_le(byframe, pinfo->num);
                    if (ex && pinfo->num <= ex->initiator.end_fnum) {
                        hints->map_rec = ex->initiator.addr64 == packet->dst64 ? &ex->target : &ex->initiator;
                        if (tx == 0 && rx == 0) {
                            // last frame
                            ex->initiator.end_fnum = pinfo->num;
                        }
                    }
                }
            }
            if (hints->map_rec) {
                // Set address to ensure that 6LoWPAN reassembly works
                // Adapted from packet-ieee802.15.4.c
                guint64 *p_addr = wmem_new(pinfo->pool, guint64);
                /* Copy and convert the address to network byte order. */
                *p_addr = pntoh64(&(hints->map_rec->addr64));
                set_address(&pinfo->dl_src, AT_EUI64, 8, p_addr);
                copy_address_shallow(&pinfo->src, &pinfo->dl_src);
                proto_item* src = proto_tree_add_eui64(tree, hf_wisun_fcie_src, tvb, 0, 0, hints->map_rec->addr64);
                PROTO_ITEM_SET_GENERATED(src);
                proto_item* frm = proto_tree_add_uint(tree, hf_wisun_fcie_initial_frame, tvb, 0, 0, hints->map_rec->start_fnum);
                PROTO_ITEM_SET_GENERATED(frm);
            } else {
                expert_add_info(pinfo, tree, &ei_wisun_edfe_start_not_found);
            }
        }
    }

    return 2;
}

static int
dissect_wisun_rslie(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, guint offset)
{
    guint32 rsl = tvb_get_guint8(tvb, offset);
    if (rsl == 0xff) {
        // "A value of 255 MUST be used to indicate not measured" [FANTPS 1v21]
        proto_tree_add_uint_format_value(tree, hf_wisun_rslie_rsl, tvb, offset, 1, rsl, "not measured");
    } else {
        // "This provides a range of -174 (0) to +80 (254) dBm" [FANTPS 1v21]
        proto_tree_add_uint_format_value(tree, hf_wisun_rslie_rsl, tvb, offset, 1, rsl, "%d dBm", rsl-174);
    }
    return 1;
}

static int
dissect_wisun_vhie(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint offset)
{
    guint vidlen = wisun_add_wbxml_uint(tvb, tree, hf_wisun_vhie_vid, offset);
    call_data_dissector(tvb_new_subset_remaining(tvb, offset + vidlen), pinfo, tree);
    return tvb_reported_length(tvb);
}

static int
dissect_wisun_eaie(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, guint offset)
{
    proto_tree_add_item(tree, hf_wisun_eaie_eui, tvb, offset, 8, ENC_BIG_ENDIAN);
    return 8;
}

static int
dissect_wisun_netricity_nftie(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, guint offset)
{
    guint8 frame_type = tvb_get_guint8(tvb, offset);
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "Wi-SUN Netricity");
    col_set_str(pinfo->cinfo, COL_INFO, val_to_str_const(frame_type, wisun_frame_type_vals, "Unknown Wi-SUN Netricity Frame"));
    proto_tree_add_item(tree, hf_wisun_netricity_nftie_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    return 1;
}

static int
dissect_wisun_netricity_lqiie(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, guint offset)
{
    guint8 lqi = tvb_get_guint8(tvb, offset);
    switch (lqi) {
        case 0:
            // "-10 dB or lower (0x00)" [IEEE1901.2-2013]
            proto_tree_add_uint_format_value(tree, hf_wisun_netricity_lqiie_lqi, tvb, offset, 1, lqi, "0 (SNR -10 dB or lower)");
            break;
        case 255:
            // "A value of 255 MUST be used to indicate 'not measured'" [NTPS 1v04]
            proto_tree_add_uint_format_value(tree, hf_wisun_netricity_lqiie_lqi, tvb, offset, 1, lqi, "255 (not measured)");
            break;
        default:
            // "where the value of -9.75 dB is represented as 0x01 and the value of 52.75 dB is represented as 0xFE" [IEEE1901.2-2013]
            proto_tree_add_uint_format_value(tree, hf_wisun_netricity_lqiie_lqi, tvb, offset, 1, lqi, "%d (SNR %1.2f dB)", lqi,
                                             (lqi-1) * (52.75+9.75) / (0xfe - 0x01) - 9.75);
    }
    return 1;
}

static int
dissect_wisun_hie(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    guint offset;
    proto_tree *subtree;
    guint8 subid = tvb_get_guint8(tvb, 2);
    ieee802154_packet *packet = (ieee802154_packet*)data;

    offset = 3;
    switch (subid) {
        case WISUN_SUBID_UTT:
            subtree = wisun_create_hie_tree(tvb, tree, hf_wisun_uttie, ett_wisun_uttie);
            offset += dissect_wisun_uttie(tvb, pinfo, subtree, offset);
            break;

        case WISUN_SUBID_BT:
            subtree = wisun_create_hie_tree(tvb, tree, hf_wisun_btie, ett_wisun_btie);
            offset += dissect_wisun_btie(tvb, pinfo, subtree, offset);
            break;

        case WISUN_SUBID_FC:
            subtree = wisun_create_hie_tree(tvb, tree, hf_wisun_fcie, ett_wisun_fcie);
            offset += dissect_wisun_fcie(tvb, pinfo, subtree, offset, packet);
            break;

        case WISUN_SUBID_RSL:
            subtree = wisun_create_hie_tree(tvb, tree, hf_wisun_rslie, ett_wisun_rslie);
            offset += dissect_wisun_rslie(tvb, pinfo, subtree, offset);
            break;

        case WISUN_SUBID_VH:
            subtree = wisun_create_hie_tree(tvb, tree, hf_wisun_vhie, ett_wisun_vhie);
            offset += dissect_wisun_vhie(tvb, pinfo, subtree, offset);
            break;

        case WISUN_SUBID_N_NFT:
            subtree = wisun_create_hie_tree(tvb, tree, hf_wisun_netricity_nftie, ett_wisun_netricity_nftie);
            offset += dissect_wisun_netricity_nftie(tvb, pinfo, subtree, offset);
            break;

        case WISUN_SUBID_N_LQI:
            subtree = wisun_create_hie_tree(tvb, tree, hf_wisun_netricity_lqiie, ett_wisun_netricity_lqiie);
            offset += dissect_wisun_netricity_lqiie(tvb, pinfo, subtree, offset);
            break;

        case WISUN_SUBID_EA:
            subtree = wisun_create_hie_tree(tvb, tree, hf_wisun_eaie, ett_wisun_eaie);
            offset += dissect_wisun_eaie(tvb, pinfo, subtree, offset);
            break;

        default:
            subtree = wisun_create_hie_tree(tvb, tree, hf_wisun_unknown_ie, ett_wisun_unknown_ie);
            expert_add_info(pinfo, subtree, &ei_wisun_subid_unsupported);
            call_data_dissector(tvb_new_subset_remaining(tvb, offset), pinfo, subtree);
            offset = tvb_reported_length(tvb);
            break;
    }
    return offset;
}

/*-----------------------------------------------
 * Wi-SUN Payload IE Dissection
 *---------------------------------------------*/
static int
dissect_wisun_schedule_common(tvbuff_t *tvb, packet_info *pinfo, guint offset, proto_tree *tree)
{
    static const int * fields_usie_channel[] = {
            &hf_wisun_usie_channel_plan,
            &hf_wisun_usie_channel_function,
            &hf_wisun_usie_channel_exclude,
            NULL
    };

    static const int * fields_usie_channel_plan_explicit[] = {
            &hf_wisun_usie_explicit_frequency,
            &hf_wisun_usie_explicit_spacing,
            &hf_wisun_usie_explicit_reserved,
            NULL
    };
    gint count;
    proto_item *ti;

    proto_tree_add_item(tree, hf_wisun_usie_dwell_interval, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset++;
    proto_tree_add_item(tree, hf_wisun_usie_clock_drift, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset++;
    guint clock_drift = tvb_get_guint8(tvb, offset);
    // "Resolution is 10 microseconds and the valid range of the field value is 0-255 (0 to 2.55msec)" [FANTPS 1v21]
    proto_tree_add_uint_format_value(tree, hf_wisun_usie_timing_accuracy, tvb, offset, 1, clock_drift, "%1.2fms", clock_drift/100.0);
    offset++;
    guint8 control = tvb_get_guint8(tvb, offset);
    proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_wisun_usie_channel_control, ett_wisun_usie_channel_control,
                                      fields_usie_channel, ENC_LITTLE_ENDIAN, BMT_NO_FLAGS);
    offset++;

    switch ((control & WISUN_CHANNEL_PLAN) >> 0) {
        case WISUN_CHANNEL_PLAN_REGULATORY:
            proto_tree_add_item(tree, hf_wisun_usie_regulatory_domain, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset++;
            proto_tree_add_item(tree, hf_wisun_usie_operating_class, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset++;
            break;

        case WISUN_CHANNEL_PLAN_EXPLICIT:
            ti = proto_tree_add_bitmask(tree, tvb, offset, hf_wisun_usie_explicit, ett_wisun_usie_explicit,
                                        fields_usie_channel_plan_explicit, ENC_LITTLE_ENDIAN);
            offset += 3;
            if (tvb_get_guint8(tvb, offset) & 0xf0) {
                expert_add_info(pinfo, ti, &ei_wisun_usie_explicit_reserved_bits_not_zero);
            }
            offset++;
            proto_tree_add_item(tree, hf_wisun_usie_number_channels, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            break;

        default:
            expert_add_info(pinfo, tree, &ei_wisun_usie_channel_plan_invalid);
            return tvb_reported_length(tvb);
    }

    switch ((control & WISUN_CHANNEL_FUNCTION) >> 3) {
        case WISUN_CHANNEL_FUNCTION_FIXED:
            proto_tree_add_item(tree, hf_wisun_usie_fixed_channel, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            break;

        case WISUN_CHANNEL_FUNCTION_VENDOR:
            count = tvb_get_guint8(tvb, offset);
            proto_tree_add_item(tree, hf_wisun_usie_hop_count, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset++;
            while (count--) {
                proto_tree_add_item(tree, hf_wisun_usie_hop_list, tvb, offset, 1, ENC_LITTLE_ENDIAN);
                offset++;
            }
            break;

        default:
            /* Hopping list is implied by the hashing function. */
            break;
    }

    switch ((control & WISUN_CHANNEL_EXCLUDE) >> 6) {
        case WISUN_CHANNEL_EXCLUDE_RANGE:
            count = tvb_get_guint8(tvb, offset);
            proto_tree_add_item(tree, hf_wisun_usie_number_ranges, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset++;
            while (count) {
                guint16 ex_start = tvb_get_letohs(tvb, offset);
                guint16 ex_end = tvb_get_letohs(tvb, offset+2);
                proto_tree_add_uint_format_value(tree, hf_wisun_usie_exclude_range, tvb, offset, 4, ex_start, "[%u-%u]", ex_start, ex_end);
                offset += 4;
                count--;
            }
            break;

        case WISUN_CHANNEL_EXCLUDE_MASK:
            count = tvb_reported_length_remaining(tvb, offset);
            proto_tree_add_item(tree, hf_wisun_usie_exclude_mask, tvb, offset, count, ENC_NA);
            offset += count;
            break;

        default:
            /* Assume there is nothing to exclude. */
            break;
    }
    return offset;
}

static int
dissect_wisun_usie(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    proto_item *item;
    proto_tree *subtree;
    guint offset = 0;

    item = proto_tree_add_item(tree, hf_wisun_usie, tvb, 0, tvb_reported_length_remaining(tvb, 0), ENC_NA);
    subtree = proto_item_add_subtree(item, ett_wisun_usie);

    proto_tree_add_bitmask(subtree, tvb, offset, hf_wisun_wsie, ett_wisun_wsie_bitmap, wisun_format_nested_ie, ENC_LITTLE_ENDIAN);
    offset += 2;

    return dissect_wisun_schedule_common(tvb, pinfo, offset, subtree);
}

static int
dissect_wisun_bsie(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    proto_item *item;
    proto_tree *subtree;
    guint offset = 0;

    item = proto_tree_add_item(tree, hf_wisun_bsie, tvb, 0, tvb_reported_length(tvb), ENC_NA);
    subtree = proto_item_add_subtree(item, ett_wisun_bsie);

    proto_tree_add_bitmask(subtree, tvb, offset, hf_wisun_wsie, ett_wisun_wsie_bitmap, wisun_format_nested_ie, ENC_LITTLE_ENDIAN);
    offset += 2;
    proto_tree_add_item(subtree, hf_wisun_bsie_bcast_interval, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    proto_tree_add_item(subtree, hf_wisun_bsie_bcast_schedule_id, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;

    return dissect_wisun_schedule_common(tvb, pinfo, offset, subtree);
}

static int
dissect_wisun_vpie(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, void *data _U_)
{
    proto_item *item;
    proto_tree *subtree;
    guint vidlen;

    item = proto_tree_add_item(tree, hf_wisun_vpie, tvb, 0, tvb_reported_length(tvb), ENC_NA);
    subtree = proto_item_add_subtree(item, ett_wisun_vpie);

    vidlen = wisun_add_wbxml_uint(tvb, subtree, hf_wisun_vpie_vid, 2);
    call_data_dissector(tvb_new_subset_remaining(tvb, 2 + vidlen), pinfo, subtree);
    return tvb_reported_length(tvb);
}

static int
dissect_wisun_panie(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, void *data _U_)
{
    proto_item *item;
    proto_tree *subtree;
    guint offset = 0;
    guint32 routingCost;
    static const int * fields_panie_flags[] = {
        &hf_wisun_panie_flag_parent_bsie,
        &hf_wisun_panie_flag_routing_method,
        &hf_wisun_panie_flag_version,
        NULL
    };

    item = proto_tree_add_item(tree, hf_wisun_panie, tvb, 0, tvb_reported_length(tvb), ENC_NA);
    subtree = proto_item_add_subtree(item, ett_wisun_panie);

    proto_tree_add_bitmask(subtree, tvb, offset, hf_wisun_wsie, ett_wisun_wsie_bitmap, wisun_format_nested_ie_short, ENC_LITTLE_ENDIAN);
    offset += 2;
    proto_tree_add_item(subtree, hf_wisun_panie_size, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    proto_tree_add_item_ret_uint(subtree, hf_wisun_panie_cost, tvb, offset, 2, ENC_LITTLE_ENDIAN, &routingCost);
    offset += 2;
    proto_tree_add_bitmask_with_flags(subtree, tvb, offset, hf_wisun_panie_flags, ett_wisun_panie_flags,
                            fields_panie_flags, ENC_LITTLE_ENDIAN, BMT_NO_FLAGS);
    offset++;

    col_append_sep_fstr(pinfo->cinfo, COL_INFO, ", ", "Routing Cost: %d", routingCost);

    return offset;
}

static int
dissect_wisun_netnameie(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, void *data _U_)
{
    proto_item *item;
    proto_tree *subtree;

    item = proto_tree_add_item(tree, hf_wisun_netnameie, tvb, 0, tvb_reported_length(tvb), ENC_NA);
    subtree = proto_item_add_subtree(item, ett_wisun_netnameie);

    proto_tree_add_bitmask(subtree, tvb, 0, hf_wisun_wsie, ett_wisun_wsie_bitmap, wisun_format_nested_ie_short, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(subtree, hf_wisun_netnameie_name, tvb, 2, tvb_reported_length_remaining(tvb, 2), ENC_ASCII|ENC_NA);

    col_append_sep_fstr(pinfo->cinfo, COL_INFO, ", ", "Netname: %s",
                        tvb_get_string_enc(wmem_packet_scope(), tvb, 2, tvb_reported_length_remaining(tvb, 2), ENC_ASCII));
    return tvb_reported_length(tvb);
}

static int
dissect_wisun_panverie(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, void *data _U_)
{
    proto_item *item;
    proto_tree *subtree;

    item = proto_tree_add_item(tree, hf_wisun_panverie, tvb, 0, tvb_reported_length(tvb), ENC_NA);
    subtree = proto_item_add_subtree(item, ett_wisun_panverie);

    proto_tree_add_bitmask(subtree, tvb, 0, hf_wisun_wsie, ett_wisun_wsie_bitmap, wisun_format_nested_ie_short, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(subtree, hf_wisun_panverie_version, tvb, 2, 2, ENC_LITTLE_ENDIAN);

    col_append_sep_fstr(pinfo->cinfo, COL_INFO, ", ", "PAN Version: %d", tvb_get_guint16(tvb, 2, ENC_LITTLE_ENDIAN));

    return tvb_reported_length(tvb);
}

static int
dissect_wisun_gtkhashie(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, void *data _U_)
{
    proto_item *item;
    proto_tree *subtree;

    item = proto_tree_add_item(tree, hf_wisun_gtkhashie, tvb, 0, tvb_reported_length(tvb), ENC_NA);
    subtree = proto_item_add_subtree(item, ett_wisun_gtkhashie);

    proto_tree_add_bitmask(subtree, tvb, 0, hf_wisun_wsie, ett_wisun_wsie_bitmap, wisun_format_nested_ie_short, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(subtree, hf_wisun_gtkhashie_gtk0, tvb, 2, 8, ENC_NA);
    proto_tree_add_item(subtree, hf_wisun_gtkhashie_gtk1, tvb, 10, 8, ENC_NA);
    proto_tree_add_item(subtree, hf_wisun_gtkhashie_gtk2, tvb, 18, 8, ENC_NA);
    proto_tree_add_item(subtree, hf_wisun_gtkhashie_gtk3, tvb, 26, 8, ENC_NA);
    return 34;
}

static int
dissect_wisun_pie(tvbuff_t *tvb, packet_info *pinfo, proto_tree *ies_tree, void *data)
{
    proto_tree *tree = ieee802154_create_pie_tree(tvb, ies_tree, hf_wisun_pie, ett_wisun_pie);
    guint offset = 2;
    while (tvb_reported_length_remaining(tvb, offset) > 1) {
        /* Wi-SUN Payload IE contains nested IE's using the same format as IEEE802.15.4 */
        guint16     wsie_ie = tvb_get_letohs(tvb, offset);
        guint16     wsie_len;
        tvbuff_t *  wsie_tvb;

        if (wsie_ie & IEEE802154_PSIE_TYPE_MASK) {
            /* long format: Table 7-17-Sub-ID allocation for long format */
            wsie_len = (wsie_ie & IEEE802154_PSIE_LENGTH_MASK_LONG);
            wsie_tvb = tvb_new_subset_length(tvb, offset, wsie_len + 2);
            switch ((wsie_ie & IEEE802154_PSIE_ID_MASK_LONG) >> 11) {
                case WISUN_PIE_SUBID_US:
                    dissect_wisun_usie(wsie_tvb, pinfo, tree, data);
                    break;
                case WISUN_PIE_SUBID_BS:
                    dissect_wisun_bsie(wsie_tvb, pinfo, tree, data);
                    break;
                case WISUN_PIE_SUBID_VP:
                    dissect_wisun_vpie(wsie_tvb, pinfo, tree, data);
                    break;

                default:{
                    proto_item *item = proto_tree_add_item(tree, hf_wisun_unknown_ie, wsie_tvb, 0, tvb_reported_length(wsie_tvb), ENC_NA);
                    proto_tree *subtree = proto_item_add_subtree(item, ett_wisun_unknown_ie);
                    proto_tree_add_bitmask(subtree, wsie_tvb, 0, hf_wisun_wsie, ett_wisun_wsie_bitmap, wisun_format_nested_ie, ENC_LITTLE_ENDIAN);
                    expert_add_info(pinfo, subtree, &ei_wisun_wsie_unsupported);
                    call_data_dissector(tvb_new_subset_remaining(wsie_tvb, 2), pinfo, subtree);
                    break;
                }
            }
        } else {
            /* short format: Table 7-16-Sub-ID allocation for short format */
            wsie_len = (wsie_ie & IEEE802154_PSIE_LENGTH_MASK_SHORT);
            wsie_tvb = tvb_new_subset_length(tvb, offset, wsie_len + 2);
            switch ((wsie_ie & IEEE802154_PSIE_ID_MASK_SHORT) >> 8) {
                case WISUN_PIE_SUBID_PAN:
                    dissect_wisun_panie(wsie_tvb, pinfo, tree, data);
                    break;
                case WISUN_PIE_SUBID_NETNAME:
                    dissect_wisun_netnameie(wsie_tvb, pinfo, tree, data);
                    break;
                case WISUN_PIE_SUBID_PENVER:
                    dissect_wisun_panverie(wsie_tvb, pinfo, tree, data);
                    break;
                case WISUN_PIE_SUBID_GTKHASH:
                    dissect_wisun_gtkhashie(wsie_tvb, pinfo, tree, data);
                    break;

                default:{
                    proto_item *item = proto_tree_add_item(tree, hf_wisun_unknown_ie, wsie_tvb, 0, tvb_reported_length(wsie_tvb), ENC_NA);
                    proto_tree *subtree = proto_item_add_subtree(item, ett_wisun_unknown_ie);
                    proto_tree_add_bitmask(subtree, wsie_tvb, 0, hf_wisun_wsie, ett_wisun_wsie_bitmap, wisun_format_nested_ie, ENC_LITTLE_ENDIAN);
                    expert_add_info(pinfo, subtree, &ei_wisun_wsie_unsupported);
                    call_data_dissector(tvb_new_subset_remaining(wsie_tvb, 2), pinfo, subtree);
                    break;
                }
            }
        }
        offset += tvb_reported_length(wsie_tvb);
    }
    return offset;
}

/*-----------------------------------------------
 * Wi-SUN FAN Security Extensions Dissection
 *---------------------------------------------*/
static int
dissect_wisun_sec(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    proto_item *ws_root;
    proto_tree *ws_tree;
    guint8 function = tvb_get_guint8(tvb, 0);
    const char *function_name = val_to_str_const(function, wisun_sec_functions, "Unknown Function");

    /* Create the protocol tree. */
    ws_root = proto_tree_add_protocol_format(tree, proto_wisun_sec, tvb, 0, -1, "Wi-SUN %s", function_name);
    ws_tree = proto_item_add_subtree(ws_root, ett_wisun_sec);

    /* Add the protocol name. */
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "Wi-SUN");
    col_set_str(pinfo->cinfo, COL_INFO, function_name);

    /* Decode based on the function code. */
    proto_tree_add_item(ws_tree, hf_wisun_sec_function, tvb, 0, 1, ENC_LITTLE_ENDIAN);
    switch (function) {
        case 0x01:{
            const char *err_name = val_to_str_const(tvb_get_guint8(tvb, 1), wisun_sec_sm_errors, "Unknown Error");
            col_append_fstr(pinfo->cinfo, COL_INFO, ": %s", err_name);
            proto_item_append_text(ws_root, ": %s", err_name);

            proto_tree_add_item(ws_tree, hf_wisun_sec_error_type, tvb, 1, 1, ENC_LITTLE_ENDIAN);
            proto_tree_add_item(ws_tree, hf_wisun_sec_error_nonce, tvb, 2, tvb_reported_length_remaining(tvb, 2), ENC_NA);
            return tvb_reported_length(tvb);
        }

        default:
            call_data_dissector(tvb_new_subset_remaining(tvb, 2), pinfo, ws_tree);
            return tvb_reported_length(tvb);
    }
}


/*-----------------------------------------------
 * Wi-SUN FAN EAPOL Relay
 *---------------------------------------------*/

static int dissect_wisun_eapol_relay(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    guint offset = 0;
    proto_item *subitem = proto_tree_add_item(tree, proto_wisun_eapol_relay, tvb, offset, 9, ENC_NA);
    proto_tree *subtree = proto_item_add_subtree(subitem, ett_wisun_eapol_relay);

    proto_tree_add_item(subtree, hf_wisun_eapol_relay_sup, tvb, offset, 8, ENC_BIG_ENDIAN);
    offset += 8;
    proto_tree_add_item(subtree, hf_wisun_eapol_relay_kmp_id, tvb, offset, 1, ENC_NA);
    offset += 1;

    int up = 0;
    // eapol.type == EAP_PACKET?
    if (tvb_get_guint8(tvb, offset+1) == 0) {
        up = tvb_get_guint8(tvb, offset+4) == 2;  // eap.code == EAP_CODE_RESPONSE
    } else {
        up = (tvb_get_guint8(tvb, offset+6) & 0x80) == 0;  // Key Info ACK==0
    }
    proto_item* diritem = proto_tree_add_boolean(subtree, hf_wisun_eapol_relay_direction, tvb, offset, 0, (guint32) up);
    PROTO_ITEM_SET_GENERATED(diritem);

    int r = call_dissector(eapol_handle, tvb_new_subset_remaining(tvb, offset), pinfo, tree);

    // UTF-8 arrow up or down
    col_append_str(pinfo->cinfo, COL_INFO, up ? " [Relay \xe2\x86\x91]" : " [Relay \xe2\x86\x93]");

    return offset + r;
}

/*-----------------------------------------------
 * Wi-SUN Netricity Segment Control
 *---------------------------------------------*/

static int dissect_wisun_netricity_sc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    static const int * fields_sc[] = {
        &hf_wisun_netricity_sc_reserved,
        &hf_wisun_netricity_sc_tone_map_request,
        &hf_wisun_netricity_sc_contention_control,
        &hf_wisun_netricity_sc_channel_access_priority,
        &hf_wisun_netricity_sc_last_segment,
        NULL
    };

    guint offset = 0;

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "Wi-SUN Netricity");

    proto_item *subitem = proto_tree_add_item(tree, proto_wisun_netricity_sc, tvb, offset, -1, ENC_NA);
    proto_tree *subtree = proto_item_add_subtree(subitem, ett_wisun_netricity_sc);

    gboolean is_last = tvb_get_guint8(tvb, 0) & 1;
    proto_tree_add_bitmask(subtree, tvb, offset++, hf_wisun_netricity_sc_flags, ett_wisun_netricity_sc_bitmask, fields_sc, ENC_BIG_ENDIAN);
    guint32 seg_count;
    guint32 seg_len;
    proto_tree_add_item_ret_uint(subtree, hf_wisun_netricity_sc_segment_count, tvb, offset, 2, ENC_BIG_ENDIAN, &seg_count);
    proto_tree_add_item_ret_uint(subtree, hf_wisun_netricity_sc_segment_length, tvb, offset, 2, ENC_BIG_ENDIAN, &seg_len);
    offset += 2;

    gboolean is_segmented = !is_last || seg_count != 0;
    proto_tree *ieee802154_tree;
    ieee802154_packet *packet;
    tvbuff_t *frame = tvb_new_subset_remaining(tvb, offset);
    // if security is used, all segments have the flag set in the FCF, but only the first has the Auxiliary Security Header
    guint mhr_len = ieee802154_dissect_header(frame, pinfo,
                                              is_segmented ? subtree : tree,
                                              seg_count == 0 ? 0 : IEEE802154_DISSECT_HEADER_OPTION_NO_AUX_SEC_HDR,
                                              &ieee802154_tree, &packet);
    if (!mhr_len) {
        return tvb_captured_length(tvb);
    }
    frame = tvb_new_subset_length(frame, 0, mhr_len + seg_len);

    if (is_segmented) {
        gboolean save_fragmented = pinfo->fragmented;
        pinfo->fragmented = TRUE;
        fragment_head *frag_msg = fragment_add_seq_check(&netricity_reassembly_table,
                                                         frame,
                                                         seg_count == 0 ? 0 : mhr_len,
                                                         pinfo,
                                                         packet->seqno,
                                                         NULL,
                                                         seg_count,
                                                         (seg_count == 0 ? mhr_len : 0) + seg_len,
                                                         !is_last);
        tvbuff_t *reframe = process_reassembled_data(tvb, offset, pinfo, "Reassembled segments", frag_msg, &netricity_scr_frag_items, NULL, subtree);

        if (reframe) {
            call_dissector(ieee802154_nofcs_handle, reframe, pinfo, tree);
            col_append_fstr(pinfo->cinfo, COL_INFO, " (Reassembled %u segments)", seg_count + 1);
        } else {
            call_data_dissector(tvb_new_subset_remaining(frame, mhr_len), pinfo, subtree);
            col_append_fstr(pinfo->cinfo, COL_INFO, " (Segment %u)", seg_count);
        }

        pinfo->fragmented = save_fragmented;
    } else {
        tvbuff_t *payload = ieee802154_decrypt_payload(frame, mhr_len, pinfo, ieee802154_tree, packet);
        if (payload) {
            guint pie_size = ieee802154_dissect_payload_ies(payload, pinfo, ieee802154_tree, packet);
            payload = tvb_new_subset_remaining(payload, pie_size);
            ieee802154_dissect_frame_payload(payload, pinfo, ieee802154_tree, packet, TRUE);
        }
    }

    return tvb_captured_length(tvb);
}

/*-----------------------------------------------
 * Wi-SUN Protocol Registration
 *---------------------------------------------*/
void proto_register_wisun(void)
{
    static hf_register_info hf[] = {
        /* Wi-SUN Header IEs */
        { &hf_wisun_subid,
          { "Header Sub ID", "wisun.subid", FT_UINT8, BASE_DEC, VALS(wisun_subid_vals), 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_unknown_ie,
          { "Unknown IE", "wisun.unknown", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_uttie,
          { "Unicast Timing IE", "wisun.uttie", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_uttie_type,
          { "Frame Type", "wisun.uttie.type", FT_UINT8, BASE_DEC, VALS(wisun_frame_type_vals), 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_uttie_ufsi,
          { "Unicast Fractional Sequence Interval", "wisun.uttie.ufsi", FT_UINT24, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_btie,
          { "Broadcast Timing IE", "wisun.btie", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_btie_slot,
          { "Broadcast Slot Number", "wisun.btie.slot", FT_UINT24, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_btie_bio,
          { "Broadcast Interval Offset", "wisun.btie.bio", FT_UINT24, BASE_DEC|BASE_UNIT_STRING, &units_milliseconds, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_fcie,
          { "Flow Control IE", "wisun.fcie", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_fcie_tx,
          { "Transmit Flow Control", "wisun.fcie.tx", FT_UINT8, BASE_DEC|BASE_UNIT_STRING, &units_milliseconds, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_fcie_rx,
          { "Receive Flow Control", "wisun.fcie.rx", FT_UINT8, BASE_DEC|BASE_UNIT_STRING, &units_milliseconds, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_fcie_src,
          { "Source Address", "wisun.fcie.src", FT_EUI64, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_fcie_initial_frame,
          { "Initial Frame", "wisun.fcie.initial_frame", FT_FRAMENUM, BASE_NONE, FRAMENUM_TYPE(FT_FRAMENUM_REQUEST), 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_rslie,
          { "Received Signal Level IE", "wisun.rslie", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_rslie_rsl,
          { "Received Signal Level", "wisun.rslie.rsl", FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_vhie,
          { "Vendor Header IE", "wisun.vhie", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_vhie_vid,
          { "Vendor ID", "wisun.vhie.vid", FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_eaie,
          { "EAPOL Authenticator IE", "wisun.eaie", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_eaie_eui,
          { "Authenticator EUI-64", "wisun.eaie.eui", FT_EUI64, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        /* Wi-SUN Payload IE */
        { &hf_wisun_pie,
          { "Wi-SUN Payload IE", "wisun.pie", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_wsie,
          { "Wi-SUN Sub IE", "wisun.wsie", FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_wsie_type,
          { "Type", "wisun.wsie.type", FT_UINT16, BASE_DEC, VALS(wisun_wsie_types), IEEE802154_PSIE_TYPE_MASK,
            NULL, HFILL }
        },

        { &hf_wisun_wsie_id,
          { "Sub ID", "wisun.wsie.id", FT_UINT16, BASE_DEC, VALS(wisun_wsie_names), IEEE802154_PSIE_ID_MASK_LONG,
            NULL, HFILL }
        },

        { &hf_wisun_wsie_length,
          { "Length", "wisun.wsie.length", FT_UINT16, BASE_DEC, NULL, IEEE802154_PSIE_LENGTH_MASK_LONG,
            NULL, HFILL }
        },

        { &hf_wisun_wsie_id_short,
          { "Sub ID", "wisun.wsie.id", FT_UINT16, BASE_DEC, VALS(wisun_wsie_names), IEEE802154_PSIE_ID_MASK_SHORT,
            NULL, HFILL }
        },

        { &hf_wisun_wsie_length_short,
          { "Length", "wisun.wsie.length", FT_UINT16, BASE_DEC, NULL, IEEE802154_PSIE_LENGTH_MASK_SHORT,
            NULL, HFILL }
        },

        { &hf_wisun_usie,
          { "Unicast Schedule IE", "wisun.usie", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_usie_dwell_interval,
          { "Dwell Interval", "wisun.usie.dwell", FT_UINT8, BASE_DEC|BASE_UNIT_STRING, &units_milliseconds, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_usie_clock_drift,
          { "Clock Drift", "wisun.usie.drift", FT_UINT8, BASE_DEC|BASE_SPECIAL_VALS, VALS(wisun_usie_clock_drift_names), 0x0,
            "Clock Drift in +/- ppm", HFILL }
        },

        { &hf_wisun_usie_timing_accuracy,
          { "Timing Accuracy", "wisun.usie.accuracy", FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_usie_channel_control,
          { "Channel Control", "wisun.usie.channel", FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_usie_channel_plan,
          { "Channel Plan", "wisun.usie.channel.plan", FT_UINT8, BASE_DEC, VALS(wisun_channel_plan_names), WISUN_CHANNEL_PLAN,
            NULL, HFILL }
        },

        { &hf_wisun_usie_channel_function,
          { "Channel Function", "wisun.usie.channel.function", FT_UINT8, BASE_DEC, VALS(wisun_channel_function_names), WISUN_CHANNEL_FUNCTION,
            NULL, HFILL }
        },

        { &hf_wisun_usie_channel_exclude,
          { "Excluded Channels", "wisun.usie.channel.exclude", FT_UINT8, BASE_DEC, VALS(wisun_channel_exclude_names), WISUN_CHANNEL_EXCLUDE,
            NULL, HFILL }
        },

        { &hf_wisun_usie_regulatory_domain,
          { "Regulatory Domain", "wisun.usie.domain", FT_UINT8, BASE_DEC, VALS(wisun_channel_regulatory_domains_names), 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_usie_operating_class,
          { "Operating Class", "wisun.usie.class", FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_usie_explicit,
          { "Explicit Channel Plan", "wisun.usie.explicit", FT_UINT32, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_usie_explicit_frequency,
          { "CH0 Frequency", "wisun.usie.explicit.frequency", FT_UINT32, BASE_DEC|BASE_UNIT_STRING, &units_khz, WISUN_CH_PLAN_EXPLICIT_FREQ,
            NULL, HFILL }
        },

        { &hf_wisun_usie_explicit_reserved,
          { "Reserved", "wisun.usie.explicit.reserved", FT_UINT32, BASE_DEC, NULL, WISUN_CH_PLAN_EXPLICIT_RESERVED,
            NULL, HFILL }
        },

        { &hf_wisun_usie_explicit_spacing,
          { "Channel Spacing", "wisun.usie.explicit.spacing", FT_UINT32, BASE_DEC, VALS(wisun_channel_spacing_names), WISUN_CH_PLAN_EXPLICIT_SPACING,
            NULL, HFILL }
        },

        { &hf_wisun_usie_number_channels,
          { "Number of Channels", "wisun.usie.num_channels", FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_usie_fixed_channel,
          { "Fixed Channel", "wisun.usie.fixed_channel", FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_usie_hop_count,
          { "Chanel Hop Count", "wisun.usie.hop_count", FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_usie_hop_list,
          { "Channel Hop List", "wisun.usie.hop_list", FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_usie_number_ranges,
          { "Number of Excluded Ranges", "wisun.usie.num_ranges", FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_usie_exclude_range,
          { "Excluded Channel Range", "wisun.usie.exclude.range", FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_usie_exclude_mask,
          { "Excluded Channel Mask", "wisun.usie.exclude.mask", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_bsie,
          { "Broadcast Schedule IE", "wisun.bsie", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_bsie_bcast_interval,
          { "Broadcast Interval", "wisun.bsie.interval", FT_UINT32, BASE_DEC|BASE_UNIT_STRING, &units_milliseconds, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_bsie_bcast_schedule_id,
          { "Broadcast Schedule ID", "wisun.bsie.schedule", FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_vpie,
          { "Vendor Payload IE", "wisun.vpie", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_vpie_vid,
          { "Vendor ID", "wisun.vpie.vid", FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_panie,
          { "PAN Information IE", "wisun.panie", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_panie_size,
          { "PAN Size", "wisun.panie.size", FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_panie_cost,
          { "Routing Cost", "wisun.panie.cost", FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_panie_flags,
          { "PAN Flags", "wisun.panie.flags", FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_panie_flag_parent_bsie,
          { "Use Parent BS-IE", "wisun.panie.flags.parent_bsie", FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },

        { &hf_wisun_panie_flag_routing_method,
          { "Routing Method", "wisun.panie.flags.routing_method", FT_UINT8, BASE_HEX, VALS(wisun_routing_methods), 0x02,
            NULL, HFILL }
        },

        { &hf_wisun_panie_flag_version,
          { "FAN TPS Version", "wisun.panie.flags.version", FT_UINT8, BASE_DEC, NULL, 0xe0,
            NULL, HFILL }
        },

        { &hf_wisun_netnameie,
          { "Network Name IE", "wisun.netnameie", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_netnameie_name,
          { "Network Name", "wisun.netnameie.name", FT_STRING, STR_ASCII, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_panverie,
          { "PAN Version IE", "wisun.panverie", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_panverie_version,
          { "PAN Version", "wisun.panverie.version", FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_gtkhashie,
          { "GTK Hash IE", "wisun.gtkhashie", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_gtkhashie_gtk0,
          { "GTK0 Hash", "wisun.gtkhashie.gtk0", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_gtkhashie_gtk1,
          { "GTK1 Hash", "wisun.gtkhashie.gtk1", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_gtkhashie_gtk2,
          { "GTK2 Hash", "wisun.gtkhashie.gtk2", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_gtkhashie_gtk3,
          { "GTK3 Hash", "wisun.gtkhashie.gtk3", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        /* Wi-SUN FAN Security Extension */
        { &hf_wisun_sec_function,
          { "Function Code", "wisun.sec.function", FT_UINT8, BASE_HEX, VALS(wisun_sec_functions), 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_sec_error_type,
          { "Error Type", "wisun.sec.error", FT_UINT8, BASE_HEX, VALS(wisun_sec_sm_errors), 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_sec_error_nonce,
          { "Initiator Nonce", "wisun.sec.nonce", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        /* EAPOL Relay */
        { &hf_wisun_eapol_relay_sup,
          { "SUP EUI-64", "wisun.eapol_relay.sup", FT_EUI64, BASE_NONE, NULL, 0x0,
          NULL, HFILL }},

        { &hf_wisun_eapol_relay_kmp_id,
          { "KMP ID", "wisun.eapol_relay.kmp_id", FT_UINT8, BASE_DEC, VALS(ieee802154_mpx_kmp_id_vals), 0x0,
          NULL, HFILL }},

        { &hf_wisun_eapol_relay_direction,
          { "Direction", "wisun.eapol_relay.direction", FT_BOOLEAN, BASE_NONE, TFS(&tfs_up_down), 0x0,
          NULL, HFILL }},

        /* Wi-SUN Netricity */
        { &hf_wisun_netricity_nftie,
          { "Netricity Frame Type IE", "wisun.netricity.nftie", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_netricity_nftie_type,
          { "Frame Type", "wisun.netricity.nftie.type", FT_UINT8, BASE_DEC, VALS(wisun_frame_type_vals), 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_netricity_lqiie,
          { "Link Quality Index IE", "wisun.netricity.lqiie", FT_NONE, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_wisun_netricity_lqiie_lqi,
          { "Link Quality Index", "wisun.netricity.lqiie.lqi", FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        /* Wi-SUN Netricity Segment Control Field and reassembly */
        { &hf_wisun_netricity_sc_flags,
          { "Segment Control", "wisun.netricity.sc.flags", FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_wisun_netricity_sc_reserved,
          { "Reserved", "wisun.netricity.sc.reserved", FT_UINT8, BASE_DEC, NULL, 0xf0,
            NULL, HFILL }
        },
        { &hf_wisun_netricity_sc_tone_map_request,
          { "Tone Map Request", "wisun.netricity.sc.tone_map_request", FT_BOOLEAN, 8, NULL, 1<<3,
            NULL, HFILL }
        },
        { &hf_wisun_netricity_sc_contention_control,
          { "Contention Control", "wisun.netricity.sc.contention_control", FT_BOOLEAN, 8, TFS(&wisun_netricity_sc_contention_control_tfs), 1<<2,
            NULL, HFILL }
        },
        { &hf_wisun_netricity_sc_channel_access_priority,
          { "Channel access priority", "wisun.netricity.sc.channel_access_priority", FT_BOOLEAN, 8, TFS(&tfs_high_normal), 1<<1,
            NULL, HFILL }
        },
        { &hf_wisun_netricity_sc_last_segment,
          { "Last Segment", "wisun.netricity.sc.last_segment", FT_BOOLEAN, 8, NULL, 1<<0,
            NULL, HFILL }
        },
        { &hf_wisun_netricity_sc_segment_count,
          { "Segment Count", "wisun.netricity.sc.segment_count", FT_UINT16, BASE_DEC, NULL, 0xfc00,
            NULL, HFILL }
        },
        { &hf_wisun_netricity_sc_segment_length,
          { "Segment Length", "wisun.netricity.sc.segment_length", FT_UINT16, BASE_DEC, NULL, 0x03ff,
            NULL, HFILL }
        },

        { &hf_wisun_netricity_scr_segments,
          { "Message segments", "wisun.netricity.scr.segments",
            FT_NONE, BASE_NONE, NULL, 0x00, NULL, HFILL }
        },
        { &hf_wisun_netricity_scr_segment,
          { "Message segment", "wisun.netricity.scr.segment",
            FT_FRAMENUM, BASE_NONE, NULL, 0x00, NULL, HFILL }
        },
        { &hf_wisun_netricity_scr_segment_overlap,
          { "Message segment overlap", "wisun.netricity.scr.segment.overlap",
            FT_BOOLEAN, 0, NULL, 0x00, NULL, HFILL }
        },
        { &hf_wisun_netricity_scr_segment_overlap_conflicts,
          { "Message segment overlapping with conflicting data", "wisun.netricity.scr.segment.overlap.conflicts",
            FT_BOOLEAN, 0, NULL, 0x00, NULL, HFILL }
        },
        { &hf_wisun_netricity_scr_segment_multiple_tails,
          { "Message has multiple tail segments", "wisun.netricity.scr.segment.multiple_tails",
            FT_BOOLEAN, 0, NULL, 0x00, NULL, HFILL }
        },
        { &hf_wisun_netricity_scr_segment_too_long_segment,
          { "Message segment too long", "wisun.netricity.scr.segment.too_long_segment",
            FT_BOOLEAN, 0, NULL, 0x00, NULL, HFILL }
        },
        { &hf_wisun_netricity_scr_segment_error,
          { "Message segment reassembly error", "wisun.netricity.scr.segment.error",
            FT_FRAMENUM, BASE_NONE, NULL, 0x00, NULL, HFILL }
        },
        { &hf_wisun_netricity_scr_segment_count,
          { "Message segment count", "wisun.netricity.scr.segment.count",
            FT_UINT32, BASE_DEC, NULL, 0x00, NULL, HFILL }
        },
        { &hf_wisun_netricity_scr_reassembled_in,
          { "Reassembled in", "wisun.netricity.scr.reassembled.in",
            FT_FRAMENUM, BASE_NONE, NULL, 0x00, NULL, HFILL }
        },
        { &hf_wisun_netricity_scr_reassembled_length,
          { "Reassembled length", "wisun.netricity.scr.reassembled.length",
            FT_UINT32, BASE_DEC, NULL, 0x00, NULL, HFILL }
        },


    };

    /* Subtrees */
    static gint *ett[] = {
        &ett_wisun_unknown_ie,
        &ett_wisun_uttie,
        &ett_wisun_btie,
        &ett_wisun_fcie,
        &ett_wisun_rslie,
        &ett_wisun_vhie,
        &ett_wisun_eaie,
        &ett_wisun_pie,
        &ett_wisun_wsie_bitmap,
        &ett_wisun_usie,
        &ett_wisun_bsie,
        &ett_wisun_vpie,
        &ett_wisun_panie,
        &ett_wisun_panie_flags,
        &ett_wisun_netnameie,
        &ett_wisun_panverie,
        &ett_wisun_gtkhashie,
        &ett_wisun_sec,
        &ett_wisun_eapol_relay,
        &ett_wisun_netricity_nftie,
        &ett_wisun_netricity_lqiie,
        &ett_wisun_netricity_sc,
        &ett_wisun_netricity_sc_bitmask,
        &ett_wisun_netricity_scr_segment,
        &ett_wisun_netricity_scr_segments,
    };

    static ei_register_info ei[] = {
        { &ei_wisun_subid_unsupported, { "wisun.subid.unsupported", PI_PROTOCOL, PI_WARN,
                "Unsupported Header Sub ID", EXPFILL }},
        { &ei_wisun_usie_channel_plan_invalid, { "wisun.usie.channel.plan.invalid", PI_PROTOCOL, PI_WARN,
                "Invalid Channel Plan", EXPFILL }},
        { &ei_wisun_wsie_unsupported, { "wisun.wsie.unsupported", PI_PROTOCOL, PI_WARN,
                "Unsupported Sub-IE ID", EXPFILL }},
        { &ei_wisun_edfe_start_not_found, { "wisun.edfe.start_not_found", PI_SEQUENCE, PI_WARN,
                "EDFE Transfer: start frame not found", EXPFILL }},
        { &ei_wisun_usie_explicit_reserved_bits_not_zero, { "wisun.usie.explicit.reserved.invalid", PI_MALFORMED, PI_ERROR,
                "Reserved bits not zero", EXPFILL }},
    };

    expert_module_t* expert_wisun;

    proto_wisun = proto_register_protocol("Wi-SUN Field Area Network", "Wi-SUN", "wisun");
    proto_wisun_sec = proto_register_protocol("Wi-SUN FAN Security Extension", "Wi-SUN WM-SEC", "wisun.sec");
    proto_wisun_eapol_relay = proto_register_protocol("Wi-SUN FAN EAPOL Relay", "Wi-SUN EAPOL Relay", "wisun.eapol_relay");
    proto_wisun_netricity_sc = proto_register_protocol("Wi-SUN Netricity Segment", "Wi-SUN Netricity Segment", "wisun.netricity.sc");

    proto_register_field_array(proto_wisun, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    expert_wisun = expert_register_protocol(proto_wisun);
    expert_register_field_array(expert_wisun, ei, array_length(ei));

    register_dissector("wisun.sec", dissect_wisun_sec, proto_wisun_sec);

    edfe_byaddr = wmem_map_new_autoreset(wmem_epan_scope(), wmem_file_scope(), g_int64_hash, g_int64_equal);

    wisun_eapol_relay_handle = register_dissector("wisun.eapol_relay", dissect_wisun_eapol_relay, proto_wisun_eapol_relay);

    register_dissector("wisun.netricity.sc", dissect_wisun_netricity_sc, proto_wisun_netricity_sc);
    reassembly_table_register(&netricity_reassembly_table, &addresses_reassembly_table_functions);
}

void proto_reg_handoff_wisun(void)
{
    dissector_add_uint(IEEE802154_HEADER_IE_DTABLE, IEEE802154_HEADER_IE_WISUN, create_dissector_handle(dissect_wisun_hie, proto_wisun));
    dissector_add_uint(IEEE802154_PAYLOAD_IE_DTABLE, IEEE802154_PAYLOAD_IE_WISUN, create_dissector_handle(dissect_wisun_pie, proto_wisun));

    // For EAPOL relay
    dissector_add_uint("udp.port", WISUN_EAPOL_RELAY_UDP_PORT, wisun_eapol_relay_handle);
    eapol_handle = find_dissector("eapol");

    // For Netricity reassembly
    ieee802154_nofcs_handle = find_dissector("wpan_nofcs");
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
