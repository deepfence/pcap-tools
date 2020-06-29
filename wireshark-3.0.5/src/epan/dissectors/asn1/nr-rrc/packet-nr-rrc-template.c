/* packet-nr-rrc-template.c
 * NR;
 * Radio Resource Control (RRC) protocol specification
 * (3GPP TS 38.331 V15.4.0 Release 15) packet dissection
 * Copyright 2018-2019, Pascal Quantin
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <stdlib.h>

#include <epan/packet.h>
#include <epan/asn1.h>
#include <epan/expert.h>
#include <epan/reassemble.h>
#include <epan/exceptions.h>
#include <epan/show_exception.h>

#include <wsutil/str_util.h>

#include "packet-per.h"
#include "packet-gsm_map.h"
#include "packet-cell_broadcast.h"
#include "packet-lte-rrc.h"
#include "packet-nr-rrc.h"

#define PNAME  "NR Radio Resource Control (RRC) protocol"
#define PSNAME "NR RRC"
#define PFNAME "nr-rrc"

void proto_register_nr_rrc(void);
void proto_reg_handoff_nr_rrc(void);

static dissector_handle_t nas_5gs_handle = NULL;
static dissector_handle_t lte_rrc_conn_reconf_handle = NULL;

static wmem_map_t *nr_rrc_etws_cmas_dcs_hash = NULL;

static reassembly_table nr_rrc_sib7_reassembly_table;
static reassembly_table nr_rrc_sib8_reassembly_table;

/* Include constants */
#include "packet-nr-rrc-val.h"

/* Initialize the protocol and registered fields */
static int proto_nr_rrc = -1;
#include "packet-nr-rrc-hf.c"
static int hf_nr_rrc_serialNumber_gs = -1;
static int hf_nr_rrc_serialNumber_msg_code = -1;
static int hf_nr_rrc_serialNumber_upd_nb = -1;
static int hf_nr_rrc_warningType_value = -1;
static int hf_nr_rrc_warningType_emergency_user_alert = -1;
static int hf_nr_rrc_warningType_popup = -1;
static int hf_nr_rrc_warningMessageSegment_nb_pages = -1;
static int hf_nr_rrc_warningMessageSegment_decoded_page = -1;
static int hf_nr_rrc_sib7_fragments = -1;
static int hf_nr_rrc_sib7_fragment = -1;
static int hf_nr_rrc_sib7_fragment_overlap = -1;
static int hf_nr_rrc_sib7_fragment_overlap_conflict = -1;
static int hf_nr_rrc_sib7_fragment_multiple_tails = -1;
static int hf_nr_rrc_sib7_fragment_too_long_fragment = -1;
static int hf_nr_rrc_sib7_fragment_error = -1;
static int hf_nr_rrc_sib7_fragment_count = -1;
static int hf_nr_rrc_sib7_reassembled_in = -1;
static int hf_nr_rrc_sib7_reassembled_length = -1;
static int hf_nr_rrc_sib7_reassembled_data = -1;
static int hf_nr_rrc_sib8_fragments = -1;
static int hf_nr_rrc_sib8_fragment = -1;
static int hf_nr_rrc_sib8_fragment_overlap = -1;
static int hf_nr_rrc_sib8_fragment_overlap_conflict = -1;
static int hf_nr_rrc_sib8_fragment_multiple_tails = -1;
static int hf_nr_rrc_sib8_fragment_too_long_fragment = -1;
static int hf_nr_rrc_sib8_fragment_error = -1;
static int hf_nr_rrc_sib8_fragment_count = -1;
static int hf_nr_rrc_sib8_reassembled_in = -1;
static int hf_nr_rrc_sib8_reassembled_length = -1;
static int hf_nr_rrc_sib8_reassembled_data = -1;
static int hf_nr_rrc_utc_time = -1;
static int hf_nr_rrc_local_time = -1;

/* Initialize the subtree pointers */
static gint ett_nr_rrc = -1;
#include "packet-nr-rrc-ett.c"
static gint ett_nr_rrc_DedicatedNAS_Message = -1;
static gint ett_rr_rrc_targetRAT_MessageContainer = -1;
static gint ett_nr_rrc_nas_Container = -1;
static gint ett_nr_rrc_serialNumber = -1;
static gint ett_nr_rrc_warningType = -1;
static gint ett_nr_rrc_dataCodingScheme = -1;
static gint ett_nr_rrc_sib7_fragment = -1;
static gint ett_nr_rrc_sib7_fragments = -1;
static gint ett_nr_rrc_sib8_fragment = -1;
static gint ett_nr_rrc_sib8_fragments = -1;
static gint ett_nr_rrc_warningMessageSegment = -1;
static gint ett_nr_rrc_timeInfo = -1;
static gint ett_nr_rrc_capabilityRequestFilter = -1;

static expert_field ei_nr_rrc_number_pages_le15 = EI_INIT;

static const unit_name_string units_periodicities = { " periodicity", " periodicities" };
static const unit_name_string units_prbs = { " PRB", " PRBs" };
static const unit_name_string units_slots = { " slot", " slots" };

typedef struct {
  guint8 rat_type;
  guint8 target_rat_type;
  guint16 message_identifier;
  guint8 warning_message_segment_type;
  guint8 warning_message_segment_number;
} nr_rrc_private_data_t;

/* Helper function to get or create a struct that will be actx->private_data */
static nr_rrc_private_data_t*
nr_rrc_get_private_data(asn1_ctx_t *actx)
{
  if (actx->private_data == NULL) {
    actx->private_data = wmem_new0(wmem_packet_scope(), nr_rrc_private_data_t);
  }
  return (nr_rrc_private_data_t*)actx->private_data;
}

static void
nr_rrc_call_dissector(dissector_handle_t handle, tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  TRY {
    call_dissector(handle, tvb, pinfo, tree);
  }
  CATCH_BOUNDS_ERRORS {
    show_exception(tvb, pinfo, tree, EXCEPT_CODE, GET_MESSAGE);
  }
  ENDTRY;
}

/* Forward declarations */
static int dissect_UE_CapabilityRequestFilterNR_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);

static void
nr_rrc_q_RxLevMin_fmt(gchar *s, guint32 v)
{
  g_snprintf(s, ITEM_LABEL_LENGTH, "%u dB (%u)", 2*v, v);
}

static const value_string nr_rrc_serialNumber_gs_vals[] = {
  { 0, "Display mode immediate, cell wide"},
  { 1, "Display mode normal, PLMN wide"},
  { 2, "Display mode normal, tracking area wide"},
  { 3, "Display mode normal, cell wide"},
  { 0, NULL},
};

static const value_string nr_rrc_warningType_vals[] = {
  { 0, "Earthquake"},
  { 1, "Tsunami"},
  { 2, "Earthquake and Tsunami"},
  { 3, "Test"},
  { 4, "Other"},
  { 0, NULL},
};

static const fragment_items nr_rrc_sib7_frag_items = {
    &ett_nr_rrc_sib7_fragment,
    &ett_nr_rrc_sib7_fragments,
    &hf_nr_rrc_sib7_fragments,
    &hf_nr_rrc_sib7_fragment,
    &hf_nr_rrc_sib7_fragment_overlap,
    &hf_nr_rrc_sib7_fragment_overlap_conflict,
    &hf_nr_rrc_sib7_fragment_multiple_tails,
    &hf_nr_rrc_sib7_fragment_too_long_fragment,
    &hf_nr_rrc_sib7_fragment_error,
    &hf_nr_rrc_sib7_fragment_count,
    &hf_nr_rrc_sib7_reassembled_in,
    &hf_nr_rrc_sib7_reassembled_length,
    &hf_nr_rrc_sib7_reassembled_data,
    "SIB7 warning message segments"
};

static const fragment_items nr_rrc_sib8_frag_items = {
    &ett_nr_rrc_sib8_fragment,
    &ett_nr_rrc_sib8_fragments,
    &hf_nr_rrc_sib8_fragments,
    &hf_nr_rrc_sib8_fragment,
    &hf_nr_rrc_sib8_fragment_overlap,
    &hf_nr_rrc_sib8_fragment_overlap_conflict,
    &hf_nr_rrc_sib8_fragment_multiple_tails,
    &hf_nr_rrc_sib8_fragment_too_long_fragment,
    &hf_nr_rrc_sib8_fragment_error,
    &hf_nr_rrc_sib8_fragment_count,
    &hf_nr_rrc_sib8_reassembled_in,
    &hf_nr_rrc_sib8_reassembled_length,
    &hf_nr_rrc_sib8_reassembled_data,
    "SIB8 warning message segments"
};

static void
dissect_nr_rrc_warningMessageSegment(tvbuff_t *warning_msg_seg_tvb, proto_tree *tree, packet_info *pinfo, guint8 dataCodingScheme)
{
  guint32 offset;
  guint8 nb_of_pages, length, *str;
  proto_item *ti;
  tvbuff_t *cb_data_page_tvb, *cb_data_tvb;
  int i;

  nb_of_pages = tvb_get_guint8(warning_msg_seg_tvb, 0);
  ti = proto_tree_add_uint(tree, hf_nr_rrc_warningMessageSegment_nb_pages, warning_msg_seg_tvb, 0, 1, nb_of_pages);
  if (nb_of_pages > 15) {
    expert_add_info_format(pinfo, ti, &ei_nr_rrc_number_pages_le15,
                           "Number of pages should be <=15 (found %u)", nb_of_pages);
    nb_of_pages = 15;
  }
  for (i = 0, offset = 1; i < nb_of_pages; i++) {
    length = tvb_get_guint8(warning_msg_seg_tvb, offset+82);
    cb_data_page_tvb = tvb_new_subset_length(warning_msg_seg_tvb, offset, length);
    cb_data_tvb = dissect_cbs_data(dataCodingScheme, cb_data_page_tvb, tree, pinfo, 0);
    if (cb_data_tvb) {
      str = tvb_get_string_enc(wmem_packet_scope(), cb_data_tvb, 0, tvb_reported_length(cb_data_tvb), ENC_UTF_8|ENC_NA);
      proto_tree_add_string_format(tree, hf_nr_rrc_warningMessageSegment_decoded_page, warning_msg_seg_tvb, offset, 83,
                                   str, "Decoded Page %u: %s", i+1, str);
    }
    offset += 83;
  }
}

static const value_string nr_rrc_daylightSavingTime_vals[] = {
  { 0, "No adjustment for Daylight Saving Time"},
  { 1, "+1 hour adjustment for Daylight Saving Time"},
  { 2, "+2 hours adjustment for Daylight Saving Time"},
  { 3, "Reserved"},
  { 0, NULL},
};

static void
nr_rrc_localTimeOffset_fmt(gchar *s, guint32 v)
{
  gint32 time_offset = (gint32) v;

  g_snprintf(s, ITEM_LABEL_LENGTH, "UTC time %c %dhr %dmin (%d)",
             (time_offset < 0) ? '-':'+', abs(time_offset) >> 2,
             (abs(time_offset) & 0x03) * 15, time_offset);
}

static void
nr_rrc_drx_SlotOffset_fmt(gchar *s, guint32 v)
{
  g_snprintf(s, ITEM_LABEL_LENGTH, "%g ms (%u)", 1./32 * v, v);
}

static void
nr_rrc_Hysteresis_fmt(gchar *s, guint32 v)
{
  g_snprintf(s, ITEM_LABEL_LENGTH, "%gdB (%u)", 0.5 * v, v);
}

static void
nr_rrc_msg3_DeltaPreamble_fmt(gchar *s, guint32 v)
{
  gint32 d = (gint32)v;

  g_snprintf(s, ITEM_LABEL_LENGTH, "%ddB (%d)", 2 * d, d);
}

static void
nr_rrc_Q_RxLevMin_fmt(gchar *s, guint32 v)
{
  gint32 d = (gint32)v;

  g_snprintf(s, ITEM_LABEL_LENGTH, "%ddBm (%d)", 2 * d, d);
}

static void
nr_rrc_RSRP_RangeEUTRA_fmt(gchar *s, guint32 v)
{
  if (v == 0) {
    g_snprintf(s, ITEM_LABEL_LENGTH, "RSRP < -140dBm (0)");
  } else if (v < 97) {
    g_snprintf(s, ITEM_LABEL_LENGTH, "%ddBm <= RSRP < %ddBm (%u)", v-141, v-140, v);
  } else {
    g_snprintf(s, ITEM_LABEL_LENGTH, "-44dBm <= RSRP (97)");
  }
}

static void
nr_rrc_RSRQ_RangeEUTRA_fmt(gchar *s, guint32 v)
{
  if (v == 0) {
    g_snprintf(s, ITEM_LABEL_LENGTH, "RSRQ < -19.5dB (0)");
  } else if (v < 34) {
    g_snprintf(s, ITEM_LABEL_LENGTH, "%.1fdB <= RSRQ < %.1fdB (%u)", ((float)v/2)-20, (((float)v+1)/2)-20, v);
  } else {
    g_snprintf(s, ITEM_LABEL_LENGTH, "-3dB <= RSRQ (34)");
  }
}

static void
nr_rrc_SINR_RangeEUTRA_fmt(gchar *s, guint32 v)
{
  if (v == 0) {
    g_snprintf(s, ITEM_LABEL_LENGTH, "SINR < -23dB (0)");
  } else if (v == 127) {
    g_snprintf(s, ITEM_LABEL_LENGTH, "40dB <= SINR (127)");
  } else {
    g_snprintf(s, ITEM_LABEL_LENGTH, "%.1fdB <= SINR < %.1fdB (%u)", (((float)v-1)/2)-23, ((float)v/2)-23, v);
  }
}

static void
nr_rrc_ReselectionThreshold_fmt(gchar *s, guint32 v)
{
  g_snprintf(s, ITEM_LABEL_LENGTH, "%udB (%u)", 2 * v, v);
}

static void
nr_rrc_RSRP_Range_fmt(gchar *s, guint32 v)
{
  if (v == 0) {
    g_snprintf(s, ITEM_LABEL_LENGTH, "SS-RSRP < -156dBm (0)");
  } else if (v < 126) {
    g_snprintf(s, ITEM_LABEL_LENGTH, "%ddBm <= SS-RSRP < %ddBm (%u)", v-157, v-156, v);
  } else if (v == 126) {
    g_snprintf(s, ITEM_LABEL_LENGTH, "-31dBm <= SS-RSRP (126)");
  } else {
    g_snprintf(s, ITEM_LABEL_LENGTH, "infinity (127)");
  }
}

static void
nr_rrc_RSRQ_Range_fmt(gchar *s, guint32 v)
{
  if (v == 0) {
    g_snprintf(s, ITEM_LABEL_LENGTH, "SS-RSRQ < -43dB (0)");
  } else if (v < 127) {
    g_snprintf(s, ITEM_LABEL_LENGTH, "%.1fdB <= SS-RSRQ < %.1fdB (%u)", (((float)v-1)/2)-43, ((float)v/2)-43, v);
  } else {
    g_snprintf(s, ITEM_LABEL_LENGTH, "-20dB <= SS-RSRQ (127)");
  }
}

static void
nr_rrc_SINR_Range_fmt(gchar *s, guint32 v)
{
  if (v == 0) {
    g_snprintf(s, ITEM_LABEL_LENGTH, "SS-SINR < -23dB (0)");
  } else if (v < 127) {
    g_snprintf(s, ITEM_LABEL_LENGTH, "%.1fdB <= SS-SINR < %.1fdB (%u)", (((float)v-1)/2)-23, ((float)v/2)-23, v);
  } else {
    g_snprintf(s, ITEM_LABEL_LENGTH, "40dB <= SS-SINR (127)");
  }
}

#include "packet-nr-rrc-fn.c"

void
proto_register_nr_rrc(void) {

  /* List of fields */
  static hf_register_info hf[] = {

#include "packet-nr-rrc-hfarr.c"

    { &hf_nr_rrc_serialNumber_gs,
      { "Geographical Scope", "nr-rrc.serialNumber.gs",
        FT_UINT16, BASE_DEC, VALS(nr_rrc_serialNumber_gs_vals), 0xc000,
        NULL, HFILL }},
    { &hf_nr_rrc_serialNumber_msg_code,
      { "Message Code", "nr-rrc.serialNumber.msg_code",
        FT_UINT16, BASE_DEC, NULL, 0x3ff0,
        NULL, HFILL }},
    { &hf_nr_rrc_serialNumber_upd_nb,
      { "Update Number", "nr-rrc.serialNumber.upd_nb",
        FT_UINT16, BASE_DEC, NULL, 0x000f,
        NULL, HFILL }},
    { &hf_nr_rrc_warningType_value,
      { "Warning Type Value", "nr-rrc.warningType.value",
        FT_UINT16, BASE_DEC, VALS(nr_rrc_warningType_vals), 0xfe00,
        NULL, HFILL }},
    { &hf_nr_rrc_warningType_emergency_user_alert,
      { "Emergency User Alert", "nr-rrc.warningType.emergency_user_alert",
        FT_BOOLEAN, 16, TFS(&tfs_yes_no), 0x0100,
        NULL, HFILL }},
    { &hf_nr_rrc_warningType_popup,
      { "Popup", "nr-rrc.warningType.popup",
        FT_BOOLEAN, 16, TFS(&tfs_yes_no), 0x0080,
        NULL, HFILL }},
    { &hf_nr_rrc_warningMessageSegment_nb_pages,
      { "Number of Pages", "nr-rrc.warningMessageSegment.nb_pages",
        FT_UINT8, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_warningMessageSegment_decoded_page,
      { "Decoded Page", "nr-rrc.warningMessageSegment.decoded_page",
        FT_STRING, STR_UNICODE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib7_fragments,
      { "Fragments", "nr-rrc.warningMessageSegment.fragments",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib7_fragment,
      { "Fragment", "nr-rrc.warningMessageSegment.fragment",
         FT_FRAMENUM, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib7_fragment_overlap,
      { "Fragment Overlap", "nr-rrc.warningMessageSegment.fragment_overlap",
         FT_BOOLEAN, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib7_fragment_overlap_conflict,
      { "Fragment Overlap Conflict", "nr-rrc.warningMessageSegment.fragment_overlap_conflict",
         FT_BOOLEAN, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib7_fragment_multiple_tails,
      { "Fragment Multiple Tails", "nr-rrc.warningMessageSegment.fragment_multiple_tails",
         FT_BOOLEAN, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib7_fragment_too_long_fragment,
      { "Too Long Fragment", "nr-rrc.warningMessageSegment.fragment_too_long_fragment",
         FT_BOOLEAN, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib7_fragment_error,
      { "Fragment Error", "nr-rrc.warningMessageSegment.fragment_error",
         FT_FRAMENUM, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib7_fragment_count,
      { "Fragment Count", "nr-rrc.warningMessageSegment.fragment_count",
         FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib7_reassembled_in,
      { "Reassembled In", "nr-rrc.warningMessageSegment.reassembled_in",
         FT_FRAMENUM, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib7_reassembled_length,
      { "Reassembled Length", "nr-rrc.warningMessageSegment.reassembled_length",
         FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib7_reassembled_data,
      { "Reassembled Data", "nr-rrc.warningMessageSegment.reassembled_data",
         FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib8_fragments,
      { "Fragments", "nr-rrc.warningMessageSegment.fragments",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib8_fragment,
      { "Fragment", "nr-rrc.warningMessageSegment.fragment",
         FT_FRAMENUM, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib8_fragment_overlap,
      { "Fragment Overlap", "nr-rrc.warningMessageSegment.fragment_overlap",
         FT_BOOLEAN, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib8_fragment_overlap_conflict,
      { "Fragment Overlap Conflict", "nr-rrc.warningMessageSegment.fragment_overlap_conflict",
         FT_BOOLEAN, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib8_fragment_multiple_tails,
      { "Fragment Multiple Tails", "nr-rrc.warningMessageSegment.fragment_multiple_tails",
         FT_BOOLEAN, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib8_fragment_too_long_fragment,
      { "Too Long Fragment", "nr-rrc.warningMessageSegment.fragment_too_long_fragment",
         FT_BOOLEAN, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib8_fragment_error,
      { "Fragment Error", "nr-rrc.warningMessageSegment.fragment_error",
         FT_FRAMENUM, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib8_fragment_count,
      { "Fragment Count", "nr-rrc.warningMessageSegment.fragment_count",
         FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib8_reassembled_in,
      { "Reassembled In", "nr-rrc.warningMessageSegment.reassembled_in",
         FT_FRAMENUM, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib8_reassembled_length,
      { "Reassembled Length", "nr-rrc.warningMessageSegment.reassembled_length",
         FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_sib8_reassembled_data,
      { "Reassembled Data", "nr-rrc.warningMessageSegment.reassembled_data",
         FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_nr_rrc_utc_time,
      { "UTC   time", "nr-rrc.utc_time",
        FT_ABSOLUTE_TIME, ABSOLUTE_TIME_UTC, NULL, 0x0,
        NULL, HFILL }},
    { &hf_nr_rrc_local_time,
      { "Local time", "nr-rrc.local_time",
        FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL, NULL, 0x0,
        NULL, HFILL }},
  };

  static gint *ett[] = {
    &ett_nr_rrc,
#include "packet-nr-rrc-ettarr.c"
    &ett_nr_rrc_DedicatedNAS_Message,
    &ett_rr_rrc_targetRAT_MessageContainer,
    &ett_nr_rrc_nas_Container,
    &ett_nr_rrc_serialNumber,
    &ett_nr_rrc_warningType,
    &ett_nr_rrc_dataCodingScheme,
    &ett_nr_rrc_sib7_fragment,
    &ett_nr_rrc_sib7_fragments,
    &ett_nr_rrc_sib8_fragment,
    &ett_nr_rrc_sib8_fragments,
    &ett_nr_rrc_warningMessageSegment,
    &ett_nr_rrc_timeInfo,
    &ett_nr_rrc_capabilityRequestFilter
  };

  static ei_register_info ei[] = {
     { &ei_nr_rrc_number_pages_le15, { "nr-rrc.number_pages_le15", PI_MALFORMED, PI_ERROR, "Number of pages should be <=15", EXPFILL }},
  };

  expert_module_t* expert_nr_rrc;

  /* Register protocol */
  proto_nr_rrc = proto_register_protocol(PNAME, PSNAME, PFNAME);

  /* Register fields and subtrees */
  proto_register_field_array(proto_nr_rrc, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
  expert_nr_rrc = expert_register_protocol(proto_nr_rrc);
  expert_register_field_array(expert_nr_rrc, ei, array_length(ei));

  /* Register the dissectors defined in nr-rrc.cnf */
#include "packet-nr-rrc-dis-reg.c"

  nr_rrc_etws_cmas_dcs_hash = wmem_map_new_autoreset(wmem_epan_scope(), wmem_file_scope(),
                                                     g_direct_hash, g_direct_equal);

  reassembly_table_register(&nr_rrc_sib7_reassembly_table,
                            &addresses_reassembly_table_functions);
  reassembly_table_register(&nr_rrc_sib8_reassembly_table,
                            &addresses_reassembly_table_functions);
}

void
proto_reg_handoff_nr_rrc(void)
{
  nas_5gs_handle = find_dissector("nas-5gs");
  lte_rrc_conn_reconf_handle = find_dissector("lte-rrc.rrc_conn_reconf");
}
