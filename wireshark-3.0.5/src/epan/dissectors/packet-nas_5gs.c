/* packet-nas_5gs.c
 * Routines for Non-Access-Stratum (NAS) protocol for Evolved Packet System (EPS) dissection
 *
 * Copyright 2018-2019, Anders Broman <anders.broman@ericsson.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * References: 3GPP TS 24.501 15.1.0
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/expert.h>
#include <epan/proto_data.h>

#include <wsutil/pow2.h>

#include "packet-gsm_a_common.h"
#include "packet-e212.h"

void proto_register_nas_5gs(void);
void proto_reg_handoff_nas_5gs(void);

static gboolean g_nas_5gs_null_decipher = FALSE;

static int dissect_nas_5gs_common(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, void* data);
static int dissect_nas_5gs(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data);
static guint16 de_nas_5gs_cmn_dnn(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len, gchar *add_string _U_, int string_len _U_);

static dissector_handle_t nas_5gs_handle = NULL;
static dissector_handle_t eap_handle = NULL;
static dissector_handle_t nas_eps_handle = NULL;
static dissector_handle_t nas_eps_plain_handle = NULL;

#define PNAME  "Non-Access-Stratum 5GS (NAS)PDU"
#define PSNAME "NAS-5GS"
#define PFNAME "nas-5gs"

static int proto_nas_5gs = -1;

int hf_nas_5gs_common_elem_id = -1;
int hf_nas_5gs_mm_elem_id = -1;
int hf_nas_5gs_sm_elem_id = -1;

static int hf_nas_5gs_epd = -1;
static int hf_nas_5gs_spare_b7 = -1;
static int hf_nas_5gs_spare_b6 = -1;
static int hf_nas_5gs_spare_b5 = -1;
static int hf_nas_5gs_spare_b4 = -1;
static int hf_nas_5gs_spare_b3 = -1;
static int hf_nas_5gs_spare_b2 = -1;
static int hf_nas_5gs_spare_b1 = -1;
static int hf_nas_5gs_rfu_b2;
static int hf_nas_5gs_rfu_b1;
static int hf_nas_5gs_rfu_b0;
static int hf_nas_5gs_security_header_type = -1;
static int hf_nas_5gs_msg_auth_code = -1;
static int hf_nas_5gs_seq_no = -1;
static int hf_nas_5gs_mm_msg_type = -1;
static int hf_nas_5gs_sm_msg_type = -1;
static int hf_nas_5gs_proc_trans_id = -1;
static int hf_nas_5gs_spare_half_octet = -1;
static int hf_nas_5gs_pdu_session_id = -1;
static int hf_nas_5gs_msg_elems = -1;
static int hf_nas_5gs_mm_for = -1;
static int hf_nas_5gs_cmn_dnn = -1;
static int hf_nas_5gs_mm_sms_requested = -1;
static int hf_nas_5gs_mm_5gs_reg_type = -1;
static int hf_nas_5gs_mm_tsc = -1;
static int hf_nas_5gs_mm_nas_key_set_id = -1;
static int hf_nas_5gs_mm_5gmm_cause = -1;
static int hf_nas_5gs_mm_pld_cont_type = -1;
static int hf_nas_5gs_mm_sst = -1;
static int hf_nas_5gs_mm_sd = -1;
static int hf_nas_5gs_mm_mapped_conf_sst = -1;
static int hf_nas_5gs_mm_mapped_conf_ssd = -1;
static int hf_nas_5gs_mm_switch_off = -1;
static int hf_nas_5gs_mm_re_reg_req = -1;
static int hf_nas_5gs_mm_acc_type = -1;
static int hf_nas_5gs_mm_raai_b0 = -1;
static int hf_nas_5gs_mm_conf_upd_ind_ack_b0 = -1;
static int hf_nas_5gs_mm_conf_upd_ind_red_b1 = -1;
static int hf_nas_5gs_mm_nas_sec_algo_enc = -1;
static int hf_nas_5gs_mm_nas_sec_algo_ip = -1;
static int hf_nas_5gs_mm_s1_mode_b0 = -1;
static int hf_nas_5gs_mm_ho_attach_b1 = -1;
static int hf_nas_5gs_mm_lpp_cap_b2 = -1;
static int hf_nas_5gs_mm_type_id = -1;
static int hf_nas_5gs_mm_odd_even = -1;
static int hf_nas_5gs_mm_length = -1;
static int hf_nas_5gs_mm_pld_cont = -1;
static int hf_nas_5gs_mm_req_type = -1;
static int hf_nas_5gs_mm_serv_type = -1;
static int hf_nas_5gs_mm_5g_ea0 = -1;
static int hf_nas_5gs_mm_128_5g_ea1 = -1;
static int hf_nas_5gs_mm_128_5g_ea2 = -1;
static int hf_nas_5gs_mm_128_5g_ea3 = -1;
static int hf_nas_5gs_mm_5g_ea4 = -1;
static int hf_nas_5gs_mm_5g_ea5 = -1;
static int hf_nas_5gs_mm_5g_ea6 = -1;
static int hf_nas_5gs_mm_5g_ea7 = -1;
static int hf_nas_5gs_mm_5g_ia0 = -1;
static int hf_nas_5gs_mm_5g_128_ia1 = -1;
static int hf_nas_5gs_mm_5g_128_ia2 = -1;
static int hf_nas_5gs_mm_5g_128_ia3 = -1;
static int hf_nas_5gs_mm_5g_ia4 = -1;
static int hf_nas_5gs_mm_5g_ia5 = -1;
static int hf_nas_5gs_mm_5g_ia6 = -1;
static int hf_nas_5gs_mm_5g_ia7 = -1;
static int hf_nas_5gs_mm_eea0 = -1;
static int hf_nas_5gs_mm_128eea1 = -1;
static int hf_nas_5gs_mm_128eea2 = -1;
static int hf_nas_5gs_mm_eea3 = -1;
static int hf_nas_5gs_mm_eea4 = -1;
static int hf_nas_5gs_mm_eea5 = -1;
static int hf_nas_5gs_mm_eea6 = -1;
static int hf_nas_5gs_mm_eea7 = -1;
static int hf_nas_5gs_mm_eia0 = -1;
static int hf_nas_5gs_mm_128eia1 = -1;
static int hf_nas_5gs_mm_128eia2 = -1;
static int hf_nas_5gs_mm_eia3 = -1;
static int hf_nas_5gs_mm_eia4 = -1;
static int hf_nas_5gs_mm_eia5 = -1;
static int hf_nas_5gs_mm_eia6 = -1;
static int hf_nas_5gs_mm_eia7 = -1;
static int hf_nas_5gs_mm_n1_mode_reg_b1 = -1;
static int hf_nas_5gs_mm_s1_mode_reg_b0 = -1;

static int hf_nas_5gs_mm_sal_al_t = -1;
static int hf_nas_5gs_mm_sal_t_li = -1;
static int hf_nas_5gs_mm_sal_num_e = -1;

static int hf_nas_5gs_pdu_ses_sts_psi_7_b7 = -1;
static int hf_nas_5gs_pdu_ses_sts_psi_6_b6 = -1;
static int hf_nas_5gs_pdu_ses_sts_psi_5_b5 = -1;
static int hf_nas_5gs_pdu_ses_sts_psi_4_b4 = -1;
static int hf_nas_5gs_pdu_ses_sts_psi_3_b3 = -1;
static int hf_nas_5gs_pdu_ses_sts_psi_2_b2 = -1;
static int hf_nas_5gs_pdu_ses_sts_psi_1_b1 = -1;
static int hf_nas_5gs_pdu_ses_sts_psi_0_b0 = -1;

static int hf_nas_5gs_pdu_ses_sts_psi_15_b7 = -1;
static int hf_nas_5gs_pdu_ses_sts_psi_14_b6 = -1;
static int hf_nas_5gs_pdu_ses_sts_psi_13_b5 = -1;
static int hf_nas_5gs_pdu_ses_sts_psi_12_b4 = -1;
static int hf_nas_5gs_pdu_ses_sts_psi_11_b3 = -1;
static int hf_nas_5gs_pdu_ses_sts_psi_10_b2 = -1;
static int hf_nas_5gs_pdu_ses_sts_psi_9_b1 = -1;
static int hf_nas_5gs_pdu_ses_sts_psi_8_b0 = -1;

static int hf_nas_5gs_pdu_ses_rect_res_psi_7_b7 = -1;
static int hf_nas_5gs_pdu_ses_rect_res_psi_6_b6 = -1;
static int hf_nas_5gs_pdu_ses_rect_res_psi_5_b5 = -1;
static int hf_nas_5gs_pdu_ses_rect_res_psi_4_b4 = -1;
static int hf_nas_5gs_pdu_ses_rect_res_psi_3_b3 = -1;
static int hf_nas_5gs_pdu_ses_rect_res_psi_2_b2 = -1;
static int hf_nas_5gs_pdu_ses_rect_res_psi_1_b1 = -1;
static int hf_nas_5gs_pdu_ses_rect_res_psi_0_b0 = -1;

static int hf_nas_5gs_pdu_ses_rect_res_psi_15_b7 = -1;
static int hf_nas_5gs_pdu_ses_rect_res_psi_14_b6 = -1;
static int hf_nas_5gs_pdu_ses_rect_res_psi_13_b5 = -1;
static int hf_nas_5gs_pdu_ses_rect_res_psi_12_b4 = -1;
static int hf_nas_5gs_pdu_ses_rect_res_psi_11_b3 = -1;
static int hf_nas_5gs_pdu_ses_rect_res_psi_10_b2 = -1;
static int hf_nas_5gs_pdu_ses_rect_res_psi_9_b1 = -1;
static int hf_nas_5gs_pdu_ses_rect_res_psi_8_b0 = -1;

static int hf_nas_5gs_ul_data_sts_psi_7_b7 = -1;
static int hf_nas_5gs_ul_data_sts_psi_6_b6 = -1;
static int hf_nas_5gs_ul_data_sts_psi_5_b5 = -1;
static int hf_nas_5gs_ul_data_sts_psi_4_b4 = -1;
static int hf_nas_5gs_ul_data_sts_psi_3_b3 = -1;
static int hf_nas_5gs_ul_data_sts_psi_2_b2 = -1;
static int hf_nas_5gs_ul_data_sts_psi_1_b1 = -1;
static int hf_nas_5gs_ul_data_sts_psi_0_b0 = -1;

static int hf_nas_5gs_ul_data_sts_psi_15_b7 = -1;
static int hf_nas_5gs_ul_data_sts_psi_14_b6 = -1;
static int hf_nas_5gs_ul_data_sts_psi_13_b5 = -1;
static int hf_nas_5gs_ul_data_sts_psi_12_b4 = -1;
static int hf_nas_5gs_ul_data_sts_psi_11_b3 = -1;
static int hf_nas_5gs_ul_data_sts_psi_10_b2 = -1;
static int hf_nas_5gs_ul_data_sts_psi_9_b1 = -1;
static int hf_nas_5gs_ul_data_sts_psi_8_b0 = -1;

static int hf_nas_5gs_allow_pdu_ses_sts_psi_7_b7 = -1;
static int hf_nas_5gs_allow_pdu_ses_sts_psi_6_b6 = -1;
static int hf_nas_5gs_allow_pdu_ses_sts_psi_5_b5 = -1;
static int hf_nas_5gs_allow_pdu_ses_sts_psi_4_b4 = -1;
static int hf_nas_5gs_allow_pdu_ses_sts_psi_3_b3 = -1;
static int hf_nas_5gs_allow_pdu_ses_sts_psi_2_b2 = -1;
static int hf_nas_5gs_allow_pdu_ses_sts_psi_1_b1 = -1;
static int hf_nas_5gs_allow_pdu_ses_sts_psi_0_b0 = -1;

static int hf_nas_5gs_allow_pdu_ses_sts_psi_15_b7 = -1;
static int hf_nas_5gs_allow_pdu_ses_sts_psi_14_b6 = -1;
static int hf_nas_5gs_allow_pdu_ses_sts_psi_13_b5 = -1;
static int hf_nas_5gs_allow_pdu_ses_sts_psi_12_b4 = -1;
static int hf_nas_5gs_allow_pdu_ses_sts_psi_11_b3 = -1;
static int hf_nas_5gs_allow_pdu_ses_sts_psi_10_b2 = -1;
static int hf_nas_5gs_allow_pdu_ses_sts_psi_9_b1 = -1;
static int hf_nas_5gs_allow_pdu_ses_sts_psi_8_b0 = -1;

static int hf_nas_5gs_sm_pdu_session_type = -1;
static int hf_nas_5gs_sm_sc_mode = -1;
static int hf_nas_5gs_sm_sel_sc_mode = -1;
static int hf_nas_5gs_sm_rqos_b0 = -1;
static int hf_nas_5gs_sm_5gsm_cause = -1;
static int hf_nas_5gs_sm_pdu_ses_type = -1;
static int hf_nas_5gs_sm_pdu_addr_inf_ipv4 = -1;
static int hf_nas_5gs_sm_pdu_addr_inf_ipv6 = -1;
static int hf_nas_5gs_sm_qos_rule_id = -1;
static int hf_nas_5gs_sm_length = -1;
static int hf_nas_5gs_sm_rop = -1;
static int hf_nas_5gs_sm_dqr = -1;
static int hf_nas_5gs_sm_nof_pkt_filters = -1;
static int hf_nas_5gs_sm_pkt_flt_id = -1;
static int hf_nas_5gs_sm_pkt_flt_dir = -1;
static int hf_nas_5gs_sm_pf_len = -1;
static int hf_nas_5gs_sm_pf_type = -1;
static int hf_nas_5gs_sm_e = -1;
static int hf_nas_5gs_sm_nof_params = -1;
static int hf_nas_5gs_sm_param_id = -1;
static int hf_nas_5gs_sm_param_len = -1;
static int hf_nas_5gs_sm_qos_rule_precedence = -1;
static int hf_nas_5gs_sm_pal_cont = -1;
static int hf_nas_5gs_sm_qfi = -1;
static int hf_nas_5gs_sm_mapd_eps_b_cont_id = -1;
static int hf_nas_5gs_sm_mapd_eps_b_cont_opt_code = -1;
static int hf_nas_5gs_sm_qos_des_flow_opt_code = -1;
static int hf_nas_5gs_sm_mapd_eps_b_cont_DEB = -1;
static int hf_nas_5gs_sm_mapd_eps_b_cont_E = -1;
static int hf_nas_5gs_sm_mapd_eps_b_cont_num_eps_parms = -1;
static int hf_nas_5gs_sm_mapd_eps_b_cont_E_mod = -1;
static int hf_nas_5gs_sm_mapd_eps_b_cont_param_id = -1;

static int hf_nas_5gs_sm_unit_for_session_ambr_dl = -1;
static int hf_nas_5gs_sm_session_ambr_dl = -1;
static int hf_nas_5gs_sm_unit_for_session_ambr_ul = -1;
static int hf_nas_5gs_sm_session_ambr_ul = -1;
static int hf_nas_5gs_sm_all_ssc_mode_b0 = -1;
static int hf_nas_5gs_sm_all_ssc_mode_b1 = -1;
static int hf_nas_5gs_sm_all_ssc_mode_b2 = -1;
static int hf_nas_5gs_addr_mask_ipv4 = -1;
static int hf_nas_5gs_protocol_identifier_or_next_hd = -1;
static int hf_nas_5gs_protocol_identifier_or_next_hd_val = -1;
static int hf_nas_5gs_mm_rinmr = -1;
static int hf_nas_5gs_mm_hdp = -1;
static int hf_nas_5gs_mm_dcni = -1;
static int hf_nas_5gs_mm_nssci = -1;
static int hf_nas_5gs_mm_nssai_inc_mode = -1;
static int hf_nas_5gs_mm_ue_usage_setting = -1;
static int hf_nas_5gs_mm_ng_ran_rcu = -1;
static int hf_nas_5gs_mm_5gs_drx_param = -1;

static int ett_nas_5gs = -1;
static int ett_nas_5gs_mm_nssai = -1;
static int ett_nas_5gs_mm_pdu_ses_id = -1;
static int ett_nas_5gs_sm_qos_rules = -1;
static int ett_nas_5gs_sm_qos_params = -1;
static int ett_nas_5gs_plain = -1;
static int ett_nas_5gs_sec = -1;
static int ett_nas_5gs_mm_part_sal = -1;
static int ett_nas_5gs_mm_part_tal = -1;
static int ett_nas_5gs_sm_mapd_eps_b_cont = -1;
static int ett_nas_5gs_sm_mapd_eps_b_cont_params_list = -1;
static int ett_nas_5gs_enc = -1;
static int ett_nas_5gs_mm_ladn_indic = -1;
static int ett_nas_5gs_mm_sor = -1;

static int hf_nas_5gs_mm_abba = -1;
static int hf_nas_5gs_mm_suci = -1;
static int hf_nas_5gs_mm_imei = -1;
static int hf_nas_5gs_mm_imeisv = -1;
static int hf_nas_5gs_mm_reg_res_sms_allowed = -1;
static int hf_nas_5gs_mm_reg_res_res = -1;
static int hf_nas_5gs_amf_region_id = -1;
static int hf_nas_5gs_amf_set_id = -1;
static int hf_nas_5gs_amf_pointer = -1;
static int hf_nas_5gs_5g_tmsi = -1;
static int hf_nas_5gs_mm_precedence = -1;
static int hf_nas_5gs_mm_sms_indic_sai = -1;

static int hf_nas_5gs_nw_feat_sup_mpsi_b7 = -1;
static int hf_nas_5gs_nw_feat_sup_ims_iwk_n26_b6 = -1;
static int hf_nas_5gs_nw_feat_sup_ims_emf_b5b4 = -1;
static int hf_nas_5gs_nw_feat_sup_ims_emc_b3b2 = -1;
static int hf_nas_5gs_nw_feat_sup_ims_vops_b1b0 = -1;

static int hf_nas_5gs_tac = -1;

static int hf_nas_5gs_mm_tal_t_li = -1;
static int hf_nas_5gs_mm_tal_num_e = -1;
static int hf_nas_5gs_sm_mapd_eps_b_cont_eps_param_cont = -1;

static int hf_nas_5gs_kacf = -1;
static int hf_nas_5gs_ncc = -1;

static int hf_nas_5gs_sor_hdr0_ack = -1;
static int hf_nas_5gs_sor_hdr0_list_type = -1;
static int hf_nas_5gs_sor_hdr0_list_ind = -1;
static int hf_nas_5gs_sor_hdr0_sor_data_type = -1;
static int hf_nas_5gs_sor_mac_iue = -1;
static int hf_nas_5gs_sor_mac_iausf = -1;
static int hf_nas_5gs_counter_sor = -1;
static int hf_nas_5gs_sor_sec_pkt = -1;

static int hf_nas_5gs_acces_tech_o1_b7 = -1;
static int hf_nas_5gs_acces_tech_o1_b6 = -1;
static int hf_nas_5gs_acces_tech_o1_b5 = -1;
static int hf_nas_5gs_acces_tech_o1_b4 = -1;
static int hf_nas_5gs_acces_tech_o1_b3 = -1;

static int hf_nas_5gs_acces_tech_o2_b7 = -1;
static int hf_nas_5gs_acces_tech_o2_b6 = -1;
static int hf_nas_5gs_acces_tech_o2_b5 = -1;
static int hf_nas_5gs_acces_tech_o2_b4 = -1;
static int hf_nas_5gs_acces_tech_o2_b3 = -1;
static int hf_nas_5gs_acces_tech_o2_b2 = -1;

static expert_field ei_nas_5gs_extraneous_data = EI_INIT;
static expert_field ei_nas_5gs_unknown_pd = EI_INIT;
static expert_field ei_nas_5gs_mm_unknown_msg_type = EI_INIT;
static expert_field ei_nas_5gs_sm_unknown_msg_type = EI_INIT;
static expert_field ei_nas_5gs_msg_not_dis = EI_INIT;
static expert_field ei_nas_5gs_ie_not_dis = EI_INIT;
static expert_field ei_nas_5gs_missing_mandatory_elemen = EI_INIT;
static expert_field ei_nas_5gs_dnn_too_long = EI_INIT;
static expert_field ei_nas_5gs_unknown_value = EI_INIT;
static expert_field ei_nas_5gs_num_pkt_flt = EI_INIT;
static expert_field ei_nas_5gs_not_diss = EI_INIT;

#define NAS_5GS_PLAN_NAS_MSG 0

static const value_string nas_5gs_security_header_type_vals[] = {
    { 0,    "Plain NAS message, not security protected"},
    { 1,    "Integrity protected"},
    { 2,    "Integrity protected and ciphered"},
    { 3,    "Integrity protected with new 5GS security context"},
    { 4,    "Integrity protected and ciphered with new 5GS security context"},
    { 0,    NULL }
};


#define TGPP_PD_5GMM 0x7e
#define TGPP_PD_5GSM 0x2e

static const value_string nas_5gs_epd_vals[] = {
    { 0x00,              "Group call control" },
    { 0x01,              "Broadcast call control" },
    { 0x02,              "EPS session management messages" },
    { 0x03,              "Call Control; call related SS messages" },
    { 0x04,              "GPRS Transparent Transport Protocol (GTTP)" },
    { 0x05,              "Mobility Management messages" },
    { 0x06,              "Radio Resources Management messages" },
    { 0x07,              "EPS mobility management messages" },
    { 0x08,              "GPRS mobility management messages" },
    { 0x09,              "SMS messages" },
    { 0x0a,              "GPRS session management messages" },
    { 0x0b,              "Non call related SS messages" },
    { 0x0c,              "Location services specified in 3GPP TS 44.071" },
    { 0x0d,              "Unknown" },
    /*{0x0e,            "Reserved for extension of the PD to one octet length "},*/
    { 0x0f,              "Tests procedures described in 3GPP TS 44.014, 3GPP TS 34.109 and 3GPP TS 36.509" },
    { TGPP_PD_5GSM,      "5G session management messages" },
    { TGPP_PD_5GMM,      "5G mobility management messages" },
    { 0,    NULL }
};

struct nas5gs_private_data {
    guint32 payload_container_type;
};

static struct nas5gs_private_data*
nas5gs_get_private_data(packet_info *pinfo)
{
    struct nas5gs_private_data *nas5gs_data = (struct nas5gs_private_data*)p_get_proto_data(pinfo->pool, pinfo, proto_nas_5gs, pinfo->curr_layer_num);
    if (!nas5gs_data) {
        nas5gs_data = wmem_new0(pinfo->pool, struct nas5gs_private_data);
        nas5gs_data->payload_container_type = 0;
        p_add_proto_data(pinfo->pool, pinfo, proto_nas_5gs, pinfo->curr_layer_num, nas5gs_data);
    }
    return nas5gs_data;
}

static guint32
get_ext_ambr_unit(guint32 unit, const char **unit_str)
{
    guint32 mult;

    if (unit == 0) {
        mult = 1;
        *unit_str = "Unit value 0, Illegal";
        return mult;
    }
    unit = unit - 1;

    if (unit <= 0x05) {
        mult = pow4(guint32, unit);
        *unit_str = "Kbps";
    } else if (unit <= 0x0a) {
        mult = pow4(guint32, unit - 0x05);
        *unit_str = "Mbps";
    } else if (unit <= 0x0e) {
        mult = pow4(guint32, unit - 0x07);
        *unit_str = "Gbps";
    } else if (unit <= 0x14) {
        mult = pow4(guint32, unit - 0x0c);
        *unit_str = "Tbps";
    } else if (unit <= 0x19) {
        mult = pow4(guint32, unit - 0x11);
        *unit_str = "Pbps";
    } else {
        mult = 256;
        *unit_str = "Pbps";
    }
    return mult;
}

/*
 * 9.11.3 5GS mobility management (5GMM) information elements
 */

 /*
  * 9.11.3.1 5GMM capability
  */
static guint16
de_nas_5gs_mm_5gmm_cap(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{
    static const int * flags[] = {
        &hf_nas_5gs_spare_b7,
        &hf_nas_5gs_spare_b6,
        &hf_nas_5gs_spare_b5,
        &hf_nas_5gs_spare_b4,
        &hf_nas_5gs_spare_b3,
        &hf_nas_5gs_mm_lpp_cap_b2,
        &hf_nas_5gs_mm_ho_attach_b1,
        &hf_nas_5gs_mm_s1_mode_b0,
        NULL
    };

    proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags, ENC_BIG_ENDIAN);

    return 1;
}

/*
 * 9.11.3.2 5GMM cause
 */

static const value_string nas_5gs_mm_cause_vals[] = {
    { 0x03, "Illegal UE" },
    { 0x05, "PEI not accepted" },
    { 0x06, "Illegal ME" },
    { 0x07, "5GS services not allowed" },
    { 0x09, "UE identity cannot be derived by the network" },
    { 0x0a, "Implicitly deregistered" },
    { 0x0b, "PLMN not allowed" },
    { 0x0c, "Tracking area not allowed" },
    { 0x0d, "Roaming not allowed in this tracking area" },
    { 0x0f, "No suitable cells in tracking area" },
    { 0x14, "MAC failure" },
    { 0x15, "Synch failure" },
    { 0x16, "Congestion" },
    { 0x17, "UE security capabilities mismatch" },
    { 0x18, "Security mode rejected, unspecified" },
    { 0x1a, "Non-5G authentication unacceptable" },
    { 0x1b, "N1 mode not allowed" },
    { 0x1c, "Restricted service area" },
    { 0x2b, "LADN not available" },
    { 0x41, "Maximum number of PDU sessions reached" },
    { 0x43, "Insufficient resources for specific slice and DNN" },
    { 0x45, "Insufficient resources for specific slice" },
    { 0x47, "ngKSI already in use" },
    { 0x48, "Non-3GPP access to 5GCN not allowed" },
    { 0x49, "Serving network not authorized" },
    { 0x5a, "Payload was not forwarded" },
    { 0x5b, "DNN not supported or not subscribed in the slice" },
    { 0x5c, "Insufficient user-plane resources for the PDU session" },
    { 0x5f, "Semantically incorrect message" },
    { 0x60, "Invalid mandatory information" },
    { 0x61, "Message type non-existent or not implemented" },
    { 0x62, "Message type not compatible with the protocol state" },
    { 0x63, "Information element non-existent or not implemented" },
    { 0x64, "Conditional IE error" },
    { 0x65, "Message not compatible with the protocol state" },
    { 0x6f, "Protocol error, unspecified" },
    { 0,    NULL }
};

static guint16
de_nas_5gs_mm_5gmm_cause(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{
    guint32 cause;

    proto_tree_add_item_ret_uint(tree, hf_nas_5gs_mm_5gmm_cause, tvb, offset, 1, ENC_BIG_ENDIAN, &cause);

    col_append_fstr(pinfo->cinfo, COL_INFO, " (%s)",
        val_to_str_const(cause, nas_5gs_mm_cause_vals, "Unknown"));


    return 1;
}


static const value_string nas_5gs_mm_drx_vals[] = {
    { 0x0, "DRX value not specified" },
    { 0x1, "DRX cycle parameter T = 32" },
    { 0x2, "DRX cycle parameter T = 64" },
    { 0x3, "DRX cycle parameter T = 128" },
    { 0x4, "DRX cycle parameter T = 256" },
    { 0, NULL }
};


/* 9.11.3.2A    5GS DRX parameters*/
static guint16
de_nas_5gs_mm_5gs_drx_param(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{

    proto_tree_add_item(tree, hf_nas_5gs_mm_5gs_drx_param, tvb, offset, 1, ENC_BIG_ENDIAN);

    return 1;
}

/*
 * 9.11.3.3 5GS identity type
 */
static guint16
de_nas_5gs_mm_5gs_identity_type(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{

    proto_tree_add_item(tree, hf_nas_5gs_mm_type_id, tvb, offset, 1, ENC_BIG_ENDIAN);

    return 1;
}

/*
 * 9.11.3.4    5GS mobile identity
 */
static const value_string nas_5gs_mm_type_id_vals[] = {
    { 0x0, "No identity" },
    { 0x1, "SUCI" },
    { 0x2, "5G-GUTI" },
    { 0x3, "IMEI" },
    { 0x4, "5G-S-TMSI" },
    { 0x5, "IMEISV" },
    { 0, NULL }
 };

static true_false_string nas_5gs_odd_even_tfs = {
    "Odd number of identity digits",
    "Even number of identity digits"
};

static guint16
de_nas_5gs_mm_5gs_mobile_id(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    guint8 type_id;
    tvbuff_t * new_tvb;
    const char *digit_str;

    static const int * flags_odd_even_tid[] = {
        &hf_nas_5gs_mm_odd_even,
        &hf_nas_5gs_mm_type_id,
        NULL
    };

    type_id = tvb_get_guint8(tvb, offset) & 0x07;

    switch (type_id) {
    case 0:
        proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags_odd_even_tid, ENC_BIG_ENDIAN);
        break;
    case 1:
        /* SUCI */
        proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags_odd_even_tid, ENC_BIG_ENDIAN);
        new_tvb = tvb_new_subset_length(tvb, offset, len);
        digit_str = tvb_bcd_dig_to_wmem_packet_str(new_tvb, 0, -1, NULL, TRUE);
        proto_tree_add_string(tree, hf_nas_5gs_mm_suci, new_tvb, 0, -1, digit_str);
        break;
    case 2:
        /* 5G-GUTI*/
        proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags_odd_even_tid, ENC_BIG_ENDIAN);
        offset++;
        /* MCC digit 2    MCC digit 1
         * MNC digit 3     MCC digit 3
         * MNC digit 2    MNC digit 1
         */
        offset = dissect_e212_mcc_mnc(tvb, pinfo, tree, offset, E212_NONE, TRUE);
        /* AMF Region ID octet 7 */
        proto_tree_add_item(tree, hf_nas_5gs_amf_region_id, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset += 1;
        /* AMF Set ID octet 8 */
        proto_tree_add_item(tree, hf_nas_5gs_amf_set_id, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset++;
        /* AMF AMF Pointer AMF Set ID (continued) */
        proto_tree_add_item(tree, hf_nas_5gs_amf_pointer, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;
        proto_tree_add_item(tree, hf_nas_5gs_5g_tmsi, tvb, offset, 4, ENC_BIG_ENDIAN);
        break;
    case 3:
        /* IMEI */
        proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags_odd_even_tid, ENC_BIG_ENDIAN);
        new_tvb = tvb_new_subset_length(tvb, offset, len);
        digit_str = tvb_bcd_dig_to_wmem_packet_str(new_tvb, 0, -1, NULL, TRUE);
        proto_tree_add_string(tree, hf_nas_5gs_mm_imei, new_tvb, 0, -1, digit_str);
        break;
    case 4:
        /*5G-S-TMSI*/
        proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags_odd_even_tid, ENC_BIG_ENDIAN);
        offset++;
        /* AMF Set ID */
        proto_tree_add_item(tree, hf_nas_5gs_amf_set_id, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset++;
        /* AMF Pointer AMF Set ID (continued) */
        proto_tree_add_item(tree, hf_nas_5gs_amf_pointer, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;
        proto_tree_add_item(tree, hf_nas_5gs_5g_tmsi, tvb, offset, 4, ENC_BIG_ENDIAN);
        break;
    case 5:
        /* IMEISV */
        proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags_odd_even_tid, ENC_BIG_ENDIAN);
        new_tvb = tvb_new_subset_length(tvb, offset, len);
        digit_str = tvb_bcd_dig_to_wmem_packet_str(new_tvb, 0, -1, NULL, TRUE);
        proto_tree_add_string(tree, hf_nas_5gs_mm_imeisv, new_tvb, 0, -1, digit_str);
        break;

    default:
        proto_tree_add_item(tree, hf_nas_5gs_mm_type_id, tvb, offset, 1, ENC_BIG_ENDIAN);
        proto_tree_add_expert(tree, pinfo, &ei_nas_5gs_unknown_value, tvb, offset, len);
        break;
    }

    return len;
}

/*
 * 9.11.3.5    5GS network feature support
 */


static const value_string nas_5gs_nw_feat_sup_ims_vops_values[] = {
    { 0x0, "IMS voice over PS session not supported" },
    { 0x1, "IMS voice over PS session supported over 3GPP access" },
    { 0x2, "IMS voice over PS session supported over non - 3GPP access" },
    { 0x3, "Reserved" },
    { 0, NULL }
};

static const value_string nas_5gs_nw_feat_sup_emc_values[] = {
    { 0x0, "Emergency services not supported" },
    { 0x1, "Emergency services supported in NR connected to 5GCN only" },
    { 0x2, "Emergency services supported in E-UTRA connected to 5GCN only" },
    { 0x3, "Emergency services supported in NR connected to 5GCN and E-UTRA connected to 5GCN" },
    { 0, NULL }
};

static const value_string nas_5gs_nw_feat_sup_emf_values[] = {
    { 0x0, "Emergency services fallback not supported" },
    { 0x1, "Emergency services fallback supported in NR connected to 5GCN only" },
    { 0x2, "Emergency services fallback supported in E-UTRA connected to 5GCN only" },
    { 0x3, "mergency services fallback supported in NR connected to 5GCN and E-UTRA connected to 5GCN" },
    { 0, NULL }
};

static const true_false_string tfs_nas_5gs_nw_feat_sup_mpsi = {
    "Access identity 1 valid in RPLMN or equivalent PLMN",
    "Access identity 1 not valid in RPLMN or equivalent PLMN"
};

static guint16
de_nas_5gs_mm_5gs_nw_feat_sup(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{

    static const int * flags[] = {
        &hf_nas_5gs_nw_feat_sup_mpsi_b7,
        &hf_nas_5gs_nw_feat_sup_ims_iwk_n26_b6,
        &hf_nas_5gs_nw_feat_sup_ims_emf_b5b4,
        &hf_nas_5gs_nw_feat_sup_ims_emc_b3b2,
        &hf_nas_5gs_nw_feat_sup_ims_vops_b1b0,
        NULL
    };


    /* MPSI    IWK N26    EMF    EMC    IMS VoPS    octet 3*/
    proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags, ENC_BIG_ENDIAN);


    return len;
}

/*
 * 9.11.3.6    5GS registration result
 */

static const value_string nas_5gs_mm_reg_res_values[] = {
    { 0x1, "3GPP access" },
    { 0x2, "Non-3GPP access" },
    { 0x3, "3GPP access and non-3GPP access" },
{ 0, NULL }
};


static guint16
de_nas_5gs_mm_5gs_reg_res(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{
    /* 0 Spare 0 Spare 0 Spare 0 Spare SMS allowed 5GS registration result value */
    proto_tree_add_item(tree, hf_nas_5gs_mm_reg_res_sms_allowed, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_nas_5gs_mm_reg_res_res, tvb, offset, 1, ENC_BIG_ENDIAN);

    return 1;
}

/*
 * 9.11.3.7    5GS registration type
 */

static const value_string nas_5gs_registration_type_values[] = {
    { 0x1, "initial registration" },
    { 0x2, "mobility registration updating" },
    { 0x3, "periodic registration updating" },
    { 0x4, "emergency registration" },
    { 0x7, "reserved" },
    { 0, NULL }
 };

static true_false_string nas_5gs_for_tfs = {
    "Follow-on request pending",
    "No follow-on request pending"
};

static const int * nas_5gs_registration_type_flags[] = {
    &hf_nas_5gs_mm_for,
    &hf_nas_5gs_mm_5gs_reg_type,
    NULL
};

static guint16
de_nas_5gs_mm_5gs_reg_type(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{



    /* FOR    SMS requested    5GS registration type value    octet 3*/
    proto_tree_add_bitmask_list(tree, tvb, offset, 1, nas_5gs_registration_type_flags, ENC_BIG_ENDIAN);

    return 1;
}

/*
 * 9.11.3.8     5GS tracking area identity
 */
static guint16
de_nas_5gs_mm_5gs_ta_id(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{
    /* MCC digit 2    MCC digit 1 Octet 2*/
    /* MNC digit 3    MCC digit 3 Octet 3*/
    /* MNC digit 2    MNC digit 1 Octet 4*/
    /* TAC Octet 5 - 7 */
    guint32 curr_offset;

    curr_offset = offset;

    curr_offset = dissect_e212_mcc_mnc(tvb, pinfo, tree, curr_offset, E212_TAI, TRUE);
    proto_tree_add_item(tree, hf_nas_5gs_tac, tvb, curr_offset, 3, ENC_BIG_ENDIAN);
    curr_offset += 3;

    return(curr_offset - offset);
}

/*
 * 9.11.3.9     5GS tracking area identity list
 */
static const value_string nas_5gs_mm_tal_t_li_values[] = {
    { 0x00, "list of TACs belonging to one PLMN, with non-consecutive TAC values" },
    { 0x01, "list of TACs belonging to one PLMN, with consecutive TAC values" },
    { 0x02, "list of TAIs belonging to different PLMNs" },
    { 0, NULL } };

static const value_string nas_5gs_mm_tal_num_e[] = {
    { 0x00, "1 element" },
    { 0x01, "2 elements" },
    { 0x02, "3 elements" },
    { 0x03, "4 elements" },
    { 0x04, "5 elements" },
    { 0x05, "6 elements" },
    { 0x06, "7 elements" },
    { 0x07, "8 elements" },
    { 0x08, "9 elements" },
    { 0x09, "10 elements" },
    { 0x0a, "11 elements" },
    { 0x0b, "12 elements" },
    { 0x0c, "13 elements" },
    { 0x0d, "14 elements" },
    { 0x0e, "15 elements" },
    { 0x0f, "16 elements" },
    { 0, NULL }
};

static guint16
de_nas_5gs_mm_5gs_ta_id_list(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    proto_tree *sub_tree;
    proto_item *item;

    static const int * flags[] = {
        &hf_nas_5gs_mm_tal_t_li,
        &hf_nas_5gs_mm_tal_num_e,
        NULL
    };

    guint num_par_tal = 1;
    guint32 curr_offset = offset;
    guint32 start_offset;
    guint8 tal_head, tal_t_li, tal_num_e;

    /*Partial tracking area list*/
    while ((curr_offset - offset) < len) {
        start_offset = curr_offset;
        sub_tree = proto_tree_add_subtree_format(tree, tvb, curr_offset, -1, ett_nas_5gs_mm_part_tal, &item, "Partial tracking area list  %u", num_par_tal);
        /*Head of Partial tracking area list*/
        /* Type of list    Number of elements    octet 1 */
        tal_head = tvb_get_guint8(tvb, curr_offset);
        tal_t_li = (tal_head & 0x60) >> 5;
        tal_num_e = (tal_head & 0x1f) + 1;
        proto_tree_add_bitmask_list(sub_tree, tvb, curr_offset, 1, flags, ENC_BIG_ENDIAN);
        curr_offset++;
        switch (tal_t_li) {
        case 0:
            /*octet 2  MCC digit2  MCC digit1*/
            /*octet 3  MNC digit3  MCC digit3*/
            /*octet 4  MNC digit2  MNC digit1*/
            dissect_e212_mcc_mnc(tvb, pinfo, sub_tree, curr_offset, E212_NONE, FALSE);
            curr_offset += 3;
            while (tal_num_e > 0) {
                proto_tree_add_item(sub_tree, hf_nas_5gs_tac, tvb, curr_offset, 3, ENC_BIG_ENDIAN);
                curr_offset += 3;
                tal_num_e--;
            }
            break;
        case 1:
            /*octet 2  MCC digit2  MCC digit1*/
            /*octet 3  MNC digit3  MCC digit3*/
            /*octet 4  MNC digit2  MNC digit1*/
            dissect_e212_mcc_mnc(tvb, pinfo, sub_tree, curr_offset, E212_NONE, FALSE);
            curr_offset += 3;

            /*octet 5  TAC 1*/
            proto_tree_add_item(sub_tree, hf_nas_5gs_tac, tvb, curr_offset, 3, ENC_BIG_ENDIAN);
            curr_offset+=3;
            break;
        case 2:
            while (tal_num_e > 0) {
                /*octet 2  MCC digit2  MCC digit1*/
                /*octet 3  MNC digit3  MCC digit3*/
                /*octet 4  MNC digit2  MNC digit1*/
                dissect_e212_mcc_mnc(tvb, pinfo, sub_tree, curr_offset, E212_NONE, FALSE);
                curr_offset += 3;

                /*octet 5  TAC 1*/
                proto_tree_add_item(sub_tree, hf_nas_5gs_tac, tvb, curr_offset, 3, ENC_BIG_ENDIAN);
                curr_offset += 3;

                tal_num_e--;
            }
            break;
        case 3:
            dissect_e212_mcc_mnc(tvb, pinfo, sub_tree, curr_offset, E212_NONE, FALSE);
            curr_offset += 3;
            break;
        default:
            proto_tree_add_expert(sub_tree, pinfo, &ei_nas_5gs_unknown_value, tvb, curr_offset, len - 1);
        }

        /*calculate the length of IE?*/
        proto_item_set_len(item, curr_offset - start_offset);
        /*calculate the number of Partial tracking area list*/
        num_par_tal++;
    }

    return len;
}

/*
 * 9.11.3.9A    5GS update type
 */

static true_false_string tfs_nas5gs_sms_requested = {
    "SMS over NAS supported",
    "SMS over NAS not supported"
};

static guint16
de_nas_5gs_mm_update_type(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{
    static const int * flags[] = {
        &hf_nas_5gs_spare_b3,
        &hf_nas_5gs_spare_b2,
        &hf_nas_5gs_mm_ng_ran_rcu,
        &hf_nas_5gs_mm_sms_requested,
        NULL
    };

    proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags, ENC_BIG_ENDIAN);

    return 1;

}

/*
 * 9.11.3.10    ABBA
 */
static guint16
de_nas_5gs_mm_abba(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    proto_tree_add_item(tree, hf_nas_5gs_mm_abba, tvb, offset, len, ENC_BIG_ENDIAN);
    return len;
}

/*
 * 9.11.3.11    Access type
 */
static guint16
de_nas_5gs_mm_access_type(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{
    static const int * flags[] = {
        &hf_nas_5gs_spare_b3,
        &hf_nas_5gs_spare_b2,
        &hf_nas_5gs_mm_acc_type,
        NULL
    };

    proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags, ENC_BIG_ENDIAN);

    return 1;

}

/*
 * 9.11.3.12    Additional 5G security information
 */
static guint16
de_nas_5gs_mm_add_5g_sec_inf(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{
    static const int * flags[] = {
        &hf_nas_5gs_spare_b3,
        &hf_nas_5gs_spare_b2,
        &hf_nas_5gs_mm_rinmr,
        &hf_nas_5gs_mm_hdp,
        NULL
    };

    proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags, ENC_BIG_ENDIAN);

    return 1;
}

/*
 *  9.11.3.13    Allowed PDU session status
 */
static true_false_string tfs_nas_5gs_allow_pdu_ses_sts_psi = {
    "user-plane resources of corresponding PDU session can be re-established over 3GPP access",
    "user-plane resources of corresponding PDU session is not allowed to be re-established over 3GPP access"
};

static guint16
de_nas_5gs_mm_allow_pdu_ses_sts(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)

{
    int curr_offset;

    static const int * psi_0_7_flags[] = {
        &hf_nas_5gs_allow_pdu_ses_sts_psi_7_b7,
        &hf_nas_5gs_allow_pdu_ses_sts_psi_6_b6,
        &hf_nas_5gs_allow_pdu_ses_sts_psi_5_b5,
        &hf_nas_5gs_allow_pdu_ses_sts_psi_4_b4,
        &hf_nas_5gs_allow_pdu_ses_sts_psi_3_b3,
        &hf_nas_5gs_allow_pdu_ses_sts_psi_2_b2,
        &hf_nas_5gs_allow_pdu_ses_sts_psi_1_b1,
        &hf_nas_5gs_allow_pdu_ses_sts_psi_0_b0,
        NULL
    };

    static const int * psi_8_15_flags[] = {
        &hf_nas_5gs_allow_pdu_ses_sts_psi_15_b7,
        &hf_nas_5gs_allow_pdu_ses_sts_psi_14_b6,
        &hf_nas_5gs_allow_pdu_ses_sts_psi_13_b5,
        &hf_nas_5gs_allow_pdu_ses_sts_psi_12_b4,
        &hf_nas_5gs_allow_pdu_ses_sts_psi_11_b3,
        &hf_nas_5gs_allow_pdu_ses_sts_psi_10_b2,
        &hf_nas_5gs_allow_pdu_ses_sts_psi_9_b1,
        &hf_nas_5gs_allow_pdu_ses_sts_psi_8_b0,
        NULL
    };

    curr_offset = offset;
    proto_tree_add_bitmask_list(tree, tvb, curr_offset, 1, psi_0_7_flags, ENC_BIG_ENDIAN);
    curr_offset++;

    proto_tree_add_bitmask_list(tree, tvb, curr_offset, 1, psi_8_15_flags, ENC_BIG_ENDIAN);
    curr_offset++;

    EXTRANEOUS_DATA_CHECK(len, curr_offset - offset, pinfo, &ei_nas_5gs_extraneous_data);

    return (curr_offset - offset);
}
/*
 * 9.11.3.14    Authentication failure parameter
 */
/* See subclause 10.5.3.2.2 in 3GPP TS 24.008 */
/*
 *  9.11.3.15    Authentication parameter AUTN
 */
/* See subclause 10.5.3.1 in 3GPP TS 24.008 [8].*/

/*
 *   9.11.3.16    Authentication parameter RAND
 */

/* See subclause 10.5.3.1 in 3GPP TS 24.008 [8]. */

/*
 * 9.11.3.17    Authentication response parameter
 */
/* See subclause 9.9.3.4 in 3GPP TS 24.301 [15].*/

/*
 *   9.11.3.18    Configuration update indication
 */
static guint16
de_nas_5gs_mm_conf_upd_ind(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{

    static const int * flags[] = {
        &hf_nas_5gs_spare_b3,
        &hf_nas_5gs_spare_b2,
        &hf_nas_5gs_mm_conf_upd_ind_red_b1,
        &hf_nas_5gs_mm_conf_upd_ind_ack_b0,
        NULL
    };


    proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags, ENC_BIG_ENDIAN);

    return 1;
}

/*
 *   9.11.3.19    Daylight saving time
 */
/* See subclause 10.5.3.12 in 3GPP TS 24.008 */

/*
 *   9.11.3.20    De-registration type
 */
static const true_false_string nas_5gs_mm_switch_off_tfs = {
    "Switch off",
    "Normal de-registration"
};

static const true_false_string nas_5gs_mm_re_reg_req_tfs = {
    "re-registration required",
    "re-registration not required"
};

static const value_string nas_5gs_mm_acc_type_vals[] = {
    { 0x1, "3GPP access"},
    { 0x2, "Non-3GPP access"},
    { 0x3, "3GPP access and non-3GPP access"},
    {   0, NULL }
};

static guint16
de_nas_5gs_mm_de_reg_type(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{
    /* Switch off   Re-registration required    Access type */
    proto_tree_add_item(tree, hf_nas_5gs_mm_switch_off, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_nas_5gs_mm_re_reg_req, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_nas_5gs_mm_acc_type, tvb, offset, 1, ENC_BIG_ENDIAN);

    return 1;
}

/* 9.11.3.21    Void*/
/* 9.11.3.22    Void*/

/*
 * 9.11.3.23    Emergency number list
 */
/* See subclause 10.5.3.13 in 3GPP TS 24.008 */

/*
 *   9.11.3.24    EPS NAS message container
 */
static guint16
de_nas_5gs_mm_eps_nas_msg_cont(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    /* an EPS NAS message as specified in 3GPP TS 24.301 */
    if (nas_eps_handle) {
        col_append_str(pinfo->cinfo, COL_PROTOCOL, "/");
        col_set_fence(pinfo->cinfo, COL_PROTOCOL);
        call_dissector(nas_eps_handle, tvb_new_subset_length(tvb, offset, len), pinfo, tree);
    }

    return len;
}

/*
 * 9.11.3.25    EPS NAS security algorithms
 */
/* See subclause 9.9.3.23 in 3GPP TS 24.301 */

/*
 * 9.11.3.26    Extended emergency number list
 */
/* See subclause 9.9.3.37A in 3GPP TS 24.301 */

/* 9.11.3.27    Void*/

/*
 *   9.11.3.28    IMEISV request
 */
/* See subclause 10.5.5.10 in 3GPP TS 24.008 */

/*
 *   9.11.3.29    LADN indication
 */

static guint16
de_nas_5gs_mm_ladn_indic(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    proto_tree *sub_tree;
    proto_item *item;
    int i = 1;
    guint32 length;
    guint32 curr_offset;

    curr_offset = offset;

    while ((curr_offset - offset) < len) {
        sub_tree = proto_tree_add_subtree_format(tree, tvb, curr_offset, 2, ett_nas_5gs_mm_ladn_indic, &item, "LADN DNN value %u", i);
        /*LADN DNN value is coded as the length and value part of DNN information element as specified in subclause 9.11.2.1A starting with the second octet*/
        proto_tree_add_item_ret_uint(sub_tree, hf_nas_5gs_mm_length, tvb, curr_offset, 1, ENC_BIG_ENDIAN, &length);
        curr_offset++;
        curr_offset += de_nas_5gs_cmn_dnn(tvb, sub_tree, pinfo, curr_offset, length, NULL, 0);
        proto_item_set_len(item, length + 1);

        i++;

    }

    return len;
}

/*
 *   9.11.3.30    LADN information
 */

static guint16
de_nas_5gs_mm_ladn_inf(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    proto_tree *sub_tree;
    proto_item *item;
    int i = 1;
    guint32 length;
    guint32 curr_offset;

    curr_offset = offset;

    while ((curr_offset - offset) < len) {
        sub_tree = proto_tree_add_subtree_format(tree, tvb, curr_offset, 2, ett_nas_5gs_mm_ladn_indic, &item, "LADN %u", i);
        /* DNN value (octet 5 to octet m):
         * LADN DNN value is coded as the length and value part of DNN information element as specified in
         * subclause 9.11.2.1A starting with the second octet
         */
        proto_tree_add_item_ret_uint(sub_tree, hf_nas_5gs_mm_length, tvb, curr_offset, 1, ENC_BIG_ENDIAN, &length);
        curr_offset++;
        curr_offset += de_nas_5gs_cmn_dnn(tvb, sub_tree, pinfo, curr_offset, length, NULL, 0);
        /* 5GS tracking area identity list (octet m+1 to octet a):
         * 5GS tracking area identity list field is coded as the length and the value part of the
         * 5GS Tracking area identity list information element as specified in subclause 9.11.3.9
         * starting with the second octet
         */
        proto_tree_add_item_ret_uint(sub_tree, hf_nas_5gs_mm_length, tvb, curr_offset, 1, ENC_BIG_ENDIAN, &length);
        curr_offset++;
        curr_offset += de_nas_5gs_mm_5gs_ta_id_list(tvb, sub_tree, pinfo, curr_offset, length, NULL, 0);

        proto_item_set_len(item, curr_offset - offset);

        i++;

    }

    return len;
}

/*
 *   9.11.3.31    MICO indication
 */
static const true_false_string tfs_nas_5gs_raai = {
    "all PLMN registration area allocated",
    "all PLMN registration area not allocated"
};


static guint16
de_nas_5gs_mm_mico_ind(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{
    static const int * flags[] = {
        &hf_nas_5gs_spare_b3,
        &hf_nas_5gs_spare_b2,
        &hf_nas_5gs_spare_b1,
        &hf_nas_5gs_mm_raai_b0,
        NULL
    };

    proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags, ENC_BIG_ENDIAN);

    return 1;
}

static const true_false_string nas_5gs_mm_tsc_tfs = {
    "Mapped security context (for KSIASME)",
    "Native security context (for KSIAMF)"
};

/*
 *   9.11.3.32    NAS key set identifier
 */
static guint16
de_nas_5gs_mm_nas_key_set_id(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{
    static const int * flags[] = {
        &hf_nas_5gs_mm_tsc,
        &hf_nas_5gs_mm_nas_key_set_id,
        NULL
    };

    /* NAS key set identifier IEI   TSC     NAS key set identifier */
    proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags, ENC_BIG_ENDIAN);

    return 1;
}


/*
 *   9.11.3.33    NAS message container
 */
static guint16
de_nas_5gs_mm_nas_msg_cont(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    /* The purpose of the NAS message container IE is to encapsulate a plain 5GS NAS message. */
    /* a NAS message without NAS security heade */

    dissect_nas_5gs(tvb_new_subset_length(tvb, offset, len), pinfo, tree, NULL);

    return len;
}

/*
 *   9.11.3.34    NAS security algorithms
 */

static const value_string nas_5gs_mm_type_of_ip_algo_vals[] = {
    { 0x0, "5G-IA0 (null integrity protection algorithm)"},
    { 0x1, "128-5G-IA1"},
    { 0x2, "128-5G-IA2"},
    { 0x3, "128-5G-IA3"},
    { 0x4, "5G-IA4"},
    { 0x5, "5G-IA5"},
    { 0x6, "5G-IA6"},
    { 0x7, "5G-IA7"},
    {   0, NULL }
};

static const value_string nas_5gs_mm_type_of_enc_algo_vals[] = {
    { 0x0, "5G-EA0 (null ciphering algorithm)"},
    { 0x1, "128-5G-EA1"},
    { 0x2, "128-5G-EA2"},
    { 0x3, "128-5G-EA3"},
    { 0x4, "5G-EA4"},
    { 0x5, "5G-EA5"},
    { 0x6, "5G-EA6"},
    { 0x7, "5G-EA7"},
    {   0, NULL }
};

static guint16
de_nas_5gs_mm_nas_sec_algo(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{

    static const int * flags[] = {
        &hf_nas_5gs_mm_nas_sec_algo_enc,
        &hf_nas_5gs_mm_nas_sec_algo_ip,
        NULL
    };

    proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags, ENC_BIG_ENDIAN);

    return 1;
}


/*
 *   9.11.3.35    Network name
 */
/* See subclause 10.5.3.5a in 3GPP TS 24.008 */


/*
 *   9.11.3.36    Network slicing indication
 */

static const true_false_string nas_5gs_mm_dcni_tfs = {
    "Requested NSSAI created from default configured NSSAI",
    "Requested NSSAI not created from default configured NSSAI"
};

static guint16
de_nas_5gs_mm_nw_slicing_ind(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    static const int * flags[] = {
        &hf_nas_5gs_spare_b3,
        &hf_nas_5gs_spare_b2,
        &hf_nas_5gs_mm_dcni,
        &hf_nas_5gs_mm_nssci,
        NULL
    };

    proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags, ENC_BIG_ENDIAN);

    return len;
}

/*
 *   9.11.3.37    NSSAI
 */
static guint16
de_nas_5gs_mm_nssai(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    proto_tree *sub_tree;
    proto_item *item;
    int i = 1;
    guint32 length;
    guint32 curr_offset;

    curr_offset = offset;

    while ((curr_offset - offset) < len) {
        sub_tree = proto_tree_add_subtree_format(tree, tvb, curr_offset, 2, ett_nas_5gs_mm_nssai, &item, "S-NSSAI %u", i);

        proto_tree_add_item_ret_uint(sub_tree, hf_nas_5gs_mm_length, tvb, curr_offset, 1, ENC_BIG_ENDIAN, &length);
        curr_offset++;
        curr_offset += de_nas_5gs_cmn_s_nssai(tvb, sub_tree, pinfo, curr_offset, length, NULL, 0);
        proto_item_set_len(item, length + 1);
        i++;

    }

    return len;
}

/*
 *   9.11.3.37A    NSSAI inclusion mode
 */


static const value_string nas_5gs_mm_nssai_inc_mode_vals[] = {
    { 0x00, "A" },
    { 0x01, "B" },
    { 0x02, "C" },
    { 0x03, "D" },
    {    0, NULL } };

static guint16
de_nas_5gs_mm_nssai_inc_mode(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    static const int * flags[] = {
        &hf_nas_5gs_spare_b3,
        &hf_nas_5gs_spare_b2,
        &hf_nas_5gs_mm_nssai_inc_mode,
        NULL
    };

    proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags, ENC_BIG_ENDIAN);

    return len;
}

/*
 *   9.11.3.38    Operator-defined access category definitions
 */
static guint16
de_nas_5gs_mm_op_def_acc_cat_def(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    proto_tree *sub_tree;
    proto_item *item;
    int i = 1;
    guint32 length;
    guint32 curr_offset;

    curr_offset = offset;

    while ((curr_offset - offset) < len) {
        sub_tree = proto_tree_add_subtree_format(tree, tvb, curr_offset, 2, ett_nas_5gs_mm_nssai, &item, "Operator-defined access category definition  %u", i);

        proto_tree_add_item_ret_uint(sub_tree, hf_nas_5gs_mm_length, tvb, curr_offset, 1, ENC_BIG_ENDIAN, &length);
        curr_offset++;
        /* Precedence value */
        proto_tree_add_item(sub_tree, hf_nas_5gs_mm_precedence, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
        curr_offset++;

        /* PSAC    0 Spare    0 Spare    Operator-defined access category number */
        /* Length of criteria */
        /* Criteria */
        /* 0 Spare    0 Spare    0 Spare    Standardized access category */
        proto_tree_add_expert(sub_tree, pinfo, &ei_nas_5gs_not_diss, tvb, curr_offset, length - 1);
        curr_offset += length;
        proto_item_set_len(item, length + 1);
        i++;

    }

    return len;
}

/*
 *   9.11.3.39    Payload container
 */
static guint16
de_nas_5gs_mm_pld_cont(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    struct nas5gs_private_data *nas5gs_data = nas5gs_get_private_data(pinfo);

    switch (nas5gs_data->payload_container_type) {
    case 1: /* N1 SM information */
        dissect_nas_5gs_common(tvb_new_subset_length(tvb, offset, len), pinfo, tree, 0, NULL);
        break;
    default:
        proto_tree_add_item(tree, hf_nas_5gs_mm_pld_cont, tvb, offset, len, ENC_NA);
        break;
    }

    return len;
}

/*
 *   9.11.3.40    Payload container type
 */
static const value_string nas_5gs_mm_pld_cont_type_vals[] = {
    { 0x01, "N1 SM information" },
    { 0x02, "SMS" },
    { 0x03, "LTE Positioning Protocol (LPP) message container" },
    { 0x04, "SOR transparent container" },
    { 0x05, "UE policy container" },
    { 0x06, "UE parameters update transparent container" },
    { 0x07, "Multiple payloads" },
    {    0, NULL } };

static guint16
de_nas_5gs_mm_pld_cont_type(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{
    struct nas5gs_private_data *nas5gs_data = nas5gs_get_private_data(pinfo);

    proto_tree_add_item_ret_uint(tree, hf_nas_5gs_mm_pld_cont_type, tvb, offset, 1, ENC_BIG_ENDIAN, &nas5gs_data->payload_container_type);

    return 1;
}

/*
 *   9.11.3.41    PDU session identity 2
 */
static guint16
de_nas_5gs_mm_pdu_ses_id_2(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{
    proto_tree_add_item(tree, hf_nas_5gs_pdu_session_id, tvb, offset, 1, ENC_BIG_ENDIAN);

    return 1;
}

/*
 *   9.11.3.42    PDU session reactivation result
 */


static true_false_string tfs_nas_5gs_pdu_ses_rect_res_psi = {
    "1",
    "0"
};

static guint16
de_nas_5gs_mm_pdu_ses_react_res(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    int curr_offset;

    static const int * psi_0_7_flags[] = {
        &hf_nas_5gs_pdu_ses_rect_res_psi_7_b7,
        &hf_nas_5gs_pdu_ses_rect_res_psi_6_b6,
        &hf_nas_5gs_pdu_ses_rect_res_psi_5_b5,
        &hf_nas_5gs_pdu_ses_rect_res_psi_4_b4,
        &hf_nas_5gs_pdu_ses_rect_res_psi_3_b3,
        &hf_nas_5gs_pdu_ses_rect_res_psi_2_b2,
        &hf_nas_5gs_pdu_ses_rect_res_psi_1_b1,
        &hf_nas_5gs_pdu_ses_rect_res_psi_0_b0,
        NULL
         };

        static const int * psi_8_15_flags[] = {
        &hf_nas_5gs_pdu_ses_rect_res_psi_15_b7,
        &hf_nas_5gs_pdu_ses_rect_res_psi_14_b6,
        &hf_nas_5gs_pdu_ses_rect_res_psi_13_b5,
        &hf_nas_5gs_pdu_ses_rect_res_psi_12_b4,
        &hf_nas_5gs_pdu_ses_rect_res_psi_11_b3,
        &hf_nas_5gs_pdu_ses_rect_res_psi_10_b2,
        &hf_nas_5gs_pdu_ses_rect_res_psi_9_b1,
        &hf_nas_5gs_pdu_ses_rect_res_psi_8_b0,
        NULL
         };

    curr_offset = offset;
    proto_tree_add_bitmask_list(tree, tvb, curr_offset, 1, psi_0_7_flags, ENC_BIG_ENDIAN);
    curr_offset++;

    proto_tree_add_bitmask_list(tree, tvb, curr_offset, 1, psi_8_15_flags, ENC_BIG_ENDIAN);
    curr_offset++;

    EXTRANEOUS_DATA_CHECK(len, curr_offset - offset, pinfo, &ei_nas_5gs_extraneous_data);

    return (curr_offset - offset);

}

/*
 *   9.11.3.43    PDU session reactivation result error cause
 */
static guint16
de_nas_5gs_mm_pdu_ses_react_res_err_c(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset = offset;

    /*Partial service area list*/
    while ((curr_offset - offset) < len) {
        proto_tree_add_item(tree, hf_nas_5gs_pdu_session_id, tvb, offset, 1, ENC_BIG_ENDIAN);
        curr_offset++;
        proto_tree_add_item(tree, hf_nas_5gs_mm_5gmm_cause, tvb, offset, 1, ENC_BIG_ENDIAN);
        curr_offset++;
    }

    return len;
}

/*
*   9.11.3.44    PDU session status
*/

static true_false_string tfs_nas_5gs_pdu_ses_sts_psi = {
    "Not PDU SESSION INACTIVE",
    "PDU SESSION INACTIVE"
};

static guint16
de_nas_5gs_mm_pdu_ses_status(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    int curr_offset;

    static const int * psi_0_7_flags[] = {
        &hf_nas_5gs_pdu_ses_sts_psi_7_b7,
        &hf_nas_5gs_pdu_ses_sts_psi_6_b6,
        &hf_nas_5gs_pdu_ses_sts_psi_5_b5,
        &hf_nas_5gs_pdu_ses_sts_psi_4_b4,
        &hf_nas_5gs_pdu_ses_sts_psi_3_b3,
        &hf_nas_5gs_pdu_ses_sts_psi_2_b2,
        &hf_nas_5gs_pdu_ses_sts_psi_1_b1,
        &hf_nas_5gs_pdu_ses_sts_psi_0_b0,
        NULL
    };

    static const int * psi_8_15_flags[] = {
        &hf_nas_5gs_pdu_ses_sts_psi_15_b7,
        &hf_nas_5gs_pdu_ses_sts_psi_14_b6,
        &hf_nas_5gs_pdu_ses_sts_psi_13_b5,
        &hf_nas_5gs_pdu_ses_sts_psi_12_b4,
        &hf_nas_5gs_pdu_ses_sts_psi_11_b3,
        &hf_nas_5gs_pdu_ses_sts_psi_10_b2,
        &hf_nas_5gs_pdu_ses_sts_psi_9_b1,
        &hf_nas_5gs_pdu_ses_sts_psi_8_b0,
        NULL
    };

    curr_offset = offset;
    proto_tree_add_bitmask_list(tree, tvb, curr_offset, 1, psi_0_7_flags, ENC_BIG_ENDIAN);
    curr_offset++;

    proto_tree_add_bitmask_list(tree, tvb, curr_offset, 1, psi_8_15_flags, ENC_BIG_ENDIAN);
    curr_offset++;

    EXTRANEOUS_DATA_CHECK(len, curr_offset - offset, pinfo, &ei_nas_5gs_extraneous_data);

    return (curr_offset - offset);

}


/*
 *   9.11.3.45    PLMN list
 */
/* See subclause 10.5.1.13 in 3GPP TS 24.008 */

/*
 *   9.11.3.46    Rejected NSSAI
 */
static guint16
de_nas_5gs_mm_rej_nssai(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    proto_tree_add_expert(tree, pinfo, &ei_nas_5gs_ie_not_dis, tvb, offset, len);

    return len;
}

/*
*     9.11.3.47    Request type
*/
static const value_string nas_5gs_mm_req_type_vals[] = {
    { 0x01, "Initial request" },
    { 0x02, "Existing PDU session" },
    { 0x03, "Initial emergency request" },
    { 0x04, "Existing emergency PDU session" },
    { 0x05, "Modification request" },
    { 0x07, "Reserved" },
    { 0, NULL } };

static guint16
de_nas_5gs_mm_req_type(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{

    proto_tree_add_item(tree, hf_nas_5gs_mm_req_type, tvb, offset, 1, ENC_BIG_ENDIAN);

    return 1;
}


/*
 *    9.11.3.48    S1 UE network capability
 */
/* See subclause 9.9.3.34 in 3GPP TS 24.301 */

/*
 *   9.11.3.48A    S1 UE security capability
 */
/*See subclause 9.9.3.36 in 3GPP TS 24.301 */

/*
 *     9.11.3.49    Service area list
 */
static true_false_string tfs_nas_5gs_sal_al_t = {
    "TAIs in the list are in the non-allowed area",
    "TAIs in the list are in the allowed area"
};

static const value_string nas_5gs_mm_sal_t_li_values[] = {
    { 0x00, "list of TACs belonging to one PLMN, with non-consecutive TAC values" },
    { 0x01, "list of TACs belonging to one PLMN, with consecutive TAC values" },
    { 0x02, "list of TAIs belonging to different PLMNs" },
    { 0x03, "All TAIs belonging to the PLMN are in the allowed area" },
    { 0, NULL } };


static guint16
de_nas_5gs_mm_sal(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    proto_tree *sub_tree;
    proto_item *item;

    static const int * flags_sal[] = {
        &hf_nas_5gs_mm_sal_al_t,
        &hf_nas_5gs_mm_sal_t_li,
        &hf_nas_5gs_mm_sal_num_e,
        NULL
    };

    guint num_par_sal = 1;
    guint32 curr_offset = offset;
    guint32 start_offset;
    guint8 sal_head, sal_t_li, sal_num_e;

    /*Partial service area list*/
    while ((curr_offset - offset) < len) {
        start_offset = curr_offset;
        sub_tree = proto_tree_add_subtree_format(tree, tvb, curr_offset, -1, ett_nas_5gs_mm_part_sal, &item, "Partial service area list  %u", num_par_sal);
        /*Head of Partial service area list*/
        /* Allowed type    Type of list    Number of elements    octet 1 */
        sal_head = tvb_get_guint8(tvb, curr_offset);
        sal_t_li = (sal_head & 0x60) >> 5;
        sal_num_e = (sal_head & 0x1f) + 1;
        proto_tree_add_bitmask_list(sub_tree, tvb, curr_offset, 1, flags_sal, ENC_BIG_ENDIAN);
        curr_offset++;
        switch (sal_t_li) {
        case 0:
            /*octet 2  MCC digit2  MCC digit1*/
            /*octet 3  MNC digit3  MCC digit3*/
            /*octet 4  MNC digit2  MNC digit1*/
            dissect_e212_mcc_mnc(tvb, pinfo, sub_tree, curr_offset, E212_NONE, FALSE);
            curr_offset += 3;
            while (sal_num_e > 0) {
                proto_tree_add_item(sub_tree, hf_nas_5gs_tac, tvb, curr_offset, 3, ENC_BIG_ENDIAN);
                curr_offset += 3;
                sal_num_e--;
            }
            break;
        case 1:
            /*octet 2  MCC digit2  MCC digit1*/
            /*octet 3  MNC digit3  MCC digit3*/
            /*octet 4  MNC digit2  MNC digit1*/
            dissect_e212_mcc_mnc(tvb, pinfo, sub_tree, curr_offset, E212_NONE, FALSE);
            curr_offset += 3;

            /*octet 5  TAC 1*/
            proto_tree_add_item(sub_tree, hf_nas_5gs_tac, tvb, curr_offset, 3, ENC_BIG_ENDIAN);
            curr_offset+=3;
            break;
        case 2:
            while (sal_num_e > 0) {
                /*octet 2  MCC digit2  MCC digit1*/
                /*octet 3  MNC digit3  MCC digit3*/
                /*octet 4  MNC digit2  MNC digit1*/
                dissect_e212_mcc_mnc(tvb, pinfo, sub_tree, curr_offset, E212_NONE, FALSE);
                curr_offset += 3;

                /*octet 5  TAC 1*/
                proto_tree_add_item(sub_tree, hf_nas_5gs_tac, tvb, curr_offset, 3, ENC_BIG_ENDIAN);
                curr_offset += 3;

                sal_num_e--;
            }
            break;
        case 3:
            dissect_e212_mcc_mnc(tvb, pinfo, sub_tree, curr_offset, E212_NONE, FALSE);
            curr_offset += 3;
            break;
        default:
            proto_tree_add_expert(sub_tree, pinfo, &ei_nas_5gs_unknown_value, tvb, curr_offset, len - 1);
        }



        /*calculate the length of IE?*/
        proto_item_set_len(item, curr_offset - start_offset);
        /*calculate the number of Partial service area list*/
        num_par_sal++;
    }

    return len;
}


/*
 *     9.11.3.50    Service type
 */

/* Used inline as H1 (Upper nibble)*/
static const value_string nas_5gs_mm_serv_type_vals[] = {
    { 0x00, "Signalling" },
    { 0x01, "Data" },
    { 0x02, "Mobile terminated services" },
    { 0x03, "Emergency services" },
    { 0x04, "Emergency services fallback" },
    { 0x05, "High priority access" },
    {    0, NULL } };

/*
 *   9.11.3.50A    SMS indication
 */
static guint16
de_nas_5gs_mm_sms_ind(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{

    static const int * flags[] = {
        &hf_nas_5gs_spare_b3,
        &hf_nas_5gs_spare_b2,
        &hf_nas_5gs_spare_b1,
        &hf_nas_5gs_mm_sms_indic_sai,
        NULL
    };

    proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags, ENC_BIG_ENDIAN);

    return len;
}

/*
 *    9.11.3.51    SOR transparent container
 */
static true_false_string tfs_nas_5gs_list_type = {
    "PLMN ID and access technology list",
    "Secured packet"
};

static true_false_string tfs_nas_5gs_list_ind = {
    "List of preferred PLMN/access technology combinations is provided",
    "No list of preferred PLMN/access technology combinations is provided"
};

static true_false_string tfs_nas_5gs_sor_data_type = {
    "Carries acknowledgement of successful reception of the steering of roaming information",
    "Carries steering of roaming information"
};

static guint16
de_nas_5gs_mm_sor_trasp_cont(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    /* Layout differs depending on SOR data type*/
    static const int * flags_dt0[] = {
    &hf_nas_5gs_spare_b7,
    &hf_nas_5gs_spare_b6,
    &hf_nas_5gs_spare_b5,
    &hf_nas_5gs_spare_b4,
    &hf_nas_5gs_sor_hdr0_ack,
    &hf_nas_5gs_sor_hdr0_list_type,
    &hf_nas_5gs_sor_hdr0_list_ind,
    &hf_nas_5gs_sor_hdr0_sor_data_type,
    NULL
    };

    static const int * flags_dt1[] = {
    &hf_nas_5gs_spare_b7,
    &hf_nas_5gs_spare_b6,
    &hf_nas_5gs_spare_b5,
    &hf_nas_5gs_spare_b4,
    &hf_nas_5gs_spare_b3,
    &hf_nas_5gs_spare_b2,
    &hf_nas_5gs_spare_b1,
    &hf_nas_5gs_sor_hdr0_sor_data_type,
    NULL
    };
    /* 3GPP TS 31.102 [22] subclause 4.2.5 */
    static const int * flags_acces_tech_1[] = {
    &hf_nas_5gs_acces_tech_o1_b7,
    &hf_nas_5gs_acces_tech_o1_b6,
    &hf_nas_5gs_acces_tech_o1_b5,
    &hf_nas_5gs_acces_tech_o1_b4,
    &hf_nas_5gs_acces_tech_o1_b3,
    &hf_nas_5gs_rfu_b2,
    &hf_nas_5gs_rfu_b1,
    &hf_nas_5gs_rfu_b0,
    NULL
    };

    static const int * flags_acces_tech_2[] = {
    &hf_nas_5gs_acces_tech_o2_b7,
    &hf_nas_5gs_acces_tech_o2_b6,
    &hf_nas_5gs_acces_tech_o2_b5,
    &hf_nas_5gs_acces_tech_o2_b4,
    &hf_nas_5gs_acces_tech_o2_b3,
    &hf_nas_5gs_acces_tech_o2_b2,
    &hf_nas_5gs_rfu_b1,
    &hf_nas_5gs_rfu_b0,
    NULL
    };

    proto_tree *sub_tree;

    guint8 oct, data_type, list_type;
    guint32 curr_offset = offset;
    int i = 1;

    oct = tvb_get_guint8(tvb, offset);
    data_type = oct & 0x01;
    if (data_type == 0) {
        /* SOR header    octet 4*/
        proto_tree_add_bitmask_list(tree, tvb, curr_offset, 1, flags_dt0, ENC_BIG_ENDIAN);
        curr_offset++;
        list_type = (oct & 0x4) >> 2;
        /* SOR-MAC-IAUSF    octet 5-20 */
        proto_tree_add_item(tree, hf_nas_5gs_sor_mac_iausf, tvb, curr_offset, 16, ENC_NA);
        curr_offset += 16;
        /* CounterSOR    octet 21-22 */
        proto_tree_add_item(tree, hf_nas_5gs_counter_sor, tvb, curr_offset, 2, ENC_BIG_ENDIAN);
        curr_offset += 2;
        if (list_type == 0) {
            /* Secured packet    octet 23* - 2048* */
            proto_tree_add_item(tree, hf_nas_5gs_sor_sec_pkt, tvb, curr_offset, len - 19, ENC_NA);
            curr_offset = curr_offset + (len - 19);
        } else {
            /* PLMN ID and access technology list    octet 23*-102* */
            while ((curr_offset - offset) < len) {
                sub_tree = proto_tree_add_subtree_format(tree, tvb, curr_offset, -1, ett_nas_5gs_mm_sor, NULL, "List item %u", i);
                /* The PLMN ID and access technology list consists of PLMN ID and access technology identifier
                 * and are coded as specified in 3GPP TS 31.102 [22] subclause 4.2.5
                 *  PLMN
                 * Contents:
                 * - Mobile Country Code (MCC) followed by the Mobile Network Code (MNC).
                 * Coding:
                 * - according to TS 24.008 [9].
                 */
                /* PLMN ID 1    octet 23*- 25* */
                curr_offset = dissect_e212_mcc_mnc(tvb, pinfo, sub_tree, curr_offset, E212_NONE, FALSE);
                curr_offset += 3;
                /* access technology identifier 1    octet 26*- 27* */
                proto_tree_add_bitmask_list(tree, tvb, curr_offset, 1, flags_acces_tech_1, ENC_BIG_ENDIAN);
                curr_offset++;
                proto_tree_add_bitmask_list(tree, tvb, curr_offset, 1, flags_acces_tech_2, ENC_BIG_ENDIAN);
                curr_offset++;
                i++;
            }
        }

    } else {
        /* SOR header    octet 4*/
        proto_tree_add_bitmask_list(tree, tvb, curr_offset, 1, flags_dt1, ENC_BIG_ENDIAN);
        curr_offset++;
        /* SOR-MAC-IUE    octet 5 - 20*/
        proto_tree_add_item(tree, hf_nas_5gs_sor_mac_iue, tvb, curr_offset, 16, ENC_NA);
        curr_offset+=16;
    }

    EXTRANEOUS_DATA_CHECK(len, curr_offset - offset, pinfo, &ei_nas_5gs_extraneous_data);

    return (curr_offset - offset);

}

/*
 *     9.11.3.52    Time zone
 */
/* See subclause 10.5.3.8 in 3GPP TS 24.008 */

/*
 *     9.11.3.53    Time zone and time
 */
/* See subclause 10.5.3.9 in 3GPP TS 24.00*/

/*
 *   9.11.3.53A    UE parameters update transparent container
 */
static guint16
de_nas_5gs_mm_ue_par_upd_trasnsp_cont(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    proto_tree_add_expert(tree, pinfo, &ei_nas_5gs_ie_not_dis, tvb, offset, len);

    return len;
}


/*
 *     9.11.3.54    UE security capability
 */

static guint16
de_nas_5gs_mm_ue_sec_cap(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    guint32 curr_offset;

    static const int * oct3_flags[] = {
        &hf_nas_5gs_mm_5g_ea0,
        &hf_nas_5gs_mm_128_5g_ea1,
        &hf_nas_5gs_mm_128_5g_ea2,
        &hf_nas_5gs_mm_128_5g_ea3,
        &hf_nas_5gs_mm_5g_ea4,
        &hf_nas_5gs_mm_5g_ea5,
        &hf_nas_5gs_mm_5g_ea6,
        &hf_nas_5gs_mm_5g_ea7,
        NULL
    };

    static const int * oct4_flags[] = {
        &hf_nas_5gs_mm_5g_ia0,
        &hf_nas_5gs_mm_5g_128_ia1,
        &hf_nas_5gs_mm_5g_128_ia2,
        &hf_nas_5gs_mm_5g_128_ia3,
        &hf_nas_5gs_mm_5g_ia4,
        &hf_nas_5gs_mm_5g_ia5,
        &hf_nas_5gs_mm_5g_ia6,
        &hf_nas_5gs_mm_5g_ia7,
        NULL
    };

    static const int * oct5_flags[] = {
        &hf_nas_5gs_mm_eea0,
        &hf_nas_5gs_mm_128eea1,
        &hf_nas_5gs_mm_128eea2,
        &hf_nas_5gs_mm_eea3,
        &hf_nas_5gs_mm_eea4,
        &hf_nas_5gs_mm_eea5,
        &hf_nas_5gs_mm_eea6,
        &hf_nas_5gs_mm_eea7,
        NULL
    };

    static const int * oct6_flags[] = {
        &hf_nas_5gs_mm_eia0,
        &hf_nas_5gs_mm_128eia1,
        &hf_nas_5gs_mm_128eia2,
        &hf_nas_5gs_mm_eia3,
        &hf_nas_5gs_mm_eia4,
        &hf_nas_5gs_mm_eia5,
        &hf_nas_5gs_mm_eia6,
        &hf_nas_5gs_mm_eia7,
        NULL
    };

    curr_offset = offset;


    /* 5G-EA0    128-5G-EA1    128-5G-EA2    128-5G-EA3    5G-EA4    5G-EA5    5G-EA6    5G-EA7    octet 3 */
    proto_tree_add_bitmask_list(tree, tvb, curr_offset, 1, oct3_flags, ENC_NA);
    curr_offset++;

    /* 5G-IA0    128-5G-IA1    128-5G-IA2    128-5G-IA3    5G-IA4    5G-IA5    5G-IA6    5G-IA7 octet 4 */
    proto_tree_add_bitmask_list(tree, tvb, curr_offset, 1, oct4_flags, ENC_NA);
    curr_offset++;

    if (len == 2) {
        return len;
    }

    /* EEA0    128-EEA1    128-EEA2    128-EEA3    EEA4    EEA5    EEA6    EEA7 octet 5 */
    proto_tree_add_bitmask_list(tree, tvb, curr_offset, 1, oct5_flags, ENC_NA);
    curr_offset++;

    /* EIA0    128-EIA1    128-EIA2    128-EIA3    EIA4    EIA5    EIA6    EIA7 octet 6 */
    proto_tree_add_bitmask_list(tree, tvb, curr_offset, 1, oct6_flags, ENC_NA);


    return len;
}

/*
 * 9.11.3.55    UE's usage setting
 */
static true_false_string tfs_nas_5gs_mm_ue_usage_setting = {
    "Data centric",
    "Voice centric"
};

static guint16
de_nas_5gs_mm_ue_usage_set(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    static const int * flags[] = {
        &hf_nas_5gs_spare_b3,
        &hf_nas_5gs_spare_b2,
        &hf_nas_5gs_spare_b1,
        &hf_nas_5gs_mm_ue_usage_setting,
        NULL
    };

    proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags, ENC_BIG_ENDIAN);

    return len;
}

/*
 *    9.11.3.56    UE status
 */

static true_false_string tfs_nas_5gs_mm_n1_mod = {
    "UE is in 5GMM-REGISTERED state",
    "UE is not in 5GMM-REGISTERED state"
};

static true_false_string tfs_nas_5gs_mm_s1_mod = {
    "UE is in EMM-REGISTERED state",
    "UE is not in EMM-REGISTERED state"
};



static guint16
de_nas_5gs_mm_ue_status(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{
    static const int * flags[] = {
        &hf_nas_5gs_spare_b7,
        &hf_nas_5gs_spare_b6,
        &hf_nas_5gs_spare_b5,
        &hf_nas_5gs_spare_b4,
        &hf_nas_5gs_spare_b3,
        &hf_nas_5gs_spare_b2,
        &hf_nas_5gs_mm_n1_mode_reg_b1,
        &hf_nas_5gs_mm_s1_mode_reg_b0,
        NULL
    };

    /* 0 Spare    0 Spare    0 Spare    0 Spare    0 Spare    0 Spare    0 Spare    S1 mode reg */
    proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags, ENC_BIG_ENDIAN);

    return 1;
}

/*
 * 9.11.3.57    Uplink data status
 */

static true_false_string tfs_nas_5gs_ul_data_sts_psi = {
    "uplink data are pending",
    "no uplink data are pending"
};

static guint16
de_nas_5gs_mm_ul_data_status(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    int curr_offset;

    static const int * psi_0_7_flags[] = {
        &hf_nas_5gs_ul_data_sts_psi_7_b7,
        &hf_nas_5gs_ul_data_sts_psi_6_b6,
        &hf_nas_5gs_ul_data_sts_psi_5_b5,
        &hf_nas_5gs_ul_data_sts_psi_4_b4,
        &hf_nas_5gs_ul_data_sts_psi_3_b3,
        &hf_nas_5gs_ul_data_sts_psi_2_b2,
        &hf_nas_5gs_ul_data_sts_psi_1_b1,
        &hf_nas_5gs_ul_data_sts_psi_0_b0,
        NULL
    };

    static const int * psi_8_15_flags[] = {
        &hf_nas_5gs_ul_data_sts_psi_15_b7,
        &hf_nas_5gs_ul_data_sts_psi_14_b6,
        &hf_nas_5gs_ul_data_sts_psi_13_b5,
        &hf_nas_5gs_ul_data_sts_psi_12_b4,
        &hf_nas_5gs_ul_data_sts_psi_11_b3,
        &hf_nas_5gs_ul_data_sts_psi_10_b2,
        &hf_nas_5gs_ul_data_sts_psi_9_b1,
        &hf_nas_5gs_ul_data_sts_psi_8_b0,
        NULL
    };

    curr_offset = offset;
    proto_tree_add_bitmask_list(tree, tvb, curr_offset, 1, psi_0_7_flags, ENC_BIG_ENDIAN);
    curr_offset++;

    proto_tree_add_bitmask_list(tree, tvb, curr_offset, 1, psi_8_15_flags, ENC_BIG_ENDIAN);
    curr_offset++;

    EXTRANEOUS_DATA_CHECK(len, curr_offset - offset, pinfo, &ei_nas_5gs_extraneous_data);

    return (curr_offset - offset);
}

/*
 * 9.11.4    5GS session management (5GSM) information elements
 */

 /*
 *       9.11.4.1    5GSM capability
 */

static guint16
de_nas_5gs_sm_5gsm_cap(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{

    static const int * flags[] = {
        &hf_nas_5gs_spare_b7,
        &hf_nas_5gs_spare_b6,
        &hf_nas_5gs_spare_b5,
        &hf_nas_5gs_spare_b4,
        &hf_nas_5gs_spare_b3,
        &hf_nas_5gs_spare_b2,
        &hf_nas_5gs_spare_b1,
        &hf_nas_5gs_sm_rqos_b0,
        NULL
    };


    proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags, ENC_BIG_ENDIAN);

    return 1;
}

/*
 *     9.11.4.2    5GSM cause
 */

const value_string nas_5gs_sm_cause_vals[] = {
    { 0x1a, "Insufficient resources" },
    { 0x1b, "Missing or unknown DNN" },
    { 0x1c, "Unknown PDU session type" },
    { 0x1d, "User authentication or authorization failed" },
    { 0x1f, "Request rejected, unspecified" },
    { 0x22, "Service option temporarily out of order" },
    { 0x23, "PTI already in use" },
    { 0x24, "Regular deactivation" },
    { 0x26, "Out of LADN service area" },
    { 0x27, "Reactivation requested" },
    { 0x2b, "Invalid PDU session identity" },
    { 0x2c, "Semantic errors in packet filter(s)" },
    { 0x2d, "Syntactical error in packet filter(s)" },
    { 0x2f, "PTI mismatch" },
    { 0x32, "PDU session type Ipv4 only allowed" },
    { 0x33, "PDU session type Ipv6 only allowed" },
    { 0x36, "PDU session does not exist" },
    { 0x43, "Insufficient resources for specific slice and DNN" },
    { 0x44, "Not supported SSC mode" },
    { 0x45, "Insufficient resources for specific slice" },
    { 0x46, "Missing or unknown DNN in a slice" },
    { 0x51, "Invalid PTI value" },
    { 0x52, "Maximum data rate per UE for user-plane integrity protection is too low" },
    { 0x53, "Semantic error in the QoS operation" },
    { 0x54, "Syntactical error in the QoS operation" },
    { 0x5f, "Semantically incorrect message" },
    { 0x60, "Invalid mandatory information" },
    { 0x61, "Message type non - existent or not implemented" },
    { 0x62, "Message type not compatible with the protocol state" },
    { 0x63, "Information element non - existent or not implemented" },
    { 0x64, "Conditional IE error" },
    { 0x65, "Message not compatible with the protocol state" },
    { 0x6f, "Protocol error, unspecified" },
    { 0,    NULL }
};

static guint16
de_nas_5gs_sm_5gsm_cause(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{
    guint32 cause;

    proto_tree_add_item_ret_uint(tree, hf_nas_5gs_sm_5gsm_cause, tvb, offset, 1, ENC_BIG_ENDIAN, &cause);

    col_append_fstr(pinfo->cinfo, COL_INFO, " (%s)",
        val_to_str_const(cause, nas_5gs_sm_cause_vals, "Unknown"));


    return 1;
}

/*
 * 9.11.4.3 Always-on PDU session indication
 */

/*
 * 9.11.4.4 Always-on PDU session requested
 */

/*
 * 9.11.4.5    Allowed SSC mode
 */

static guint16
de_nas_5gs_sm_5gsm_allowed_ssc_mode(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{

    static const int * flags[] = {
        &hf_nas_5gs_spare_b3,
        &hf_nas_5gs_sm_all_ssc_mode_b2,
        &hf_nas_5gs_sm_all_ssc_mode_b1,
        &hf_nas_5gs_sm_all_ssc_mode_b0,
        NULL
    };


    proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags, ENC_BIG_ENDIAN);

    return 1;
}

/*
 *     9.11.4.6    Extended protocol configuration options
 */
/* See subclause 10.5.6.3A in 3GPP TS 24.008 */

/*
 * 9.11.4.7 Integrity protection maximum data rate
 */

/*
 *     9.11.4.8 Mapped EPS bearer contexts
 */
static const value_string nas_5gs_sm_mapd_eps_b_cont_opt_code_vals[] = {
    { 0x0,  "Reserved" },
    { 0x01, "Create new EPS bearer" },
    { 0x02, "Delete existing EPS bearer" },
    { 0x03, "Modify existing EPS bearer" },
    { 0,    NULL }
};

static const value_string nas_5gs_sm_mapd_eps_b_cont_DEB_vals[] = {
    { 0x0,  "the EPS bearer is not the default EPS bearer." },
    { 0x01, "the EPS bearer is the default EPS bearer" },
    { 0,    NULL }
};

static const value_string nas_5gs_sm_mapd_eps_b_cont_E_vals[] = {
    { 0x0,  "parameters list is not included" },
    { 0x01, "parameters list is included" },
    { 0,    NULL }
};

static const value_string nas_5gs_sm_mapd_eps_b_cont_E_Modify_vals[] = {
    { 0x0,  "previously provided parameters list extension" },
    { 0x01, "previously provided parameters list replacement" },
    { 0,    NULL }
};

static const value_string nas_5gs_sm_mapd_eps_b_cont_param_id_vals[] = {
    { 0x01, "Mapped EPS QoS parameters" },
    { 0x02, "Mapped extended EPS QoS parameters" },
    { 0x03, "Traffic flow template" },
    { 0x04, "APN-AMBR" },
    { 0x05, "extended APN-AMBR" },
    { 0,    NULL }
};

static guint16
de_nas_5gs_sm_mapped_eps_b_cont(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{

    guint32 curr_offset;
    proto_tree * sub_tree, *sub_tree1;
    guint32 num_cont, length, opt_code, num_eps_parms, param_id;
    proto_item * item;
    guint i, curr_len;

    curr_len = len;
    curr_offset = offset;
    num_cont = 1;

    static const int * mapd_eps_b_cont_flags[] = {
        &hf_nas_5gs_sm_mapd_eps_b_cont_opt_code,
        &hf_nas_5gs_sm_mapd_eps_b_cont_DEB,
        &hf_nas_5gs_sm_mapd_eps_b_cont_E,
        &hf_nas_5gs_sm_mapd_eps_b_cont_num_eps_parms,
        NULL
     };

    static const int * mapd_eps_b_cont_flags_modify[] = {
        &hf_nas_5gs_sm_mapd_eps_b_cont_opt_code,
        &hf_nas_5gs_sm_mapd_eps_b_cont_DEB,
        &hf_nas_5gs_sm_mapd_eps_b_cont_E_mod,
        &hf_nas_5gs_sm_mapd_eps_b_cont_num_eps_parms,
        NULL
    };

    /* The IE contains a number of Mapped EPS bearer context */
    while ((curr_offset - offset) < len) {
        /* Figure 9.11.4.5.2: Mapped EPS bearer context */
        sub_tree = proto_tree_add_subtree_format(tree, tvb, curr_offset, -1, ett_nas_5gs_sm_mapd_eps_b_cont, &item,
            "Mapped EPS bearer context %u", num_cont);

        /* EPS bearer identity */
        proto_tree_add_item(sub_tree, hf_nas_5gs_sm_mapd_eps_b_cont_id, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
        curr_offset++;
        curr_len--;

        /* Length of Mapped EPS bearer context*/
        proto_tree_add_item_ret_uint(sub_tree, hf_nas_5gs_sm_length, tvb, curr_offset, 2, ENC_BIG_ENDIAN, &length);
        curr_offset += 2;
        curr_len -= 2;

        /*     8   7      6    5   4  3  2  1          */
        /* operation code | DEB |  E | number of EPS params     */
        proto_item_set_len(item, length + 3);

        num_eps_parms = tvb_get_guint8(tvb, curr_offset);

        opt_code = num_eps_parms & 0xc0;
        num_eps_parms = num_eps_parms & 0x0f;

        /* operation code = 3 Modify existing EPS bearer */
        if (opt_code == 3) {
            proto_tree_add_bitmask_list(sub_tree, tvb, curr_offset, 1, mapd_eps_b_cont_flags_modify, ENC_BIG_ENDIAN);

        } else {
            proto_tree_add_bitmask_list(sub_tree, tvb, curr_offset, 1, mapd_eps_b_cont_flags, ENC_BIG_ENDIAN);

        }
        curr_offset++;
        curr_len--;
        i = 1;

        /* EPS parameters list */
        while (num_eps_parms > 0) {

            sub_tree1 = proto_tree_add_subtree_format(sub_tree, tvb, curr_offset, -1, ett_nas_5gs_sm_mapd_eps_b_cont_params_list, &item,
                "EPS parameter %u", i);

            /* EPS parameter identifier */
            proto_tree_add_item_ret_uint(sub_tree1, hf_nas_5gs_sm_mapd_eps_b_cont_param_id, tvb, curr_offset, 1, ENC_BIG_ENDIAN, &param_id);
            proto_item_append_text(item, " - %s", val_to_str_const(param_id, nas_5gs_sm_mapd_eps_b_cont_param_id_vals, "Unknown"));
            curr_offset++;
            curr_len--;

            /*length of the EPS parameter contents field */
            proto_tree_add_item_ret_uint(sub_tree1, hf_nas_5gs_sm_length, tvb, curr_offset, 1, ENC_BIG_ENDIAN, &length);
            curr_offset++;
            curr_len--;

            proto_item_set_len(item, length + 3);
            /*content of the EPS parameter contents field */
            switch (param_id) {
            case 1:
                /* 01H (Mapped EPS QoS parameters) */
                de_esm_qos(tvb, sub_tree1, pinfo, curr_offset, length, NULL, 0);
                break;
            case 2:
                /* 02H (Mapped extended EPS QoS parameters) */
                de_esm_ext_eps_qos(tvb, sub_tree1, pinfo, curr_offset, length, NULL, 0);
                break;
            case 3:
                /* 03H (Traffic flow template)*/
                de_sm_tflow_temp(tvb, sub_tree1, pinfo, curr_offset, length, NULL, 0);
                break;
            case 4:
                /* 04H (APN-AMBR) */
                de_esm_apn_aggr_max_br(tvb, sub_tree1, pinfo, curr_offset, length, NULL, 0);
                break;
            case 5:
                /* 05H (extended APN-AMBR). */
                de_esm_ext_apn_agr_max_br(tvb, sub_tree1, pinfo, curr_offset, length, NULL, 0);
                break;
            default:
                proto_tree_add_item(sub_tree1, hf_nas_5gs_sm_mapd_eps_b_cont_eps_param_cont, tvb, curr_offset, length, ENC_NA);
                break;
            }
            curr_offset +=length;
            curr_len -= length;
            i++;
            num_eps_parms--;
        }

        num_cont++;
    }

    return len;


}

/*
 *     9.11.4.9    Maximum number of supported packet filters
 */
static guint16
de_nas_5gs_sm_max_num_sup_pkt_flt(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    proto_tree_add_expert(tree, pinfo, &ei_nas_5gs_ie_not_dis, tvb, offset, len);

    return len;
}

/*
 *     9.11.4.10    PDU address
 */

static const value_string nas_5gs_sm_pdu_ses_type_vals[] = {
    { 0x1, "IPv4" },
    { 0x2, "IPv6" },
    { 0x3, "IPv4v6" },
    { 0,    NULL }
};


static guint16
de_nas_5gs_sm_pdu_address(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    proto_item *ti;
    guint32 value;

    /* 0 Spare    0 Spare    0 Spare    0 Spare    PDU session type value */
    ti = proto_tree_add_item_ret_uint(tree, hf_nas_5gs_sm_pdu_ses_type, tvb, offset, 1, ENC_BIG_ENDIAN, &value);
    offset++;

    /* PDU address information */
    switch (value) {
    case 1:
        /* IPv4 */
        proto_tree_add_item(tree, hf_nas_5gs_sm_pdu_addr_inf_ipv4, tvb, offset, 4, ENC_BIG_ENDIAN);
        break;
    case 2:
        /* If the PDU session type value indicates IPv6, the PDU address information in octet 4 to octet 11
         * contains an interface identifier for the IPv6 link local address.
         */
        proto_tree_add_item(tree, hf_nas_5gs_sm_pdu_addr_inf_ipv6, tvb, offset, 8, ENC_NA);
        break;
    case 3:
        /* If the PDU session type value indicates IPv4v6, the PDU address information in octet 4 to octet 11
         * contains an interface identifier for the IPv6 link local address and in octet 12 to octet 15
         * contains an IPv4 address.
         */
        proto_tree_add_item(tree, hf_nas_5gs_sm_pdu_addr_inf_ipv6, tvb, offset, 8, ENC_NA);
        offset += 8;
        proto_tree_add_item(tree, hf_nas_5gs_sm_pdu_addr_inf_ipv4, tvb, offset, 4, ENC_BIG_ENDIAN);
        break;
    default:
        expert_add_info(pinfo, ti, &ei_nas_5gs_unknown_value);
        break;
    }
    return len;
}

/*
 *     9.11.4.11    PDU session type
 */
static const value_string nas_5gs_pdu_session_type_values[] = {
    { 0x1, "IPv4" },
    { 0x2, "Ipv6" },
    { 0x3, "Ipv4v6" },
    { 0x4, "Unstructured" },
    { 0x5, "Ethernet" },
    { 0, NULL }
 };


static guint16
de_nas_5gs_sm_pdu_session_type(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{

    proto_tree_add_item(tree, hf_nas_5gs_sm_pdu_session_type, tvb, offset, 1, ENC_BIG_ENDIAN);

    return 1;
}

/*
 * 9.11.4.12 QoS flow descriptions
 */

static const value_string nas_5gs_sm_qos_des_flow_opt_code_vals[] = {
    { 0x00, "Reserved" },
    { 0x01, "Create new QoS flow description" },
    { 0x02, "Delete existing QoS flow description" },
    { 0x03, "Modify existing QoS flow description" },
    { 0,    NULL }
};

static const value_string nas_5gs_sm_param_id_values[] = {
    { 0x01, "5QI" },
    { 0x02, "GFBR uplink" },
    { 0x03, "GFBR downlink" },
    { 0x04, "MFBR uplink" },
    { 0x05, "MFBR downlink" },
    { 0x06, "Averaging window" },
    { 0x07, "EPS bearer identity" },
    { 0, NULL }
};

static guint16
de_nas_5gs_sm_qos_flow_des(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{

    proto_tree *sub_tree, *sub_tree2;
    proto_item *item;
    int i = 1, j = 1;
    guint32 param_len, param_id;
    guint32 curr_offset, start_offset;
    guint8 num_param;
    guint32 unit, mult, ambr_val;
    const char *unit_str;

    static const int * param_flags[] = {
        &hf_nas_5gs_sm_e,
        &hf_nas_5gs_sm_nof_params,
        NULL
    };

    curr_offset = offset;

    while ((curr_offset - offset) < len) {

        /* QoS fow description */
        sub_tree = proto_tree_add_subtree_format(tree, tvb, curr_offset, -1, ett_nas_5gs_sm_qos_params, &item, "QoS flow description %u", i);

        /* 0 0 QFI */
        proto_tree_add_item(sub_tree, hf_nas_5gs_sm_qfi, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
        curr_offset += 1;

        /* Operation code */
        proto_tree_add_item(sub_tree, hf_nas_5gs_sm_qos_des_flow_opt_code, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
        curr_offset++;

        /* 0 Spare    E    Number of parameters */
        j = 1;
        num_param = tvb_get_guint8(tvb, curr_offset);
        num_param = num_param & 0x3f;
        proto_tree_add_bitmask_list(sub_tree, tvb, curr_offset, 1, param_flags, ENC_BIG_ENDIAN);
        curr_offset++;


        while (num_param > 0) {
            /* Parameter list */
            sub_tree2 = proto_tree_add_subtree_format(sub_tree, tvb, curr_offset, -1, ett_nas_5gs_sm_qos_rules, &item, "Parameter %u", j);

            start_offset = curr_offset;

            /* Parameter identifier */
            proto_tree_add_item_ret_uint(sub_tree2, hf_nas_5gs_sm_param_id, tvb, curr_offset, 1, ENC_BIG_ENDIAN, &param_id);
            proto_item_append_text(item, " - %s", val_to_str_const(param_id, nas_5gs_sm_param_id_values, "Unknown"));
            curr_offset++;
            /* Length of parameter contents */
            proto_tree_add_item_ret_uint(sub_tree2, hf_nas_5gs_sm_param_len, tvb, curr_offset, 1, ENC_BIG_ENDIAN, &param_len);
            curr_offset++;

            /*parameter content*/
            switch (param_id) {
                /* 01H (5QI)*/
            case 0x01:
                proto_tree_add_item(sub_tree2, hf_nas_5gs_sm_pal_cont, tvb, curr_offset, param_len, ENC_BIG_ENDIAN);
                curr_offset += param_len;
                break;
                /* 02H (GFBR uplink); 04H (MFBR uplink);*/
            case 0x02:
            case 0x04:
                /* Unit for Session-AMBR for uplink */
                proto_tree_add_item_ret_uint(sub_tree2, hf_nas_5gs_sm_unit_for_session_ambr_ul, tvb, curr_offset, 1, ENC_BIG_ENDIAN, &unit);
                curr_offset++;
                /* Session-AMBR for downlink */
                mult = get_ext_ambr_unit(unit, &unit_str);
                ambr_val = tvb_get_ntohs(tvb, curr_offset);
                proto_tree_add_uint_format_value(sub_tree2, hf_nas_5gs_sm_session_ambr_ul, tvb, curr_offset, param_len - 1,
                    ambr_val, "%u %s (%u)", ambr_val * mult, unit_str, ambr_val);
                curr_offset += (param_len - 1);
                break;
                /* 03H (GFBR downlink); 05H (MFBR downlink);*/
            case 0x03:
            case 0x05:
                /* Unit for Session-AMBR for uplink */
                proto_tree_add_item_ret_uint(sub_tree2, hf_nas_5gs_sm_unit_for_session_ambr_dl, tvb, curr_offset, 1, ENC_BIG_ENDIAN, &unit);
                curr_offset++;
                /* Session-AMBR for downlink*/
                mult = get_ext_ambr_unit(unit, &unit_str);
                ambr_val = tvb_get_ntohs(tvb, curr_offset);
                proto_tree_add_uint_format_value(sub_tree2, hf_nas_5gs_sm_session_ambr_dl, tvb, curr_offset, param_len - 1,
                    ambr_val, "%u %s (%u)", ambr_val * mult, unit_str, ambr_val);
                curr_offset += (param_len - 1);
                break;
            default:
                proto_tree_add_item(sub_tree2, hf_nas_5gs_sm_pal_cont, tvb, curr_offset, param_len, ENC_BIG_ENDIAN);
                curr_offset += param_len;
                break;
            }
            num_param--;
            j++;
            proto_item_set_len(item, curr_offset - start_offset);
        }

        i++;
    }

    return len;
}
/*
 *     9.11.4.13    QoS rules
 */

static true_false_string tfs_nas_5gs_sm_dqr = {
    "The QoS rule is the default QoS rule",
    "The QoS rule is not the default QoS rule"
};

static const value_string nas_5gs_rule_operation_code_values[] = {
    { 0x0, "Reserved" },
    { 0x1, "Create new QoS rule" },
    { 0x2, "Delete existing QoS rule" },
    { 0x3, "Modify existing QoS rule and add packet filters" },
    { 0x4, "Modify existing QoS rule and replace packet filters" },
    { 0x5, "Modify existing QoS rule and delete packet filters" },
    { 0x6, "Modify existing QoS rule without modifying packet filters" },
    { 0x7, "Reserved" },
    { 0, NULL }
 };

static const value_string nas_5gs_sm_pf_type_values[] = {
    { 0x01, "Match-all type" },
    { 0x10, "IPv4 remote address type" },
    { 0x11, "IPv4 local address type" },
    { 0x21, "IPv6 remote address/prefix length type" },
    { 0x23, "IPv6 local address/prefix length type" },
    { 0x30, "Protocol identifier/Next header type" },
    { 0x40, "Single local port type" },
    { 0x41, "Local port range type" },
    { 0x50, "Single remote port type" },
    { 0x51, "Remote port range type" },
    { 0x60, "Security parameter index type" },
    { 0x70, "Type of service/Traffic class type" },
    { 0x80, "Flow label type" },
    { 0x81, "Destination MAC address type" },
    { 0x82, "Source MAC address type" },
    { 0x83, "802.1Q C-TAG VID type" },
    { 0x84, "802.1Q S-TAG VID type" },
    { 0x85, "802.1Q C-TAG PCP/DEI type" },
    { 0x86, "802.1Q S-TAG PCP/DEI type" },
    { 0x87, "Ethertype type" },
    { 0, NULL }
 };

static const value_string nas_5gs_sm_pkt_flt_dir_values[] = {
    { 0x00, "Reserved" },
    { 0x01, "Downlink only" },
    { 0x02, "Uplink only" },
    { 0x03, "Bidirectional" },
    { 0, NULL }
 };

static const value_string nas_5gs_rule_param_cont[] = {
    { 0x0, "Reserved" },
    { 0x01, "5QI 1" },
    { 0x02, "5QI 2" },
    { 0x03, "5QI 3" },
    { 0x04, "5QI 4" },
    { 0x05, "5QI 5" },
    { 0x06, "5QI 6" },
    { 0x07, "5QI 7" },
    { 0x08, "5QI 8" },
    { 0x09, "5QI 9" },
    { 0, NULL }
};

guint16
de_nas_5gs_sm_qos_rules(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{

    proto_tree *sub_tree, *sub_tree2;
    proto_item *item;
    int i = 1, j = 1;
    guint32 qos_rule_id, pf_len, pf_type;
    guint32 length, curr_offset, start_offset;
    guint8 num_pkt_flt, rop;

    static const int * pkt_flt_flags[] = {
        &hf_nas_5gs_sm_rop,
        &hf_nas_5gs_sm_dqr,
        &hf_nas_5gs_sm_nof_pkt_filters,
        NULL
    };

    curr_offset = offset;

    while ((curr_offset - offset) < len) {

        /* QoS Rule */
        sub_tree = proto_tree_add_subtree_format(tree, tvb, curr_offset, -1, ett_nas_5gs_sm_qos_rules, &item, "QoS rule %u", i);

        /* QoS rule identifier Octet 4*/
        proto_tree_add_item_ret_uint(sub_tree, hf_nas_5gs_sm_qos_rule_id, tvb, curr_offset, 1, ENC_BIG_ENDIAN, &qos_rule_id);
        curr_offset += 1;
        /* Length of QoS rule Octet 5 - 6*/
        proto_tree_add_item_ret_uint(sub_tree, hf_nas_5gs_sm_length, tvb, curr_offset, 2, ENC_BIG_ENDIAN, &length);
        curr_offset += 2;

        proto_item_set_len(item, length + 3);

        /* Rule operation code    DQR bit    Number of packet filters */
        num_pkt_flt = tvb_get_guint8(tvb, curr_offset);
        rop = num_pkt_flt >> 5;
        num_pkt_flt = num_pkt_flt & 0x0f;
        proto_tree_add_bitmask_list(sub_tree, tvb, curr_offset, 1, pkt_flt_flags, ENC_BIG_ENDIAN);
        curr_offset++;

        /* For the "delete existing QoS rule" operation and for the "modify existing QoS rule without modifying packet filters"
         * operation, the number of packet filters shall be coded as 0.
         */
        if ((rop == 0) || (rop == 7)) {
            /* Reserved */
            proto_tree_add_expert(tree, pinfo, &ei_nas_5gs_unknown_value, tvb, curr_offset, length - 1);
            i++;
            curr_offset += (length - 1);
            continue;
        }
        if ((rop == 2) || (rop == 6)) {
            if (num_pkt_flt != 0) {
                proto_tree_add_expert(tree, pinfo, &ei_nas_5gs_num_pkt_flt, tvb, curr_offset, length - 1);
                i++;
                curr_offset += (length - 1);
                continue;
            }
        }

        /* Packet filter list */
        while (num_pkt_flt > 0) {
            sub_tree2 = proto_tree_add_subtree_format(sub_tree, tvb, curr_offset, -1, ett_nas_5gs_sm_qos_rules, &item, "Packet filter %u", j);
            start_offset = curr_offset;
            if (rop == 5) {
                /* modify existing QoS rule and delete packet filters */
                /* 0    0    0    0    Packet filter identifier x*/
                proto_tree_add_item(sub_tree2, hf_nas_5gs_sm_pkt_flt_id, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                curr_offset++;
            } else {
                /* "create new QoS rule", or "modify existing QoS rule and add packet filters"
                 * or "modify existing QoS rule and replace packet filters"
                 */
                 /* 0    0    Packet filter direction 1    Packet filter identifier 1*/
                proto_tree_add_item(sub_tree2, hf_nas_5gs_sm_pkt_flt_dir, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                proto_tree_add_item(sub_tree2, hf_nas_5gs_sm_pkt_flt_id, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                curr_offset++;
                /* Length of packet filter contents */
                proto_tree_add_item_ret_uint(sub_tree2, hf_nas_5gs_sm_pf_len, tvb, curr_offset, 1, ENC_BIG_ENDIAN, &pf_len);
                curr_offset++;
                /* Packet filter contents */
                /* Each packet filter component shall be encoded as a sequence of a one octet packet filter component type identifier
                 * and a fixed length packet filter component value field.
                 * The packet filter component type identifier shall be transmitted first.
                 */
                proto_tree_add_item_ret_uint(sub_tree2, hf_nas_5gs_sm_pf_type, tvb, curr_offset, 1, ENC_BIG_ENDIAN, &pf_type);
                curr_offset++;
                switch (pf_type) {
                case 1:
                    /* Match-all type
                     * . If the "match-all type" packet filter component is present in the packet filter, no other packet filter
                     * component shall be present in the packet filter and the length of the packet filter contents field shall
                     * be set to one.
                     */
                    curr_offset += pf_len;
                    break;
                case 16:
                    /* For "IPv4 remote address type", the packet filter component value field shall be encoded as a sequence
                     * of a four octet IPv4 address field and a four octet IPv4 address mask field.
                     * The IPv4 address field shall be transmitted first.
                     */
                    proto_tree_add_item(sub_tree2, hf_nas_5gs_sm_pdu_addr_inf_ipv4, tvb, curr_offset, 4, ENC_BIG_ENDIAN);
                    curr_offset += 4;
                    proto_tree_add_item(sub_tree2, hf_nas_5gs_addr_mask_ipv4, tvb, curr_offset, 4, ENC_BIG_ENDIAN);
                    curr_offset += 4;
                    proto_tree_add_item(sub_tree2, hf_nas_5gs_protocol_identifier_or_next_hd, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                    curr_offset += 1;
                    proto_tree_add_item(sub_tree2, hf_nas_5gs_protocol_identifier_or_next_hd_val, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
                    curr_offset++;
                    break;
                default:
                    proto_tree_add_expert(sub_tree2, pinfo, &ei_nas_5gs_not_diss, tvb, curr_offset, pf_len);
                    curr_offset += pf_len;
                    break;
                }
            }
            num_pkt_flt--;
            j++;
            proto_item_set_len(item, curr_offset - start_offset);

        }
        /* QoS rule precedence (octet z+1)
         * For the "delete existing QoS rule" operation, the QoS rule precedence value field shall not be included.
         * For the "create new QoS rule" operation, the QoS rule precedence value field shall be included.
         */
        if (qos_rule_id != 2) { /* Delete existing QoS rule */
            proto_tree_add_item(sub_tree, hf_nas_5gs_sm_qos_rule_precedence, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
            curr_offset++;
        }
        /* QoS flow identifier (QFI) (bits 6 to 1 of octet z+2)
         * For the "delete existing QoS rule" operation, the QoS flow identifier value field shall not be included.
         * For the "create new QoS rule" operation, the QoS flow identifier value field shall be included.
         */
        /* Segregation bit (bit 7 of octet z+2) */
        proto_tree_add_item(sub_tree, hf_nas_5gs_sm_qfi, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
        curr_offset += 1;

        i++;
    }

    return len;
}

/*
 *      9.11.4.14    Session-AMBR
 */

static const value_string nas_5gs_sm_unit_for_session_ambr_values[] = {
    { 0x00, "value is not used" },
    { 0x01, "value is incremented in multiples of 1 Kbps" },
    { 0x02, "value is incremented in multiples of 4 Kbps" },
    { 0x03, "value is incremented in multiples of 16 Kbps" },
    { 0x04, "value is incremented in multiples of 64 Kbps" },
    { 0x05, "value is incremented in multiples of 256 kbps" },
    { 0x06, "value is incremented in multiples of 1 Mbps" },
    { 0x07, "value is incremented in multiples of 4 Mbps" },
    { 0x08, "value is incremented in multiples of 16 Mbps" },
    { 0x09, "value is incremented in multiples of 64 Mbps" },
    { 0x0a, "value is incremented in multiples of 256 Mbps" },
    { 0x0b, "value is incremented in multiples of 1 Gbps" },
    { 0x0c, "value is incremented in multiples of 4 Gbps" },
    { 0x0d, "value is incremented in multiples of 16 Gbps" },
    { 0x0e, "value is incremented in multiples of 64 Gbps" },
    { 0x0f, "value is incremented in multiples of 256 Gbps" },
    { 0x10, "value is incremented in multiples of 1 Tbps" },
    { 0x11, "value is incremented in multiples of 4 Tbps" },
    { 0x12, "value is incremented in multiples of 16 Tbps" },
    { 0x13, "value is incremented in multiples of 64 Tbps" },
    { 0x14, "value is incremented in multiples of 256 Tbps" },
    { 0x15, "value is incremented in multiples of 1 Pbps" },
    { 0x16, "value is incremented in multiples of 4 Pbps" },
    { 0x17, "value is incremented in multiples of 16 Pbps" },
    { 0x18, "value is incremented in multiples of 64 Pbps" },
    { 0x19, "value is incremented in multiples of 256 Pbps" },
    { 0, NULL }
};


guint16
de_nas_5gs_sm_session_ambr(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    guint32 unit, mult, ambr_val;
    const char *unit_str;

    /* Unit for Session-AMBR for downlink */
    proto_tree_add_item_ret_uint(tree, hf_nas_5gs_sm_unit_for_session_ambr_dl, tvb, offset, 1, ENC_BIG_ENDIAN, &unit);
    offset++;

    /* Session-AMBR for downlink (octets 4 and 5) */
    mult = get_ext_ambr_unit(unit, &unit_str);
    ambr_val = tvb_get_ntohs(tvb, offset);
    proto_tree_add_uint_format_value(tree, hf_nas_5gs_sm_session_ambr_dl, tvb, offset, 2,
        ambr_val, "%u %s (%u)", ambr_val * mult, unit_str, ambr_val);
    offset += 2;

    proto_tree_add_item_ret_uint(tree, hf_nas_5gs_sm_unit_for_session_ambr_ul, tvb, offset, 1, ENC_NA, &unit);
    offset++;
    mult = get_ext_ambr_unit(unit, &unit_str);
    ambr_val = tvb_get_ntohs(tvb, offset);
    proto_tree_add_uint_format_value(tree, hf_nas_5gs_sm_session_ambr_ul, tvb, offset, 2,
        ambr_val, "%u %s (%u)", ambr_val * mult, unit_str, ambr_val);

    return len;
}

/*
 *      9.11.4.15    SM PDU DN request container
 */
/* The SM PDU DN request container contains a DN-specific identity of the UE in the network access identifier (NAI) format */
static guint16
de_nas_5gs_sm_pdu_dn_req_cont(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    proto_tree_add_expert(tree, pinfo, &ei_nas_5gs_ie_not_dis, tvb, offset, len);

    return len;
}

/*
 *      9.11.4.16    SSC mode
 */

static const value_string nas_5gs_sc_mode_values[] = {
    { 0x1, "SSC mode 1" },
    { 0x2, "SSC mode 2" },
    { 0x3, "SSC mode 3" },
    { 0, NULL }
 };


static guint16
de_nas_5gs_sm_ssc_mode(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len _U_,
    gchar *add_string _U_, int string_len _U_)
{

    proto_tree_add_item(tree, hf_nas_5gs_sm_sc_mode, tvb, offset, 1, ENC_BIG_ENDIAN);

    return 1;
}




/*
 *   9.10.2    Common information elements
 */

/* 9.10.2.1    Additional information*/

static guint16
de_nas_5gs_cmn_add_inf(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    proto_tree_add_expert(tree, pinfo, &ei_nas_5gs_ie_not_dis, tvb, offset, len);

    return len;
}

/*
 * 9.11.2.1A    DNN
 */

static guint16
de_nas_5gs_cmn_dnn(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{

    guint32     curr_offset;
    guint       curr_len;
    guint8     *str;
    proto_item *pi;

    curr_offset = offset;
    /* A DNN value field contains an APN as defined in 3GPP TS 23.003 */

    str = tvb_get_string_enc(wmem_packet_scope(), tvb, offset, len, ENC_ASCII | ENC_NA);

    curr_len = 0;
    while (curr_len < len)
    {
        guint step = str[curr_len];
        str[curr_len] = '.';
        curr_len += step + 1;
    }

    /* Highlight bytes including the first length byte */
    pi = proto_tree_add_string(tree, hf_nas_5gs_cmn_dnn, tvb, curr_offset, len, str + 1);
    if (len > 100) {
        expert_add_info(pinfo, pi, &ei_nas_5gs_dnn_too_long);
    }
    curr_offset += len;

    EXTRANEOUS_DATA_CHECK(len, curr_offset - offset, pinfo, &ei_nas_5gs_extraneous_data);

    return (curr_offset - offset);

}

/* 9.10.2.2    EAP message*/

static guint16
de_nas_5gs_cmn_eap_msg(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    /* EAP message as specified in IETF RFC 3748 */
    if (eap_handle) {
        col_append_str(pinfo->cinfo, COL_PROTOCOL, "/");
        col_set_fence(pinfo->cinfo, COL_PROTOCOL);
        call_dissector(eap_handle, tvb_new_subset_length(tvb, offset, len), pinfo, tree);
    }

    return len;
}

/* 9.10.2.3    GPRS timer */
/* See subclause 10.5.7.3 in 3GPP TS 24.008 */

/* 9.10.2.4    GPRS timer 2*/
/* See subclause 10.5.7.4 in 3GPP TS 24.008 */

/* 9.10.2.5    GPRS timer 3*/

/* 9.11.2.6     Intra N1 mode NAS transparent container*/
static guint16
de_nas_5gs_cmn_intra_n1_mode_nas_trans_cont(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    proto_tree_add_expert(tree, pinfo, &ei_nas_5gs_ie_not_dis, tvb, offset, len);

    return len;
}

/* 9.10.2.7     N1 mode to S1 mode NAS transparent containe */
static guint16
de_nas_5gs_cmn_n1_to_s1_mode_trans_cont(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    proto_tree_add_expert(tree, pinfo, &ei_nas_5gs_ie_not_dis, tvb, offset, len);

    return len;
}

/* 9.10.2.8    S-NSSAI */
guint16
de_nas_5gs_cmn_s_nssai(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    /* SST    octet 3
    * This field contains the 8 bit SST value. The coding of the SST value part is defined in 3GPP TS 23.003
    */
    proto_tree_add_item(tree, hf_nas_5gs_mm_sst, tvb, offset, 1, ENC_BIG_ENDIAN);
    if (len == 1) {
        return len;
    }
    offset += 1;
    /* SD    octet 4 - octet 6* */
    proto_tree_add_item(tree, hf_nas_5gs_mm_sd, tvb, offset, 3, ENC_BIG_ENDIAN);
    if (len == 4) {
        return len;
    }
    offset += 3;
    /* Mapped configured SST    octet 7* */
    proto_tree_add_item(tree, hf_nas_5gs_mm_mapped_conf_sst, tvb, offset, 1, ENC_BIG_ENDIAN);
    if (len == 5) {
        return len;
    }
    offset += 1;
    /* Mapped configured SD    octet 8 - octet 10* */
    proto_tree_add_item(tree, hf_nas_5gs_mm_mapped_conf_ssd, tvb, offset, 3, ENC_BIG_ENDIAN);

    return len;
}


/* 9.10.2.9    S1 mode to N1 mode NAS transparent container */
static guint16
de_nas_5gs_cmn_s1_to_n1_mode_trans_cont(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string _U_, int string_len _U_)
{
    proto_tree_add_expert(tree, pinfo, &ei_nas_5gs_ie_not_dis, tvb, offset, len);

    return len;
}
/*
 * Note this enum must be of the same size as the element decoding list
 */
typedef enum
{
    DE_NAS_5GS_CMN_ADD_INF,                      /* 9.10.2.1     Additional information*/
    DE_NAS_5GS_CMN_DNN,                          /* 9.10.2.1A    DNN*/
    DE_NAS_5GS_CMN_EAP_MESSAGE,                  /* 9.10.2.2     EAP message*/
    DE_NAS_5GS_CMN_GPRS_TIMER,                   /* 9.10.2.3     GPRS timer */
    DE_NAS_5GS_CMN_GPRS_TIMER2,                  /* 9.10.2.4     GPRS timer 2*/
    DE_NAS_5GS_CMN_GPRS_TIMER3,                  /* 9.10.2.5     GPRS timer 3*/
    DE_NAS_5GS_CMN_INTRA_N1_MODE_NAS_TRANS_CONT, /* 9.11.2.6     Intra N1 mode NAS transparent container*/
    DE_NAS_5GS_CMN_N1_TO_S1_MODE_TRANS_CONT,     /* 9.10.2.7     N1 mode to S1 mode NAS transparent containe */
    DE_NAS_5GS_CMN_S_NSSAI,                      /* 9.10.2.8     S-NSSAI */
    DE_NAS_5GS_CMN_S1_TO_N1_MODE_TRANS_CONT,     /* 9.10.2.9     S1 mode to N1 mode NAS transparent container */
    DE_NAS_5GS_COMMON_NONE                       /* NONE */
}
nas_5gs_common_elem_idx_t;

static const value_string nas_5gs_common_elem_strings[] = {
    { DE_NAS_5GS_CMN_ADD_INF,                       "Additional information" },                          /* 9.10.2.1     Additional information*/
    { DE_NAS_5GS_CMN_DNN,                           "DNN" },                                             /* 9.10.2.1A    DNN*/
    { DE_NAS_5GS_CMN_EAP_MESSAGE,                   "EAP message" },                                     /* 9.10.2.2     EAP message*/
    { DE_NAS_5GS_CMN_GPRS_TIMER,                    "GPRS timer" },                                      /* 9.10.2.3     GPRS timer*/
    { DE_NAS_5GS_CMN_GPRS_TIMER2,                   "GPRS timer 2" },                                    /* 9.10.2.4     GPRS timer 2*/
    { DE_NAS_5GS_CMN_GPRS_TIMER3,                   "GPRS timer 3" },                                    /* 9.10.2.5     GPRS timer 3*/
    { DE_NAS_5GS_CMN_INTRA_N1_MODE_NAS_TRANS_CONT,  "Intra N1 mode NAS transparent container" },         /* 9.11.2.6     Intra N1 mode NAS transparent container*/
    { DE_NAS_5GS_CMN_N1_TO_S1_MODE_TRANS_CONT,      "N1 mode to S1 mode NAS transparent container" },    /* 9.10.2.7     N1 mode to S1 mode NAS transparent container */
    { DE_NAS_5GS_CMN_S_NSSAI,                       "S-NSSAI" },                                         /* 9.10.2.8     S-NSSAI */
    { DE_NAS_5GS_CMN_S1_TO_N1_MODE_TRANS_CONT,      "S1 mode to N1 mode NAS transparent container" },    /* 9.10.2.9     S1 mode to N1 mode NAS transparent container */
    { 0, NULL }
};
value_string_ext nas_5gs_common_elem_strings_ext = VALUE_STRING_EXT_INIT(nas_5gs_common_elem_strings);

#define NUM_NAS_5GS_COMMON_ELEM (sizeof(nas_5gs_common_elem_strings)/sizeof(value_string))
gint ett_nas_5gs_common_elem[NUM_NAS_5GS_COMMON_ELEM];


guint16(*nas_5gs_common_elem_fcn[])(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string, int string_len) = {
        /*  9.10.2    Common information elements */
        de_nas_5gs_cmn_add_inf,                      /* 9.10.2.1     Additional information*/
        de_nas_5gs_cmn_dnn,                          /* 9.10.2.1A    DNN*/
        de_nas_5gs_cmn_eap_msg,                      /* 9.10.2.2     EAP message*/
        NULL,                                        /* 9.10.2.3     GPRS timer*/
        NULL,                                        /* 9.10.2.4     GPRS timer 2*/
        NULL,                                        /* 9.10.2.5     GPRS timer 3*/
        de_nas_5gs_cmn_intra_n1_mode_nas_trans_cont, /* 9.11.2.6     Intra N1 mode NAS transparent container*/
        de_nas_5gs_cmn_n1_to_s1_mode_trans_cont,     /* 9.10.2.7     N1 mode to S1 mode NAS transparent containe */
        de_nas_5gs_cmn_s_nssai,                      /* 9.10.2.8     S-NSSAI */
        de_nas_5gs_cmn_s1_to_n1_mode_trans_cont,     /* 9.10.2.9     S1 mode to N1 mode NAS transparent container */
        NULL,   /* NONE */
};



/*
 * 9.11.3    5GS mobility management (5GMM) information elements
 */
#if 0
typedef enum
{
    DE_NAS_5GS_MM_5GMM_CAP,                  /* 9.11.3.1     5GMM capability*/
    DE_NAS_5GS_MM_5GMM_CAUSE,                /* 9.11.3.2     5GMM cause*/
    DE_NAS_5GS_MM_5GS_DRX_PARAM,             /* 9.11.3.2A    5GS DRX parameters*/
    DE_NAS_5GS_MM_5GS_IDENTITY_TYPE,         /* 9.11.3.3     5GS identity type*/
    DE_NAS_5GS_MM_5GS_MOBILE_ID,             /* 9.11.3.4     5GS mobile identity*/
    DE_NAS_5GS_MM_5GS_NW_FEAT_SUP,           /* 9.11.3.5     5GS network feature support*/
    DE_NAS_5GS_MM_5GS_REG_RES,               /* 9.11.3.6     5GS registration result*/
    DE_NAS_5GS_MM_5GS_REG_TYPE,              /* 9.11.3.7     5GS registration type*/
    DE_NAS_5GS_MM_5GS_TA_ID,                 /* 9.11.3.8     5GS tracking area identity */
    DE_NAS_5GS_MM_5GS_TA_ID_LIST,            /* 9.11.3.9     5GS tracking area identity list */
    DE_NAS_5GS_MM_UPDATE_TYPE,               /* 9.11.3.9A    5GS update type */
    DE_NAS_5GS_MM_ABBA,                      /* 9.11.3.10    ABBA */
    DE_NAS_5GS_MM_ACCESS_TYPE,               /* 9.11.3.11    Access type */
    DE_NAS_5GS_MM_ADD_5G_SEC_INF,            /* 9.11.3.12    Additional 5G security information */
    DE_NAS_5GS_MM_ALLOW_PDU_SES_STS,         /* 9.11.3.13    Allowed PDU session status*/
    DE_NAS_5GS_MM_AUT_FAIL_PAR,              /* 9.11.3.14    Authentication failure parameter */
    DE_NAS_5GS_MM_AUT_PAR_AUTN,              /* 9.11.3.15    Authentication parameter AUTN*/
    DE_NAS_5GS_MM_AUT_PAR_RAND,              /* 9.11.3.16    Authentication parameter RAND*/
    DE_NAS_5GS_MM_AUT_RESP_PAR,              /* 9.11.3.17    Authentication response parameter */
    DE_NAS_5GS_MM_CONF_UPD_IND,              /* 9.11.3.18    Configuration update indication*/
    DE_NAS_5GS_MM_DLGT_SAVING_TIME,          /* 9.11.3.19    Daylight saving time*/
    DE_NAS_5GS_MM_DE_REG_TYPE,               /* 9.11.3.20    De-registration type*/
                                             /* 9.11.3.21    Void */
                                             /* 9.11.3.22    Void*/
    DE_NAS_5GS_MM_EMRG_NR_LIST,              /* 9.11.3.23    Emergency number list */
    DE_NAS_5GS_MM_EPS_NAS_MSG_CONT,          /* 9.11.3.24    EPS NAS message container */
    DE_NAS_5GS_MM_EPS_NAS_SEC_ALGO,          /* 9.11.3.25    EPS NAS security algorithms */
    DE_NAS_5GS_MM_EXT_EMERG_NUM_LIST,        /* 9.11.3.26    Extended emergency number list */
                                             /* 9.11.3.27    Void*/
    DE_NAS_5GS_MM_IMEISV_REQ,                /* 9.11.3.28    IMEISV request*/
    DE_NAS_5GS_MM_LADN_INDIC,                /* 9.11.3.29    LADN indication*/
    DE_NAS_5GS_MM_LADN_INF,                  /* 9.11.3.30    LADN information */
    DE_NAS_5GS_MM_MICO_IND,                  /* 9.11.3.31    MICO indication*/
    DE_NAS_5GS_MM_NAS_KEY_SET_ID,            /* 9.11.3.32    NAS key set identifier*/
    DE_NAS_5GS_MM_NAS_MSG_CONT,              /* 9.11.3.33    NAS message container*/
    DE_NAS_5GS_MM_NAS_SEC_ALGO,              /* 9.11.3.34    NAS security algorithms*/
    DE_NAS_5GS_MM_NW_NAME,                   /* 9.11.3.35    Network name*/
    DE_NAS_5GS_MM_NW_SLICING_IND,            /* 9.11.3.36    Network slicing indication */
    DE_NAS_5GS_MM_NSSAI,                     /* 9.11.3.37    NSSAI*/
    DE_NAS_5GS_MM_NSSAI_INC_MODE,            /* 9.11.3.37A   NSSAI inclusion mode */
    DE_NAS_5GS_MM_OP_DEF_ACC_CAT_DEF,        /* 9.11.3.38    Operator-defined access category definitions */
    DE_NAS_5GS_MM_PLD_CONT,                  /* 9.11.3.39    Payload container*/
    DE_NAS_5GS_MM_PLD_CONT_TYPE,             /* 9.11.3.40    Payload container type*/
    DE_NAS_5GS_MM_PDU_SES_ID_2,              /* 9.11.3.41    PDU session identity 2 */
    DE_NAS_5GS_MM_PDU_SES_REACT_RES,         /* 9.11.3.42    PDU session reactivation result*/
    DE_NAS_5GS_MM_PDU_SES_REACT_RES_ERR_C,   /* 9.11.3.43    PDU session reactivation result error cause */
    DE_NAS_5GS_MM_PDU_SES_STATUS,            /* 9.11.3.44    PDU session status */
    DE_NAS_5GS_MM_PLMN_LIST,                 /* 9.11.3.45    PLMN list*/
    DE_NAS_5GS_MM_REJ_NSSAI,                 /* 9.11.3.46    Rejected NSSAI*/
    DE_NAS_5GS_MM_REQ_TYPE,                  /* 9.11.3.47    Request type */
    DE_NAS_5GS_MM_S1_UE_NW_CAP,              /* 9.11.3.48    S1 UE network capability*/
    DE_NAS_5GS_MM_S1_UE_SEC_CAP,             /* 9.11.3.48A   S1 UE security capability*/
    DE_NAS_5GS_MM_SAL,                       /* 9.11.3.49    Service area list*/
    DE_NAS_5GS_MM_SERV_TYPE,                 /* 9.11.3.50    Service type,*/ /* Used inline Half octet IE*/
    DE_NAS_5GS_MM_SMS_IND,                   /* 9.11.3.50A   SMS indication */
    DE_NAS_5GS_MM_SOR_TRASP_CONT,            /* 9.11.3.51    SOR transparent container */
    DE_NAS_5GS_MM_TZ,                        /* 9.11.3.52    Time zone*/
    DE_NAS_5GS_MM_TZ_AND_T,                  /* 9.11.3.53    Time zone and time*/
    DE_NAS_5GS_MM_UE_PAR_UPD_TRASNSP_CONT,   /* 9.11.3.53A   UE parameters update transparent container */
    DE_NAS_5GS_MM_UE_SEC_CAP,                /* 9.11.3.54    UE security capability*/
    DE_NAS_5GS_MM_UE_USAGE_SET,              /* 9.11.3.55    UE's usage setting */
    DE_NAS_5GS_MM_UE_STATUS,                 /* 9.11.3.56    UE status */
    DE_NAS_5GS_MM_UL_DATA_STATUS,            /* 9.11.3.57    Uplink data status */
    DE_NAS_5GS_MM_NONE        /* NONE */
}
nas_5gs_mm_elem_idx_t;
#endif

static const value_string nas_5gs_mm_elem_strings[] = {
    { DE_NAS_5GS_MM_5GMM_CAP,                   "5GMM capability" },                    /* 9.11.3.1     5GMM capability*/
    { DE_NAS_5GS_MM_5GMM_CAUSE,                 "5GMM cause" },                         /* 9.11.3.2     5GMM cause*/
    { DE_NAS_5GS_MM_5GS_DRX_PARAM,              "5GS DRX parameters" },                 /* 9.11.3.2A    5GS DRX parameters*/
    { DE_NAS_5GS_MM_5GS_IDENTITY_TYPE,          "5GS identity type" },                  /* 9.11.3.3     5GS identity type*/
    { DE_NAS_5GS_MM_5GS_MOBILE_ID,              "5GS mobile identity" },                /* 9.11.3.4     5GS mobile identity*/
    { DE_NAS_5GS_MM_5GS_NW_FEAT_SUP,            "5GS network feature support" },        /* 9.11.3.5     5GS network feature support*/
    { DE_NAS_5GS_MM_5GS_REG_RES,                "5GS registration result" },            /* 9.11.3.6     5GS registration result*/
    { DE_NAS_5GS_MM_5GS_REG_TYPE,               "5GS registration type" },              /* 9.11.3.7     5GS registration type*/
    { DE_NAS_5GS_MM_5GS_TA_ID,                  "5GS tracking area identity" },         /* 9.11.3.8     5GS tracking area identity */
    { DE_NAS_5GS_MM_5GS_TA_ID_LIST,             "5GS tracking area identity list" },    /* 9.11.3.9     5GS tracking area identity list*/
    { DE_NAS_5GS_MM_UPDATE_TYPE,                "5GS update type" },                    /* 9.11.3.9A    5GS update type */
    { DE_NAS_5GS_MM_ABBA,                       "ABBA" },                               /* 9.11.3.10    ABBA */
    { DE_NAS_5GS_MM_ACCESS_TYPE,                "Access type" },                        /* 9.11.3.11    Access type */
    { DE_NAS_5GS_MM_ADD_5G_SEC_INF,             "Additional 5G security information" }, /* 9.11.3.12    Additional 5G security information */
    { DE_NAS_5GS_MM_ALLOW_PDU_SES_STS,          "Allowed PDU session status" },         /* 9.11.3.13    Allowed PDU session status*/
    { DE_NAS_5GS_MM_AUT_FAIL_PAR,               "Authentication failure parameter" },   /* 9.11.3.14    Authentication failure parameter*/
    { DE_NAS_5GS_MM_AUT_PAR_AUTN,               "Authentication parameter AUTN" },      /* 9.11.3.15    Authentication parameter AUTN*/
    { DE_NAS_5GS_MM_AUT_PAR_RAND,               "Authentication parameter RAND" },      /* 9.11.3.16    Authentication parameter RAND*/
    { DE_NAS_5GS_MM_AUT_RESP_PAR,               "Authentication response parameter" },  /* 9.11.3.17    Authentication response parameter*/
    { DE_NAS_5GS_MM_CONF_UPD_IND,               "Configuration update indication" },    /* 9.11.3.18    Configuration update indication*/
    { DE_NAS_5GS_MM_DLGT_SAVING_TIME,           "Daylight saving time" },               /* 9.11.3.19    Daylight saving time*/
    { DE_NAS_5GS_MM_DE_REG_TYPE,                "De-registration type" },               /* 9.11.3.20    De-registration type*/
                                                                                        /* 9.11.3.21    Void */
                                                                                        /* 9.11.3.22    Void*/
    { DE_NAS_5GS_MM_EMRG_NR_LIST,               "Emergency number list" },              /* 9.11.3.23    Emergency number list*/
    { DE_NAS_5GS_MM_EPS_NAS_MSG_CONT,           "EPS NAS message container" },          /* 9.11.3.24    EPS NAS message container*/
    { DE_NAS_5GS_MM_EPS_NAS_SEC_ALGO,           "EPS NAS security algorithms" },        /* 9.11.3.25    EPS NAS security algorithms*/
    { DE_NAS_5GS_MM_EXT_EMERG_NUM_LIST,         "Extended emergency number list" },     /* 9.11.3.26    Extended emergency number list */
                                                                                        /* 9.11.3.27    Void*/
    { DE_NAS_5GS_MM_IMEISV_REQ,                 "IMEISV request" },                     /* 9.11.3.28    IMEISV request*/
    { DE_NAS_5GS_MM_LADN_INDIC,                 "LADN indication" },                    /* 9.11.3.29    LADN indication*/
    { DE_NAS_5GS_MM_LADN_INF,                   "LADN information" },                   /* 9.11.3.30    LADN information*/
    { DE_NAS_5GS_MM_MICO_IND,                   "MICO indication" },                    /* 9.11.3.31    MICO indication*/
    { DE_NAS_5GS_MM_NAS_KEY_SET_ID,             "NAS key set identifier" },             /* 9.11.3.32    NAS key set identifier*/
    { DE_NAS_5GS_MM_NAS_MSG_CONT,               "NAS message container" },              /* 9.11.3.33    NAS message container*/
    { DE_NAS_5GS_MM_NAS_SEC_ALGO,               "NAS security algorithms" },            /* 9.11.3.34    NAS security algorithms*/
    { DE_NAS_5GS_MM_NW_NAME,                    "Network name" },                       /* 9.11.3.35    Network name*/
    { DE_NAS_5GS_MM_NW_SLICING_IND,             "Network slicing indication" },         /* 9.11.3.36    Network slicing indication */
    { DE_NAS_5GS_MM_NSSAI,                      "NSSAI" },                              /* 9.11.3.37    NSSAI*/
    { DE_NAS_5GS_MM_NSSAI_INC_MODE,             "NSSAI inclusion mode" },               /* 9.11.3.37A   NSSAI inclusion mode */
    { DE_NAS_5GS_MM_OP_DEF_ACC_CAT_DEF,         "Operator-defined access category definitions" },/* 9.11.3.38    Operator-defined access category definitions */

    { DE_NAS_5GS_MM_PLD_CONT,                   "Payload container" },                  /* 9.11.3.39    Payload container*/
    { DE_NAS_5GS_MM_PLD_CONT_TYPE,              "Payload container type" },             /* 9.11.3.40    Payload container type*/
    { DE_NAS_5GS_MM_PDU_SES_ID_2,               "PDU session identity 2" },             /* 9.11.3.42    PDU session identity 2*/
    { DE_NAS_5GS_MM_PDU_SES_REACT_RES,          "PDU session reactivation result" },    /* 9.11.3.43    PDU session reactivation result*/
    { DE_NAS_5GS_MM_PDU_SES_REACT_RES_ERR_C,    "PDU session reactivation result error cause" },    /* 9.11.3.43    PDU session reactivation result error cause*/
    { DE_NAS_5GS_MM_PDU_SES_STATUS,             "PDU session status" },                 /* 9.11.3.44    PDU session status*/
    { DE_NAS_5GS_MM_PLMN_LIST,                  "PLMN list" },                          /* 9.11.3.45    PLMN list*/
    { DE_NAS_5GS_MM_REJ_NSSAI,                  "Rejected NSSAI" },                     /* 9.11.3.46    Rejected NSSAI*/
    { DE_NAS_5GS_MM_REQ_TYPE,                   "Request type" },                       /* 9.11.3.47    Request type*/
    { DE_NAS_5GS_MM_S1_UE_NW_CAP,               "S1 UE network capability" },           /* 9.11.3.48    S1 UE network capability*/
    { DE_NAS_5GS_MM_S1_UE_SEC_CAP,              "S1 UE security capability" },          /* 9.11.3.48A   S1 UE security capability*/
    { DE_NAS_5GS_MM_SAL,                        "Service area list" },                  /* 9.11.3.49    Service area list*/
    { DE_NAS_5GS_MM_SERV_TYPE,                  "Service type" },                       /* 9.11.3.50    Service type*/
    { DE_NAS_5GS_MM_SMS_IND,                    "SMS indication" },                     /* 9.11.3.50A   SMS indication */
    { DE_NAS_5GS_MM_SOR_TRASP_CONT,             "SOR transparent container" },          /* 9.11.3.51    SOR transparent container */
    { DE_NAS_5GS_MM_TZ,                         "Time zone" },                          /* 9.11.3.52    Time zone*/
    { DE_NAS_5GS_MM_TZ_AND_T,                   "Time zone and time" },                 /* 9.11.3.53    Time zone and time*/
    { DE_NAS_5GS_MM_UE_PAR_UPD_TRASNSP_CONT,    "UE parameters update transparent container" }, /* 9.11.3.53A   UE parameters update transparent container */
    { DE_NAS_5GS_MM_UE_SEC_CAP,                 "UE security capability" },             /* 9.11.3.54    UE security capability*/
    { DE_NAS_5GS_MM_UE_USAGE_SET,               "UE's usage setting" },                 /* 9.11.3.55    UE's usage setting*/
    { DE_NAS_5GS_MM_UE_STATUS,                  "UE status" },                          /* 9.11.3.56    UE status*/
    { DE_NAS_5GS_MM_UL_DATA_STATUS,             "Uplink data status" },                 /* 9.11.3.57    Uplink data status*/

    { 0, NULL }
};
value_string_ext nas_5gs_mm_elem_strings_ext = VALUE_STRING_EXT_INIT(nas_5gs_mm_elem_strings);

#define NUM_NAS_5GS_MM_ELEM (sizeof(nas_5gs_mm_elem_strings)/sizeof(value_string))
gint ett_nas_5gs_mm_elem[NUM_NAS_5GS_MM_ELEM];

guint16(*nas_5gs_mm_elem_fcn[])(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string, int string_len) = {
        /*  9.11.3    5GS mobility management (5GMM) information elements */
        de_nas_5gs_mm_5gmm_cap,                  /* 9.11.3.1     5GMM capability*/
        de_nas_5gs_mm_5gmm_cause,                /* 9.11.3.2     5GMM cause*/
        de_nas_5gs_mm_5gs_drx_param,             /* 9.11.3.2A    5GS DRX parameters*/
        de_nas_5gs_mm_5gs_identity_type,         /* 9.11.3.3     5GS identity type*/
        de_nas_5gs_mm_5gs_mobile_id,             /* 9.11.3.4     5GS mobile identity*/
        de_nas_5gs_mm_5gs_nw_feat_sup,           /* 9.11.3.5     5GS network feature support*/
        de_nas_5gs_mm_5gs_reg_res,               /* 9.11.3.6     5GS registration result*/
        de_nas_5gs_mm_5gs_reg_type,              /* 9.11.3.7     5GS registration type*/
        de_nas_5gs_mm_5gs_ta_id,                 /* 9.11.3.8     5GS tracking area identity */
        de_nas_5gs_mm_5gs_ta_id_list,            /* 9.11.3.9     5GS tracking area identity list */
        de_nas_5gs_mm_update_type,               /* 9.11.3.9A    5GS update type */
        de_nas_5gs_mm_abba,                      /* 9.11.3.10    ABBA */
        de_nas_5gs_mm_access_type,               /* 9.11.3.11    Access type */
        de_nas_5gs_mm_add_5g_sec_inf,            /* 9.11.3.12    Additional 5G security information */
        de_nas_5gs_mm_allow_pdu_ses_sts,         /* 9.11.3.13    Allowed PDU session status*/
        NULL,                                    /* 9.11.3.14    Authentication failure parameter */
        NULL,                                    /* 9.11.3.15    Authentication parameter AUTN*/
        NULL,                                    /* 9.11.3.16    Authentication parameter RAND*/
        NULL,                                    /* 9.11.3.17    Authentication response parameter */
        de_nas_5gs_mm_conf_upd_ind,              /* 9.11.3.18    Configuration update indication*/
        NULL,                                    /* 9.11.3.19    Daylight saving time*/
        de_nas_5gs_mm_de_reg_type,               /* 9.11.3.20    De-registration type*/
                                                 /* 9.11.3.21    Void */
                                                 /* 9.11.3.22    Void*/
        NULL,                                    /* 9.11.3.23    Emergency number list*/
        de_nas_5gs_mm_eps_nas_msg_cont,          /* 9.11.3.24    EPS NAS message container*/
        NULL,                                    /* 9.11.3.25    EPS NAS security algorithms*/
        NULL,                                    /* 9.11.3.26    Extended emergency number list*/
                                                 /* 9.11.3.27    Void*/
        NULL,                                    /* 9.11.3.28    IMEISV request*/
        de_nas_5gs_mm_ladn_indic,                /* 9.11.3.29    LADN indication*/
        de_nas_5gs_mm_ladn_inf,                  /* 9.11.3.30    LADN information*/
        de_nas_5gs_mm_mico_ind,                  /* 9.11.3.31    MICO indication*/
        de_nas_5gs_mm_nas_key_set_id,            /* 9.11.3.32    NAS key set identifier*/
        de_nas_5gs_mm_nas_msg_cont,              /* 9.11.3.33    NAS message container*/
        de_nas_5gs_mm_nas_sec_algo,              /* 9.11.3.34    NAS security algorithms*/
        NULL,                                    /* 9.11.3.35    Network name*/
        de_nas_5gs_mm_nw_slicing_ind,            /* 9.11.3.36    Network slicing indication */
        de_nas_5gs_mm_nssai,                     /* 9.11.3.37    NSSAI*/
        de_nas_5gs_mm_nssai_inc_mode,            /* 9.11.3.37A   NSSAI inclusion mode */
        de_nas_5gs_mm_op_def_acc_cat_def,        /* 9.11.3.38    Operator-defined access category definitions */
        de_nas_5gs_mm_pld_cont,                  /* 9.11.3.39    Payload container*/
        de_nas_5gs_mm_pld_cont_type,             /* 9.11.3.40    Payload container type*/
        de_nas_5gs_mm_pdu_ses_id_2,              /* 9.11.3.41    PDU session identity 2*/
        de_nas_5gs_mm_pdu_ses_react_res,         /* 9.11.3.42    PDU session reactivation result*/
        de_nas_5gs_mm_pdu_ses_react_res_err_c,   /* 9.11.3.43    PDU session reactivation result error cause */
        de_nas_5gs_mm_pdu_ses_status,            /* 9.11.3.44    PDU session status*/
        NULL,                                    /* 9.11.3.45    PLMN list*/
        de_nas_5gs_mm_rej_nssai,                 /* 9.11.3.46    Rejected NSSAI*/
        de_nas_5gs_mm_req_type,                  /* 9.11.3.47    Request type*/
        NULL,                                    /* 9.11.3.48    S1 UE network capability*/
        NULL,                                    /* 9.11.3.48A   S1 UE security capability*/
        de_nas_5gs_mm_sal,                       /* 9.11.3.49    Service area list*/
        NULL,                                    /* 9.11.3.50    Service type*/ /* Used Inline Half octet IE */
        de_nas_5gs_mm_sms_ind,                   /* 9.11.3.50A   SMS indication */
        de_nas_5gs_mm_sor_trasp_cont,            /* 9.11.3.51    SOR transparent container */
        NULL,                                    /* 9.11.3.52    Time zone*/
        NULL,                                    /* 9.11.3.53    Time zone and time*/
        de_nas_5gs_mm_ue_par_upd_trasnsp_cont,   /* 9.11.3.53A   UE parameters update transparent container */
        de_nas_5gs_mm_ue_sec_cap,                /* 9.11.3.54    UE security capability*/
        de_nas_5gs_mm_ue_usage_set,              /* 9.11.3.55    UE's usage setting*/
        de_nas_5gs_mm_ue_status,                 /* 9.11.3.56    UE status*/
        de_nas_5gs_mm_ul_data_status,            /* 9.11.3.57    Uplink data status*/
        NULL,   /* NONE */
};


/*
 * 9.11.4    5GS session management (5GSM) information elements
 */

typedef enum
{

    DE_NAS_5GS_SM_5GSM_CAP,                 /* 9.11.4.1    5GSM capability */
    DE_NAS_5GS_SM_5GSM_CAUSE,               /* 9.11.4.2    5GSM cause */
                                            /* 9.11.4.3    Always-on PDU session indication */
                                            /* 9.11.4.4    Always-on PDU session requested */
    DE_NAS_5GS_SM_5GSM_ALLOWED_SSC_MODE,    /* 9.11.4.5    Allowed SSC mode */
    DE_NAS_5GS_SM_EXT_PROT_CONF_OPT,        /* 9.11.4.6    Extended protocol configuration options */
                                            /* 9.11.4.7    Integrity protection maximum data rate */
    DE_NAS_5GS_SM_MAPPED_EPS_B_CONT,        /* 9.11.4.8    Mapped EPS bearer contexts */
    DE_NAS_5GS_SM_MAX_NUM_SUP_PKT_FLT,      /* 9.11.4.9    Maximum number of supported packet filters */
    DE_NAS_5GS_SM_PDU_ADDRESS,              /* 9.11.4.10   PDU address */
    DE_NAS_5GS_SM_PDU_SESSION_TYPE,         /* 9.11.4.11   PDU session type */
    DE_NAS_5GS_SM_QOS_FLOW_DES,             /* 9.11.4.12   QoS flow descriptions */
    DE_NAS_5GS_SM_QOS_RULES,                /* 9.11.4.13   QoS rules */
    DE_NAS_5GS_SM_SESSION_AMBR,             /* 9.11.4.14   Session-AMBR */
    DE_NAS_5GS_SM_PDU_DN_REQ_CONT,          /* 9.11.4.15   SM PDU DN request container */
    DE_NAS_5GS_SM_SSC_MODE,                 /* 9.11.4.16   SSC mode */
    DE_NAS_5GS_SM_NONE        /* NONE */
}
nas_5gs_sm_elem_idx_t;


static const value_string nas_5gs_sm_elem_strings[] = {
    { DE_NAS_5GS_SM_5GSM_CAP, "5GSM capability" },                                         /* 9.11.4.1    5GSM capability */
    { DE_NAS_5GS_SM_5GSM_CAUSE, "5GSM cause" },                                            /* 9.11.4.2    5GSM cause */
                                                                                           /* 9.11.4.3    Always-on PDU session indication */
                                                                                           /* 9.11.4.4    Always-on PDU session requested */
    { DE_NAS_5GS_SM_5GSM_ALLOWED_SSC_MODE, "Allowed SSC mode" },                           /* 9.11.4.5    Allowed SSC mode */
    { DE_NAS_5GS_SM_EXT_PROT_CONF_OPT, "Extended protocol configuration options" },        /* 9.11.4.6    Extended protocol configuration options */
                                                                                           /* 9.11.4.7    Integrity protection maximum data rate */
    { DE_NAS_5GS_SM_MAPPED_EPS_B_CONT, "Mapped EPS bearer contexts" },                     /* 9.11.4.8    Mapped EPS bearer contexts */
    { DE_NAS_5GS_SM_MAX_NUM_SUP_PKT_FLT, "Maximum number of supported packet filters" },   /* 9.11.4.9    Maximum number of supported packet filters */
    { DE_NAS_5GS_SM_PDU_ADDRESS, "PDU address" },                                          /* 9.11.4.10   PDU address */
    { DE_NAS_5GS_SM_PDU_SESSION_TYPE, "PDU session type" },                                /* 9.11.4.11   PDU session type */
    { DE_NAS_5GS_SM_QOS_FLOW_DES, "QoS flow descriptions" },                               /* 9.11.4.12   QoS flow descriptions */
    { DE_NAS_5GS_SM_QOS_RULES, "QoS rules" },                                              /* 9.11.4.13   QoS rules */
    { DE_NAS_5GS_SM_SESSION_AMBR, "Session-AMBR" },                                        /* 9.11.4.14   Session-AMBR */
    { DE_NAS_5GS_SM_PDU_DN_REQ_CONT, "SM PDU DN request container" },                      /* 9.11.4.15   SM PDU DN request container */
    { DE_NAS_5GS_SM_SSC_MODE, "SSC mode" },                                                /* 9.11.4.16   SSC mode */

    { 0, NULL }
};
value_string_ext nas_5gs_sm_elem_strings_ext = VALUE_STRING_EXT_INIT(nas_5gs_sm_elem_strings);

#define NUM_NAS_5GS_SM_ELEM (sizeof(nas_5gs_sm_elem_strings)/sizeof(value_string))
gint ett_nas_5gs_sm_elem[NUM_NAS_5GS_SM_ELEM];

guint16(*nas_5gs_sm_elem_fcn[])(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo,
    guint32 offset, guint len,
    gchar *add_string, int string_len) = {
        /*  5GS session management (5GSM) information elements */
        de_nas_5gs_sm_5gsm_cap,              /* 9.11.4.1    5GSM capability */
        de_nas_5gs_sm_5gsm_cause,            /* 9.11.4.2    5GSM cause */
                                             /* 9.11.4.3    Always-on PDU session indication */
                                             /* 9.11.4.4    Always-on PDU session requested */
        de_nas_5gs_sm_5gsm_allowed_ssc_mode, /* 9.11.4.5   Allowed SSC mode */
        NULL,                                /* 9.11.4.6    Extended protocol configuration options */
                                             /* 9.11.4.7    Integrity protection maximum data rate */
        de_nas_5gs_sm_mapped_eps_b_cont,     /* 9.11.4.8    Mapped EPS bearer contexts */
        de_nas_5gs_sm_max_num_sup_pkt_flt,   /* 9.11.4.9    Maximum number of supported packet filters */
        de_nas_5gs_sm_pdu_address,           /* 9.11.4.10   PDU address */
        de_nas_5gs_sm_pdu_session_type,      /* 9.11.4.11   PDU session type */
        de_nas_5gs_sm_qos_flow_des,          /* 9.11.4.12   QoS flow descriptions */
        de_nas_5gs_sm_qos_rules,             /* 9.11.4.13    QoS rules */
        de_nas_5gs_sm_session_ambr,          /* 9.11.4.14   Session-AMBR */
        de_nas_5gs_sm_pdu_dn_req_cont,       /* 9.11.4.15   SM PDU DN request container */
        de_nas_5gs_sm_ssc_mode,              /* 9.11.4.16   SSC mode */
        NULL,   /* NONE */
};

/* Gap fill msg decoding*/
static void
nas_5gs_exp_not_dissected_yet(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{

    proto_tree_add_expert(tree, pinfo, &ei_nas_5gs_msg_not_dis, tvb, offset, len);
}

/*
 * 8.2.1    Authentication request
 */
static void
nas_5gs_mm_authentication_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /*ngKSI     NAS key set identifier 9.11.3.29    M    V    1/2  */
    /* Spare half octet    Spare half octet     9.5    M    V    1/2 H1 */
    proto_tree_add_item(tree, hf_nas_5gs_spare_half_octet, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_NAS_KEY_SET_ID, " - ngKSI", ei_nas_5gs_missing_mandatory_elemen);
    /* ABBA    ABBA 9.11.3.10    M    LV    3-n */
    ELEM_MAND_LV(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_ABBA, NULL, ei_nas_5gs_missing_mandatory_elemen);
    /*21    Authentication parameter RAND (5G authentication challenge)    Authentication parameter RAND     9.11.3.13    O    TV    17*/
    ELEM_OPT_TV(0x21, GSM_A_PDU_TYPE_DTAP, DE_AUTH_PARAM_RAND, " - 5G authentication challenge");
    /*20    Authentication parameter AUTN (5G authentication challenge)    Authentication parameter AUTN     9.11.3.14    O    TLV    18*/
    ELEM_OPT_TLV(0x20, GSM_A_PDU_TYPE_DTAP, DE_AUTH_PARAM_AUTN, " - 5G authentication challenge");
    /*78    EAP message    EAP message 9.10.2.2    O    TLV-E    7-1503 */
    ELEM_OPT_TLV_E(0x78, NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_EAP_MESSAGE, NULL);


    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}
/*
 *8.2.2    Authentication response
 */
static void
nas_5gs_mm_authentication_resp(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* 2D    Authentication response parameter    Authentication response parameter 9.11.3.15    O    TLV    6-18 */
    ELEM_OPT_TLV( 0x2d, NAS_PDU_TYPE_EMM, DE_EMM_AUTH_RESP_PAR, NULL);
    /* 78 EAP message    EAP message     9.10.2.2    O    TLV-E    7-1503 */
    ELEM_OPT_TLV_E(0x78,  NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_EAP_MESSAGE, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.2.3 Authentication result
 */
static void
nas_5gs_mm_authentication_result(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* ngKSI    NAS key set identifier 9.11.3.27    M    V    1/2
       Spare half octet    Spare half octet 9.5    M    V    1/2  H1 */
    proto_tree_add_item(tree, hf_nas_5gs_spare_half_octet, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_NAS_KEY_SET_ID, " - ngKSI", ei_nas_5gs_missing_mandatory_elemen);

    /* EAP message    EAP message     9.11.2.2    M    LV-E    7-1503 */
    ELEM_MAND_LV_E(NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_EAP_MESSAGE, NULL, ei_nas_5gs_missing_mandatory_elemen);

    /* 38    ABBA    ABBA 9.11.3.10    O    TLV    4-n */
    ELEM_OPT_TLV(0x38, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_ABBA, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}
/*
 * 8.2.4 Authentication failure
 */
static void
nas_5gs_mm_authentication_failure(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* 5GMM cause   5GMM cause     9.11.3.2  M   V   1 */
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GMM_CAUSE, NULL, ei_nas_5gs_missing_mandatory_elemen);

    /* 30    Authentication failure parameter    Authentication failure parameter 9.11.3.14    O    TLV    16 */
    ELEM_OPT_TLV(0x30, GSM_A_PDU_TYPE_DTAP, DE_AUTH_FAIL_PARAM, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}
/*
 * 8.2.5 Authentication reject
 */
static void
nas_5gs_mm_authentication_rej(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* 78    EAP message    EAP message 9.11.2.2    O    TLV-E    7-1503 */
    ELEM_OPT_TLV_E(0x78, NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_EAP_MESSAGE, NULL);


    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.2.6 Registration request
 */
static void
nas_5gs_mm_registration_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: UE to network */
    pinfo->link_dir = P2P_DIR_UL;

    /* Initalize the private struct */
    nas5gs_get_private_data(pinfo);

    /*   5GS registration type    5GS registration type 9.11.3.7    M    V    1/2  H0*/
    proto_tree_add_bitmask_list(tree, tvb, offset, 1, nas_5gs_registration_type_flags, ENC_BIG_ENDIAN);

    /*    ngKSI    NAS key set identifier 9.11.3.32    M    V    1/2 H1*/
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_NAS_KEY_SET_ID, " - ngKSI", ei_nas_5gs_missing_mandatory_elemen);

    /*    Mobile identity    5GS mobile identity 9.11.3.4    M    LV-E    6-n*/
    ELEM_MAND_LV_E(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GS_MOBILE_ID, NULL, ei_nas_5gs_missing_mandatory_elemen);

    /*C-    Non-current native NAS KSI    NAS key set identifier 9.11.3.32    O    TV    1*/
    ELEM_OPT_TV_SHORT(0xc0, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_NAS_KEY_SET_ID, " - native KSI");

    /*10    5GMM capability    5GMM capability 9.11.3.1    O    TLV    4-15*/
    ELEM_OPT_TLV(0x10, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GMM_CAP, NULL);

    /*2E    UE security capability    UE security capability 9.11.3.54    O    TLV    4-6*/
    ELEM_OPT_TLV(0x2e, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_UE_SEC_CAP, NULL);

    /*2F    Requested NSSAI    NSSAI 9.11.3.37    O    TLV    4-74*/
    ELEM_OPT_TLV(0x2f, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_NSSAI, " - Requested NSSAI");

    /*52    Last visited registered TAI    5GS tracking area identity 9.11.3.8    O    TV    7 */
    ELEM_OPT_TV(0x52, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GS_TA_ID, " - Last visited registered TAI");

    /*17    S1 UE network capability    S1 UE network capability 9.11.3.48    O    TLV    4-15 */
    ELEM_OPT_TLV(0x17, NAS_PDU_TYPE_EMM, DE_EMM_UE_NET_CAP, NULL);

    /*40    Uplink data status    Uplink data status 9.11.3.57    O    TLV    4-34 */
    ELEM_OPT_TLV(0x40, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_UL_DATA_STATUS, NULL);

    /*50    PDU session status    PDU session status 9.11.3.44    O    TLV    4-34 */
    ELEM_OPT_TLV(0x50, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_PDU_SES_STATUS, NULL);

    /*B-    MICO indication    MICO indication 9.11.3.31    O    TV    1*/
    ELEM_OPT_TV_SHORT(0xb0, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_MICO_IND, NULL);

    /*2B    UE status    UE status 9.11.3.56    O    TLV    3*/
    ELEM_OPT_TLV(0x2b, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_UE_STATUS, NULL);

    /*77    Additional GUTI    5GS mobile identity 9.11.3.4    O    TLV-E    14 */
    ELEM_OPT_TLV_E(0x77, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GS_MOBILE_ID, " -  Additional GUTI");

    /*25    Allowed PDU session status    Allowed PDU session status         9.11.3.13    O    TLV    4 - 34 */
    ELEM_OPT_TLV(0x25, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_ALLOW_PDU_SES_STS, NULL);

    /*18    UE's usage setting    UE's usage setting         9.11.3.55    O    TLV    3 */
    ELEM_OPT_TLV(0x18, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_UE_USAGE_SET, NULL);

    /*51    Requested DRX parameters    5GS DRX parameters 9.11.3.2A    O    TLV    3 */
    ELEM_OPT_TLV(0x51, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GS_DRX_PARAM, " - Requested DRX parameters");

    /*70    EPS NAS message container    EPS NAS message container 9.11.3.24    O    TLV-E    4-n */
    ELEM_OPT_TLV_E(0x70, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_EPS_NAS_MSG_CONT, NULL);

    /* 74    LADN indication    LADN indication 9.11.3.29    O    TLV-E    3-811 */
    ELEM_OPT_TLV_E(0x74, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_LADN_INF, NULL);

    /* 7B    Payload container     Payload container 9.11.3.39    O    TLV-E    4-65538 */
    ELEM_OPT_TLV_E(0x7B, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_PLD_CONT, NULL);

    /* 9-    Network slicing indication    Network slicing indication 9.11.3.36    O    TV    1 */
    ELEM_OPT_TV_SHORT(0x90, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_NW_SLICING_IND, NULL);

    /* 53    5GS update type    5GS update type 9.11.3.9A    O    TLV    3 */
    ELEM_OPT_TLV(0x53, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_UPDATE_TYPE, NULL);

    /* 71    NAS message container    NAS message container 9.11.3.33    O    TLV-E    4-n */
    ELEM_OPT_TLV_E(0x71, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_NAS_MSG_CONT, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.2.7    Registration accept
 */

static void
nas_5gs_mm_registration_accept(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /*      5GS registration result    5GS registration result     9.11.3.6    M    LV    2*/
    ELEM_MAND_LV(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GS_REG_RES, NULL, ei_nas_5gs_missing_mandatory_elemen);
    /*77    5G-GUTI    5GS mobile identity 9.11.3.4    O    TLV-E    14 */
    ELEM_OPT_TLV_E(0x77, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GS_MOBILE_ID, " - 5G-GUTI");
    /*4A    Equivalent PLMNs    PLMN list     9.11.3.33    O    TLV    5-47*/
    ELEM_OPT_TLV(0x4a, GSM_A_PDU_TYPE_COMMON, DE_PLMN_LIST, " - Equivalent PLMNs");
    /*54    TAI list    Tracking area identity list     9.11.3.9    O    TLV    8-98*/
    ELEM_OPT_TLV(0x54, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GS_TA_ID_LIST, NULL);
    /*15    Allowed NSSAI    NSSAI     9.11.3.28    O    TLV    4-74*/
    ELEM_OPT_TLV(0x15, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_NSSAI, " - Allowed NSSAI");
    /*11    Rejected NSSAI    Rejected NSSAI     9.11.3.35    O    TLV    4-42*/
    ELEM_OPT_TLV(0x11, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_REJ_NSSAI, NULL);
    /*31    Configured NSSAI    NSSAI 9.11.3.34    O    TLV    4-146 */
    ELEM_OPT_TLV(0x31, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_NSSAI, " - Configured NSSAI");
    /*21    5GS network feature support    5GS network feature support 9.11.3.5    O    TLV    3-5 */
    ELEM_OPT_TLV(0x21, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GS_NW_FEAT_SUP, NULL);
    /*50    PDU session status    PDU session status     9.10.2.2    O    TLV    4*/
    ELEM_OPT_TLV(0x50, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_PDU_SES_STATUS, NULL);
    /*26    PDU session reactivation result    PDU session reactivation result     9.11.3.32    O    TLV    4-32*/
    ELEM_OPT_TLV(0x26, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_PDU_SES_REACT_RES, NULL);
    /*72    PDU session reactivation result error cause PDU session reactivation result error cause 9.11.3.40  O TLV-E  5-515*/
    ELEM_OPT_TLV_E(0x72, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_PDU_SES_REACT_RES_ERR_C, NULL);
    /*79    LADN information    LADN information     9.11.3.19    O    TLV-E    11-1579*/
    ELEM_OPT_TLV_E(0x79, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_LADN_INF, NULL);
    /*B-    MICO indication    MICO indication     9.11.3.21    O    TV    1*/
    ELEM_OPT_TV_SHORT(0xb0, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_MICO_IND, NULL);
    /* 9-    Network slicing indication    Network slicing indication 9.11.3.36    O    TV    1 */
    ELEM_OPT_TV_SHORT(0x90, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_NW_SLICING_IND, NULL);
    /*27    Service area list    Service area list     9.11.3.47    O    TLV    6-194*/
    ELEM_OPT_TLV(0x27, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_SAL, NULL);
    /*5E    T3512 value    GPRS timer 3     9.11.3.21    O    TLV    3*/
    ELEM_OPT_TLV(0x5E, GSM_A_PDU_TYPE_GM, DE_GPRS_TIMER_3, " - T3512 value");
    /*5D    Non-3GPP de-registration timer value    GPRS timer 2     9.11.3.20    O    TLV    3*/
    ELEM_OPT_TLV(0x5D, GSM_A_PDU_TYPE_GM, DE_GPRS_TIMER_2, " - Non-3GPP de-registration timer value");
    /*16    T3502 value    GPRS timer 2     9.10.2.4     O    TLV    3*/
    ELEM_OPT_TLV(0x16, GSM_A_PDU_TYPE_GM, DE_GPRS_TIMER_2, " - T3502 value");
    /*34    Emergency number list    Emergency number list     9.11.3.17    O    TLV    5-50*/
    ELEM_OPT_TLV(0x34, GSM_A_PDU_TYPE_DTAP, DE_EMERGENCY_NUM_LIST, NULL);
    /*7A    Extended emergency number list    Extended emergency number list 9.11.3.24    O    TLV    TBD*/
    ELEM_OPT_TLV(0x7A, NAS_PDU_TYPE_EMM, DE_EMM_EXT_EMERG_NUM_LIST, NULL);
    /*73    SOR transparent container    SOR transparent container 9.11.3.51    O    TLV-E    20-2048 */
    ELEM_OPT_TLV_E(0x73, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_SOR_TRASP_CONT, NULL);
    /*78    EAP message    EAP message 9.10.2.2    O    TLV-E    7-1503 */
    ELEM_OPT_TLV_E(0x78, NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_EAP_MESSAGE, NULL);
    /* A-    NSSAI inclusion mode    NSSAI inclusion mode 9.11.3.37A    O    TV    1 */
    ELEM_OPT_TV_SHORT(0xA0, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_NSSAI_INC_MODE, NULL);
    /* 76    Operator-defined access category definitions    Operator-defined access category definitions 9.11.3.38    O    TLV-E    3-TBD */
    ELEM_OPT_TLV_E(0x76, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_OP_DEF_ACC_CAT_DEF, NULL);
    /* 51    Negotiated DRX parameters    5GS DRX parameters 9.11.3.2A    O    TLV    3 */
    ELEM_OPT_TLV(0x51, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GS_DRX_PARAM, " -  Negotiated DRX parameters");

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.2.8 Registration complete
 */
static void
nas_5gs_mm_registration_complete(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* 73    SOR transparent container    SOR transparent container 9.11.3.51    O    TLV-E    20-2048 */
    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}
/*
* 8.2.9 Registration reject
*/
static void
nas_5gs_mm_registration_rej(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* 5GMM cause   5GMM cause     9.11.3.2  M   V   1 */
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GMM_CAUSE, NULL, ei_nas_5gs_missing_mandatory_elemen);

    /* 5F  T3346 value GPRS timer 2     9.11.3.16   O   TLV 3 */
    ELEM_OPT_TLV(0x5F, GSM_A_PDU_TYPE_GM, DE_GPRS_TIMER_2, " - T3346 value");

    /* 16    T3502 value    GPRS timer 2 9.10.2.4    O    TLV    3 */
    ELEM_OPT_TLV(0x16, GSM_A_PDU_TYPE_GM, DE_GPRS_TIMER_2, " - T3502 value");

    /* 78    EAP message    EAP message 9.10.2.2    O    TLV-E    7-1503 */
    ELEM_OPT_TLV_E(0x78, NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_EAP_MESSAGE, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.2.10    UL NAS transport
 */
static void
nas_5gs_mm_ul_nas_transp(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    /* Direction: UE to network */
    pinfo->link_dir = P2P_DIR_UL;

    curr_offset = offset;
    curr_len = len;

    /* Initalize the private struct */
    nas5gs_get_private_data(pinfo);

    /*Payload container type    Payload container type     9.11.3.31    M    V    1/2 */
    /*Spare half octet    Spare half octet    9.5    M    V    1/2*/
    proto_tree_add_item(tree, hf_nas_5gs_spare_half_octet, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_PLD_CONT_TYPE, NULL, ei_nas_5gs_missing_mandatory_elemen);
    /*Payload container    Payload container    9.11.3.30    M    LV-E    3-65537*/
    ELEM_MAND_LV_E(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_PLD_CONT, NULL, ei_nas_5gs_missing_mandatory_elemen);
    /*12    PDU session ID    PDU session identity 2 9.11.3.41    C    TV    2 */
    ELEM_OPT_TV(0x12, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_PDU_SES_ID_2, " - PDU session ID");
    /*59    Old PDU session ID    PDU session identity 2 9.11.3.37    O    TV    2 */
    ELEM_OPT_TV(0x59, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_PDU_SES_ID_2, " - Old PDU session ID");
    /*8-    Request type    Request type    9.11.3.42    O    TV    1 */
    ELEM_OPT_TV_SHORT(0x80, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_REQ_TYPE, NULL);
    /*22    S-NSSAI    S-NSSAI    9.11.3.37    O    TLV    3-10 */
    ELEM_OPT_TLV(0x22, NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_S_NSSAI, NULL);
    /*25    DNN    DNN    9.11.2.1A    O    TLV    3-102 */
    ELEM_OPT_TLV(0x25, NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_DNN, NULL);
    /*24    Additional information    Additional information    9.10.2.1    O    TLV    3-n */
    ELEM_OPT_TLV(0x24, NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_ADD_INF, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
* 8.2.11 DL NAS transport
*/
static void
nas_5gs_mm_dl_nas_transp(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    /* Direction: network to UE */

    curr_offset = offset;
    curr_len = len;

    /* Initalize the private struct */
    nas5gs_get_private_data(pinfo);

    /*Payload container type    Payload container type     9.11.3.40    M    V    1/2 H0*/
    /*Spare half octet    Spare half octet    9.5    M    V    1/2 H1*/
    proto_tree_add_item(tree, hf_nas_5gs_spare_half_octet, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_PLD_CONT_TYPE, NULL, ei_nas_5gs_missing_mandatory_elemen);
    /*Payload container    Payload container    9.11.3.39    M    LV-E    3-65537*/
    ELEM_MAND_LV_E(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_PLD_CONT, NULL, ei_nas_5gs_missing_mandatory_elemen);
    /*12    PDU session ID    PDU session identity 2 9.11.3.37    C    TV    2 */
    ELEM_OPT_TV(0x12, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_PDU_SES_ID_2, " - PDU session ID");
    /*24    Additional information    Additional information    9.10.2.1    O    TLV    3-n*/
    ELEM_OPT_TLV(0x24, NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_ADD_INF, NULL);
    /*58    5GMM cause    5GMM cause 9.11.3.2    O    TV    2 */
    ELEM_OPT_TV(0x58, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GMM_CAUSE, NULL);
    /*37    Back-off timer value    GPRS timer 3 9.10.2.5    O    TLV    3 */
    ELEM_OPT_TLV(0x37, GSM_A_PDU_TYPE_GM, DE_GPRS_TIMER_3, " - Back-off timer value");

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.2.12 De-registration request (UE originating de-registration)
 */
static void
nas_5gs_mm_de_reg_req_ue_orig(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* De-registration type    De-registration type     9.11.3.18   M   V   1 */
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_DE_REG_TYPE, NULL, ei_nas_5gs_missing_mandatory_elemen);

    /* ngKSI    NAS key set identifier 9.11.3.32    M    V    1/2 H1 */

    /*5GS mobile identity     5GS mobile identity 9.11.3.4    M    LV-E    6-n */
    ELEM_MAND_LV_E(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GS_MOBILE_ID, NULL, ei_nas_5gs_missing_mandatory_elemen);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.2.13 De-registration accept (UE originating de-registration)
 */
/* No data */

/*
 * 8.2.14 De-registration request (UE terminated de-registration)
 */
static void
nas_5gs_mm_de_registration_req_ue_term(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* De-registration type    De-registration type 9.11.3.20   M   V   1 */
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_DE_REG_TYPE, NULL, ei_nas_5gs_missing_mandatory_elemen);

    /* Spare half octet    Spare half octet 9.5    M    V    1/2 */
    /* 58 5GMM cause   5GMM cause     9.11.3.2  O   TV   2 */
    ELEM_OPT_TV(0x58, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GMM_CAUSE, NULL);
    /* 5F  T3346 value GPRS timer 2     9.11.2.4   O   TLV 3 */
    ELEM_OPT_TLV(0x5F, GSM_A_PDU_TYPE_GM, DE_GPRS_TIMER_2, " - T3346 value");

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.2.15 De-registration accept (UE terminated de-registration)
 */
 /* No data */


/*
 * 8.2.16 Service request
 */
static void
nas_5gs_mm_service_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* ngKSI     NAS key set identifier 9.11.3.29    M    V    1/2 */
    /* Service type    Service type 9.11.3.46    M    V    1/2 */
    proto_tree_add_item(tree, hf_nas_5gs_mm_serv_type, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_NAS_KEY_SET_ID, " - ngKSI", ei_nas_5gs_missing_mandatory_elemen);

    /* 5G-S-TMSI    5GS mobile identity 9.11.3.4    M    LV    6 */
    ELEM_MAND_LV_E(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GS_MOBILE_ID, NULL, ei_nas_5gs_missing_mandatory_elemen);
    /*40    Uplink data status    Uplink data status         9.11.3.53    O    TLV    4 - 34*/
    ELEM_OPT_TLV(0x40, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_UL_DATA_STATUS, NULL);
    /*50    PDU session status    PDU session status         9.11.3.40    O    TLV    4 - 34*/
    ELEM_OPT_TLV(0x50, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_PDU_SES_STATUS, NULL);
    /*25    Allowed PDU session status    Allowed PDU session status         9.11.3.11    O    TLV    4 - 34*/
    ELEM_OPT_TLV(0x25, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_ALLOW_PDU_SES_STS, NULL);
    /* 71    NAS message container    NAS message container 9.11.3.33    O    TLV-E    4-n */

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
  * 8.2.17 Service accept
 */
static void
nas_5gs_mm_service_acc(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /*50    PDU session status    PDU session status     9.11.3.44    O    TLV    4-34*/
    ELEM_OPT_TLV(0x50, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_PDU_SES_STATUS, NULL);

    /*26    PDU session reactivation result    PDU session reactivation result 9.11.3.42    O    TLV    4-32*/
    ELEM_OPT_TLV(0x26, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_PDU_SES_REACT_RES, NULL);
    /*72    PDU session reactivation result error cause    PDU session reactivation result error cause 9.11.3.43    O    TLV-E    5-515 */
    ELEM_OPT_TLV_E(0x72, NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_MM_PDU_SES_REACT_RES_ERR_C, NULL);
    /*78    EAP message    EAP message     9.11.2.2    O    TLV-E    7-1503*/
    ELEM_OPT_TLV_E(0x78, NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_EAP_MESSAGE, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);
}

/*
 * 8.2.18 Service reject
 */
static void
nas_5gs_mm_service_rej(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* 5GMM cause   5GMM cause     9.11.3.2  M   V   1 */
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GMM_CAUSE, NULL, ei_nas_5gs_missing_mandatory_elemen);

    /*50    PDU session status    PDU session status 9.11.3.44    O    TLV    4*/
    ELEM_OPT_TLV(0x50, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_PDU_SES_STATUS, NULL);

    /* 5F  T3346 value GPRS timer 2     9.11.2.4   O   TLV 3 */
    ELEM_OPT_TLV(0x5F, GSM_A_PDU_TYPE_GM, DE_GPRS_TIMER_2, " - T3346 value");

    /* 78    EAP message    EAP message 9.11.2.2    O    TLV-E    7-1503 */
    ELEM_OPT_TLV_E(0x78, NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_EAP_MESSAGE, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.2.19 Configuration update command
 */
static void
nas_5gs_mm_conf_upd_cmd(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /*D-    Configuration update indication    Configuration update indication 9.11.3.16    O    TV    1 */
    ELEM_OPT_TV_SHORT(0xD0, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_CONF_UPD_IND, NULL);
    /*77    5G-GUTI    5GS mobile identity     9.11.3.4    O    TLV    TBD*/
    ELEM_OPT_TLV_E(0x77, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GS_MOBILE_ID, NULL);
    /*54    TAI list    Tracking area identity list     9.11.3.45    O    TLV    8-98*/
    ELEM_OPT_TLV(0x54, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GS_TA_ID_LIST, NULL);
    /*15    Allowed NSSAI    NSSAI     9.11.3.28    O    TLV    4-74*/
    ELEM_OPT_TLV(0x15, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_NSSAI, " - Allowed NSSAI");
    /*27    Service area list    Service area list     9.11.3.39    O    TLV    6-194 */
    ELEM_OPT_TLV(0x70, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_SAL, NULL);
    /*43    Full name for network    Network name     9.11.3.26    O    TLV    3-n*/
    ELEM_OPT_TLV(0x43, GSM_A_PDU_TYPE_DTAP, DE_NETWORK_NAME, " - Full name for network");
    /*45    Short name for network    Network name     9.11.3.26    O    TLV    3-n*/
    ELEM_OPT_TLV(0x45, GSM_A_PDU_TYPE_DTAP, DE_NETWORK_NAME, " - Short Name");
    /*46    Local time zone    Time zone     9.11.3.46    O    TV    2*/
    ELEM_OPT_TV(0x46, GSM_A_PDU_TYPE_DTAP, DE_TIME_ZONE, " - Local");
    /*47    Universal time and local time zone    Time zone and time     9.11.3.47    O    TV    8*/
    ELEM_OPT_TV(0x47, GSM_A_PDU_TYPE_DTAP, DE_TIME_ZONE_TIME, " - Universal Time and Local Time Zone");
    /*49    Network daylight saving time    Daylight saving time     9.11.3.11    O    TLV    3*/
    ELEM_OPT_TLV(0x49, GSM_A_PDU_TYPE_DTAP, DE_DAY_SAVING_TIME, NULL);
    /*79    LADN information    LADN information     9.11.3.19    O    TLV-E    11-1579*/
    ELEM_OPT_TLV_E(0x79, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_LADN_INF, NULL);
    /*B-    MICO indication    MICO indication     9.11.3.21    O    TV    1*/
    ELEM_OPT_TV_SHORT(0xB0, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_MICO_IND, NULL);
    /*31    Configured NSSAI    NSSAI     9.11.3.28    O    TLV    4-74*/
    ELEM_OPT_TLV(0x31, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_NSSAI, " - Configured NSSAI");
    /*11    Rejected NSSAI     Rejected NSSAI   9.11.3.42   O   TLV   4-42*/
    ELEM_OPT_TLV(0x11, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_REJ_NSSAI, NULL);
    /* 76    Operator-defined access category definitions    Operator-defined access category definitions 9.11.3.38    O    TLV-E    3-TBD */
    ELEM_OPT_TLV_E(0x76, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_OP_DEF_ACC_CAT_DEF, NULL);
    /* F-    SMS indication    SMS indication 9.10.3.50A    O    TV    1 */

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.2.20 Configuration update complete
 */
static void
nas_5gs_mm_conf_update_comp(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* No Data */
    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);
}
/*
 * 8.2.21 Identity request
 */
static void
nas_5gs_mm_id_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /*     Identity type    5GS identity type 9.11.3.3    M    V    1/2 */
    /* Spare half octet    Spare half octet 9.5    M    V    1/2 */
    proto_tree_add_item(tree, hf_nas_5gs_spare_half_octet, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GS_IDENTITY_TYPE, NULL, ei_nas_5gs_missing_mandatory_elemen);


    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.2.22 Identity response
 */
static void
nas_5gs_mm_id_resp(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Mobile identity  5GS mobile identity 9.11.3.4    M    LV-E    3-n  */
    ELEM_MAND_LV_E(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GS_MOBILE_ID, NULL, ei_nas_5gs_missing_mandatory_elemen);


    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.2.23 Notification
 */
static void
nas_5gs_mm_notification(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Access type    Access type 9.11.3.11    M    V    1/2 DE_NAS_5GS_MM_ACCESS_TYPE */
    /* Spare half octet    Spare half octet 9.5    M    V    1/2  */
    proto_tree_add_item(tree, hf_nas_5gs_spare_half_octet, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_ACCESS_TYPE, NULL, ei_nas_5gs_missing_mandatory_elemen);


    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.2.24 Notification response
 */
static void
nas_5gs_mm_notification_resp(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* 50    PDU session status    PDU session status 9.11.3.40    O    TLV    4-34 */
    ELEM_OPT_TLV(0x50, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_PDU_SES_STATUS, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.2.25 Security mode command
 */
static void
nas_5gs_mm_sec_mode_cmd(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: network to UE */
    /*Selected NAS security algorithms    NAS security algorithms     9.11.3.34    M    V    1  */
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_NAS_SEC_ALGO, NULL, ei_nas_5gs_missing_mandatory_elemen);

    /*ngKSI     NAS key set identifier 9.11.3.32    M    V    1/2  */
    /* Spare half octet    Spare half octet     9.5    M    V    1/2 */
    proto_tree_add_item(tree, hf_nas_5gs_spare_half_octet, tvb, curr_offset, 1, ENC_BIG_ENDIAN);
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_NAS_KEY_SET_ID, " - ngKSI", ei_nas_5gs_missing_mandatory_elemen);

    /*Replayed UE security capabilities    UE security capability     9.11.3.54    M    LV    3-5*/
    ELEM_MAND_LV(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_UE_SEC_CAP, " - Replayed UE security capabilities", ei_nas_5gs_missing_mandatory_elemen);

    /*E-    IMEISV request    IMEISV request     9.11.3.28    O    TV    1*/
    ELEM_OPT_TV_SHORT(0xE0, NAS_PDU_TYPE_EMM, DE_EMM_IMEISV_REQ, NULL);

    /*57    Selected EPS NAS security algorithms    EPS NAS security algorithms 9.11.3.25    O    TV    2 */
    ELEM_OPT_TV(0x57, NAS_PDU_TYPE_EMM, DE_EMM_NAS_SEC_ALGS, " - Selected EPS NAS security algorithms");

    /*36    Additional 5G security information    Additional 5G security information 9.11.3.12    O    TLV    3 */
    ELEM_OPT_TLV(0x36, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_ADD_5G_SEC_INF, NULL);
    /*78    EAP message    EAP message     9.10.2.2    O    TLV-E    7*/
    ELEM_OPT_TLV_E(0x78,  NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_EAP_MESSAGE, NULL);
    /*38    ABBA    ABBA 9.11.3.10    O    TLV    4-n */
    ELEM_OPT_TLV(0x38, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_ABBA, NULL);
    /*19    Replayed S1 UE security capabilities    S1 UE security capability 9.11.3.48A    O    TLV    4-7 */
    ELEM_OPT_TLV(0x19, NAS_PDU_TYPE_EMM, DE_EMM_UE_SEC_CAP, " - Replayed S1 UE security capabilities");

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.2.26 Security mode complete
 */
static void
nas_5gs_mm_sec_mode_comp(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* 77    IMEISV    5G mobile identity 9.11.3.4    O    TLV-E    11 */
    ELEM_OPT_TLV_E(0x77, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GS_MOBILE_ID, NULL);
    /* 71    NAS message container    NAS message container 9.11.3.33    O    TLV-E    4-n */
    ELEM_OPT_TLV_E(0x71, NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_NAS_MSG_CONT, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.2.27 Security mode reject
 */

static void
nas_5gs_mm_sec_mode_rej(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* 5GMM cause    5GMM cause 9.11.3.2    M    V    1 */
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GMM_CAUSE, NULL, ei_nas_5gs_missing_mandatory_elemen);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.2.28    Security protected 5GS NAS message
 */
/*
 * 8.2.29 5GMM status
 */

static void
nas_5gs_mm_5gmm_status(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: both*/
    /* 5GMM cause    5GMM cause 9.11.3.2    M    V    1 */
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_MM, DE_NAS_5GS_MM_5GMM_CAUSE, NULL, ei_nas_5gs_missing_mandatory_elemen);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/* 8.3 5GS session management messages */

/*
 * 8.3.1 PDU session establishment request
 */
static void
nas_5gs_sm_pdu_ses_est_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: UE to network */
    pinfo->link_dir = P2P_DIR_UL;

    /*9-    PDU session type    PDU session type     9.11.4.5    O    TV    1*/
    ELEM_OPT_TV_SHORT(0x90, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_PDU_SESSION_TYPE, NULL);

    /*A-    SSC mode    SSC mode     9.11.4.9    O    TV    1*/
    ELEM_OPT_TV_SHORT(0xa0, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_SSC_MODE, NULL);

    /*28    5GSM capability    5GSM capability     9.11.4.10    O    TLV    3-15 */
    ELEM_OPT_TLV(0x28, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_5GSM_CAP, NULL);

    /*55    Maximum number of supported packet filter    Maximum number of suuported packet filter   9.11.4.9    O    TV    3*/
    ELEM_OPT_TV(0x55, NAS_5GS_PDU_TYPE_SM,  DE_NAS_5GS_SM_MAX_NUM_SUP_PKT_FLT, NULL);

    /*39    SM PDU DN request container    SM PDU DN request container 9.11.4.15    O    TLV    3-255 */

    /*7B    Extended protocol configuration options    Extended protocol configuration options     9.11.4.2    O    TLV-E    4-65538*/
    ELEM_OPT_TLV_E(0x7B, NAS_PDU_TYPE_ESM, DE_ESM_EXT_PCO, NULL);



    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.3.2 PDU session establishment accept
 */
static void
nas_5gs_sm_pdu_ses_est_acc(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: network to UE */
    pinfo->link_dir = P2P_DIR_DL;

    proto_tree_add_item(tree, hf_nas_5gs_sm_sel_sc_mode, tvb, offset, 1, ENC_BIG_ENDIAN);
    /*Selected PDU session type    PDU session type 9.11.4.5    M    V    1/2 H0*/
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_PDU_SESSION_TYPE, " - Selected PDU session type", ei_nas_5gs_missing_mandatory_elemen);
    /*Selected SSC mode    SSC mode 9.11.4.9    M    V    1/2 H1*/

    /*Authorized QoS rules    QoS rules 9.11.4.6    M    LV-E    2-65537 DE_NAS_5GS_SM_QOS_RULES*/
    ELEM_MAND_LV_E(NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_QOS_RULES, " - Authorized QoS rules", ei_nas_5gs_missing_mandatory_elemen);
    /*Session AMBR    Session-AMBR 9.11.4.7    M    LV    7 */
    ELEM_MAND_LV(NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_SESSION_AMBR, NULL, ei_nas_5gs_missing_mandatory_elemen);
    /*59    5GSM cause    5GSM cause 9.11.4.2    O    TV    2*/
    ELEM_OPT_TV(0x59, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_5GSM_CAUSE, NULL);
    /*29    PDU address    PDU address 9.11.4.4    O    TLV    7 */
    ELEM_OPT_TLV(0x29, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_PDU_ADDRESS, NULL);
    /*56    RQ timer value    GPRS timer 9.10.2.3    O    TV    2*/
    ELEM_OPT_TV(0x56, GSM_A_PDU_TYPE_GM, DE_GPRS_TIMER, " - RQ timer value");
    /*22    S-NSSAI    S-NSSAI 9.11.3.37    O    TLV    3-6*/
    ELEM_OPT_TLV(0x22, NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_S_NSSAI, NULL);

    /* 8-    Always-on PDU session indication    Always-on PDU session indication 9.11.4.3    O    TV    1 */

    /* 75    Mapped EPS bearer contexts    Mapped EPS bearer contexts 9.11.4.9    O    TLV-E    7-65538 */
    ELEM_OPT_TLV_E(0x75, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_MAPPED_EPS_B_CONT, NULL);
    /*78    EAP message    EAP message 9.11.3.14    O    TLV-E    7-1503*/
    ELEM_OPT_TLV_E(0x78, NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_EAP_MESSAGE, NULL);
    /*79    Authorized QoS flow descriptions    QoS flow descriptions 9.11.4.12    O    TLV-E    6-65538 */
    ELEM_OPT_TLV_E(0x79, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_QOS_FLOW_DES, " - Authorized");
    /*7B    Extended protocol configuration options    Extended protocol configuration options 9.11.4.2    O    TLV-E    4-65538*/
    ELEM_OPT_TLV_E(0x7B, NAS_PDU_TYPE_ESM, DE_ESM_EXT_PCO, NULL);
    /* 25    DNN    DNN 9.11.2.1A    O    TLV    3-102 */
    ELEM_OPT_TLV(0x25, NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_DNN, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.3.3 PDU session establishment reject
 */

static void
nas_5gs_sm_pdu_ses_est_rej(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: network to UE */
    pinfo->link_dir = P2P_DIR_DL;

    /* 5GSM cause    5GSM cause 9.11.4.2    M    V    1 */
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_5GSM_CAUSE, " - ESM cause", ei_nas_5gs_missing_mandatory_elemen);

    /*37    Back-off timer value    GPRS timer 3 9.10.2.5    O    TLV    3 */
    ELEM_OPT_TLV(0x37, GSM_A_PDU_TYPE_GM, DE_GPRS_TIMER_3, " - Back-off timer value");

    /*F-    Allowed SSC mode    Allowed SSC mode 9.11.4.3    O    TV    1*/
    ELEM_OPT_TV_SHORT(0xF0, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_5GSM_ALLOWED_SSC_MODE, NULL);

    /*78    EAP message    EAP message 9.11.3.14    O    TLV - E    7 - 1503*/
    ELEM_OPT_TLV_E(0x78,  NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_EAP_MESSAGE, NULL);

    /*7B    Extended protocol configuration options    Extended protocol configuration options    9.11.4.2    O    TLV - E    4 - 65538*/
    ELEM_OPT_TLV_E(0x7B, NAS_PDU_TYPE_ESM, DE_ESM_EXT_PCO, NULL);


    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.3.4 PDU session authentication command
 */

static void
nas_5gs_sm_pdu_ses_auth_cmd(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: network to UE */
    pinfo->link_dir = P2P_DIR_DL;

    /*EAP message    EAP message 9.11.2.2    M    LV-E    6-1502 */
    ELEM_MAND_LV_E(NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_EAP_MESSAGE, NULL, ei_nas_5gs_missing_mandatory_elemen);

    /*7B    Extended protocol configuration options    Extended protocol configuration options    9.11.4.2    O    TLV - E    4 - 65538*/
    ELEM_OPT_TLV_E(0x7B, NAS_PDU_TYPE_ESM, DE_ESM_EXT_PCO, NULL);


    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}
/*
 * 8.3.5 PDU session authentication complete
 */

static void
nas_5gs_sm_pdu_ses_auth_comp(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: UE to network */
    pinfo->link_dir = P2P_DIR_UL;

    /*EAP message    EAP message 9.11.2.2    M    LV-E    6-1502 */
    ELEM_MAND_LV_E(NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_EAP_MESSAGE, NULL, ei_nas_5gs_missing_mandatory_elemen);

    /*7B    Extended protocol configuration options    Extended protocol configuration options    9.11.4.2    O    TLV - E    4 - 65538*/
    ELEM_OPT_TLV_E(0x7B, NAS_PDU_TYPE_ESM, DE_ESM_EXT_PCO, NULL);


    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.3.6 PDU session authentication result
 */
#if 0
static void
nas_5gs_sm_pdu_ses_auth_res(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: network to UE */
    pinfo->link_dir = P2P_DIR_DL;

    /*EAP message    EAP message 9.11.2.2    M    LV-E    6-1502 */
    ELEM_MAND_LV_E(NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_EAP_MESSAGE, NULL, ei_nas_5gs_missing_mandatory_elemen);

    /*7B    Extended protocol configuration options    Extended protocol configuration options    9.11.4.2    O    TLV - E    4 - 65538*/
    ELEM_OPT_TLV_E(0x7B, NAS_PDU_TYPE_ESM, DE_ESM_EXT_PCO, NULL);


    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}
#endif
/*
 *8.3.7 PDU session modification request
 */

static void
nas_5gs_sm_pdu_ses_mod_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: UE to network */
    pinfo->link_dir = P2P_DIR_UL;

    /* 28    5GSM capability    5GSM capability 9.11.4.10    O    TLV    3-15 */
    ELEM_OPT_TLV(0x28, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_5GSM_CAP, NULL);

    /* 59    5GSM cause    5GSM cause 9.11.4.2    O    TV    2 */
    ELEM_OPT_TLV(0x59, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_5GSM_CAUSE, NULL);

    /*55    Maximum number of suuported packet filter    Maximum number of suuported packet filter   9.11.4.6    O    TV    3*/
    ELEM_OPT_TV(0x55, NAS_5GS_PDU_TYPE_SM,  DE_NAS_5GS_SM_MAX_NUM_SUP_PKT_FLT, NULL);

    /* B-    Always-on PDU session requested    Always-on PDU session requested 9.11.4.4    O    TV    1 */
    /* 13    Integrity protection maximum data rate    Integrity protection maximum data rate 9.11.4.7    O    TV    3*/

    /*7A    Requested QoS rules    QoS rules 9.11.4.6    O    TLV-E    3-65538 */
    ELEM_OPT_TLV_E(0x7A, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_QOS_RULES, " - Requested QoS rules");

    /* 79    Requested QoS flow descriptions    QoS flow descriptions 9.11.4.12    O    TLV-E    5-65538 */
    /* 75    Mapped EPS bearer contexts    Mapped EPS bearer contexts 9.11.4.8    O    TLV-E    7-65538 */
    ELEM_OPT_TLV_E(0x75, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_MAPPED_EPS_B_CONT, NULL);
    /* 7B    Extended protocol configuration options    Extended protocol configuration options    9.11.4.2    O    TLV - E    4 - 65538*/
    ELEM_OPT_TLV_E(0x7B, NAS_PDU_TYPE_ESM, DE_ESM_EXT_PCO, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.3.8    PDU session modification reject
 */

static void
nas_5gs_sm_pdu_ses_mod_rej(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: network to UE */
    pinfo->link_dir = P2P_DIR_DL;

    /* 5GSM cause    5GSM cause 9.11.4.1    M    V    1 */
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_5GSM_CAUSE, NULL, ei_nas_5gs_missing_mandatory_elemen);

    /*37    Back-off timer value    GPRS timer 3 9.11.3.21    O    TLV    3 */
    ELEM_OPT_TLV(0x37, GSM_A_PDU_TYPE_GM, DE_GPRS_TIMER_3, " - Back-off timer value");

    /*7B    Extended protocol configuration options    Extended protocol configuration options    9.11.4.2    O    TLV - E    4 - 65538*/
    ELEM_OPT_TLV_E(0x7B, NAS_PDU_TYPE_ESM, DE_ESM_EXT_PCO, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
* 8.3.9 PDU session modification command
*/

static void
nas_5gs_sm_pdu_ses_mod_cmd(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: network to UE */
    pinfo->link_dir = P2P_DIR_DL;

    /*59    5GSM cause    5GSM cause 9.11.4.2    O    TV    2*/
    ELEM_OPT_TV(0x59, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_5GSM_CAUSE, NULL);
    /*2A    Session AMBR    Session-AMBR     9.11.4.7    O    TLV    8*/
    ELEM_OPT_TLV(0x2A, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_SESSION_AMBR, NULL);
    /*56    RQ timer value    GPRS timer     9.11.4.3    O    TV    2*/
    ELEM_OPT_TV(0x56, GSM_A_PDU_TYPE_GM, DE_GPRS_TIMER, " - PDU session release time");
    /* 8-   Always-on PDU session indication    Always-on PDU session indication 9.11.4.3    O    TV    1 */

    /*7A    Authorized QoS rules    QoS rules     9.11.4.6    O    TLV-E    3-65538*/
    ELEM_OPT_TLV_E(0x7A, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_QOS_RULES, " - Authorized QoS rules");
    /*75    Mapped EPS bearer contexts     Mapped EPS  bearer contexts     9.11.4.5    O    TLV-E    7-65538*/
    ELEM_OPT_TLV_E(0x75, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_MAPPED_EPS_B_CONT, NULL);
    /*79    Authorized QoS flow descriptions     QoS flow descriptions     9.11.4.12    O    TLV-E    6-65538*/
    ELEM_OPT_TLV_E(0x79, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_QOS_FLOW_DES, " - Authorized");
    /*7B    Extended protocol configuration options    Extended protocol configuration options     9.11.4.2    O    TLV-E    4-65538*/
    ELEM_OPT_TLV_E(0x7B, NAS_PDU_TYPE_ESM, DE_ESM_EXT_PCO, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.3.10 PDU session modification complete
 */

static void
nas_5gs_sm_pdu_ses_mod_comp(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: UE to network */
    pinfo->link_dir = P2P_DIR_UL;

    /*7B    Extended protocol configuration options    Extended protocol configuration options    9.11.4.2    O    TLV - E    4 - 65538*/
    ELEM_OPT_TLV_E(0x7B, NAS_PDU_TYPE_ESM, DE_ESM_EXT_PCO, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.3.11 PDU session modification command reject
 */

static void
nas_5gs_sm_pdu_ses_mod_com_rej(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: UE to network */
    pinfo->link_dir = P2P_DIR_UL;

    /* 5GSM cause    5GSM cause 9.11.4.1    M    V    1 */
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_5GSM_CAUSE, NULL, ei_nas_5gs_missing_mandatory_elemen);

    /*7B    Extended protocol configuration options    Extended protocol configuration options    9.11.4.2    O    TLV - E    4 - 65538*/
    ELEM_OPT_TLV_E(0x7B, NAS_PDU_TYPE_ESM, DE_ESM_EXT_PCO, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.3.12 PDU session release request
 */

static void
nas_5gs_sm_pdu_ses_rel_req(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: UE to network */
    pinfo->link_dir = P2P_DIR_UL;

    /* 59    5GSM cause    5GSM cause 9.11.4.2    O    TV    2 */
    ELEM_OPT_TV(0x59, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_5GSM_CAUSE, NULL);
    /* 7B    Extended protocol configuration options    Extended protocol configuration options    9.11.4.2    O    TLV - E    4 - 65538*/
    ELEM_OPT_TLV_E(0x7B, NAS_PDU_TYPE_ESM, DE_ESM_EXT_PCO, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.3.13 PDU session release reject
 */

static void
nas_5gs_sm_pdu_ses_rel_rej(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: network to UE */
    pinfo->link_dir = P2P_DIR_DL;

    /* 5GSM cause    5GSM cause 9.11.4.1    M    V    1 */
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_5GSM_CAUSE, NULL, ei_nas_5gs_missing_mandatory_elemen);

    /*7B    Extended protocol configuration options    Extended protocol configuration options    9.11.4.2    O    TLV - E    4 - 65538*/
    ELEM_OPT_TLV_E(0x7B, NAS_PDU_TYPE_ESM, DE_ESM_EXT_PCO, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.3.14 PDU session release command
 */

static void
nas_5gs_sm_pdu_ses_rel_cmd(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: network to UE */
    pinfo->link_dir = P2P_DIR_DL;

    /* 5GSM cause    5GSM cause 9.11.4.2    M    V    1 */
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_5GSM_CAUSE, NULL, ei_nas_5gs_missing_mandatory_elemen);

    /*37    Back-off timer value    GPRS timer 3 9.11.3.21    O    TLV    3 */
    ELEM_OPT_TLV(0x37, GSM_A_PDU_TYPE_GM, DE_GPRS_TIMER_3, " - Back-off timer value");

    /*78    EAP message    EAP message 9.10.2.2    O    TLV - E    7 - 1503*/
    ELEM_OPT_TLV_E(0x78,  NAS_5GS_PDU_TYPE_COMMON, DE_NAS_5GS_CMN_EAP_MESSAGE, NULL);

    /*7B    Extended protocol configuration options    Extended protocol configuration options    9.11.4.2    O    TLV - E    4 - 65538*/
    ELEM_OPT_TLV_E(0x7B, NAS_PDU_TYPE_ESM, DE_ESM_EXT_PCO, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
* 8.3.15 PDU session release complete
*/

static void
nas_5gs_sm_pdu_ses_rel_comp(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: UE to network */
    pinfo->link_dir = P2P_DIR_UL;

    /* 59    5GSM cause    5GSM cause 9.11.4.2    O    TV    2 */
    ELEM_OPT_TV(0x59, NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_5GSM_CAUSE, NULL);
    /*7B    Extended protocol configuration options    Extended protocol configuration options    9.11.4.2    O    TLV - E    4 - 65538*/
    ELEM_OPT_TLV_E(0x7B, NAS_PDU_TYPE_ESM, DE_ESM_EXT_PCO, NULL);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}

/*
 * 8.3.16 5GSM status
 */

static void
nas_5gs_sm_5gsm_status(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_, guint32 offset, guint len)
{
    guint32 curr_offset;
    guint32 consumed;
    guint   curr_len;

    curr_offset = offset;
    curr_len = len;

    /* Direction: both */
    /* 5GSM cause    5GSM cause 9.11.4.1    M    V    1 */
    ELEM_MAND_V(NAS_5GS_PDU_TYPE_SM, DE_NAS_5GS_SM_5GSM_CAUSE, NULL, ei_nas_5gs_missing_mandatory_elemen);

    EXTRANEOUS_DATA_CHECK(curr_len, 0, pinfo, &ei_nas_5gs_extraneous_data);

}


/* 9.7  Message type */

/* 5GS mobility management messages */
static const value_string nas_5gs_mm_message_type_vals[] = {
    { 0x41,    "Registration request"},
    { 0x42,    "Registration accept"},
    { 0x43,    "Registration complete"},
    { 0x44,    "Registration reject"},
    { 0x45,    "Deregistration request (UE originating)"},
    { 0x46,    "Deregistration accept (UE originating)"},
    { 0x47,    "Deregistration request (UE terminated)"},
    { 0x48,    "Deregistration accept (UE terminated)"},

    { 0x49,    "Not used in current version"},
    { 0x4a,    "Not used in current version" },
    { 0x4b,    "Not used in current version" },

    { 0x4c,    "Service request"},
    { 0x4d,    "Service reject"},
    { 0x4e,    "Service accept"},

    { 0x4f,    "Not used in current version" },
    { 0x50,    "Not used in current version" },
    { 0x51,    "Not used in current version" },
    { 0x52,    "Not used in current version" },
    { 0x53,    "Not used in current version" },

    { 0x54,    "Configuration update command"},
    { 0x55,    "Configuration update complete"},
    { 0x56,    "Authentication request"},
    { 0x57,    "Authentication response"},
    { 0x58,    "Authentication reject"},
    { 0x59,    "Authentication failure"},

    { 0x5a,    "Authentication result"},
    { 0x5b,    "Identity request"},
    { 0x5c,    "Identity response"},
    { 0x5d,    "Security mode command"},
    { 0x5e,    "Security mode complete"},
    { 0x5f,    "Security mode reject"},

    { 0x60,    "Not used in current version" },
    { 0x61,    "Not used in current version" },
    { 0x62,    "Not used in current version" },
    { 0x63,    "Not used in current version" },
    { 0x64,    "5GMM status"},
    { 0x65,    "Notification"},
    { 0x66,    "Notification response" },
    { 0x67,    "UL NAS transport"},
    { 0x68,    "DL NAS transport"},
    { 0,    NULL }
};

static value_string_ext nas_5gs_mm_msg_strings_ext = VALUE_STRING_EXT_INIT(nas_5gs_mm_message_type_vals);

#define NUM_NAS_5GS_MM_MSG (sizeof(nas_5gs_mm_message_type_vals)/sizeof(value_string))
static gint ett_nas_5gs_mm_msg[NUM_NAS_5GS_MM_MSG];
static void(*nas_5gs_mm_msg_fcn[])(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len) = {
    nas_5gs_mm_registration_req,                /* 0x41    Registration request */
    nas_5gs_mm_registration_accept,             /* 0x42    Registration accept */
    nas_5gs_mm_registration_complete,           /* 0x43    Registration complete */
    nas_5gs_mm_registration_rej,                /* 0x44    Registration reject */
    nas_5gs_mm_de_reg_req_ue_orig,              /* 0x45    Deregistration request (UE originating) */
    NULL,                                       /* 0x46    Deregistration accept (UE originating) No data*/
    nas_5gs_mm_de_registration_req_ue_term,     /* 0x47    Deregistration request (UE terminated) */
    NULL,                                       /* 0x48    Deregistration accept (UE terminated) No data */

    nas_5gs_exp_not_dissected_yet,              /* 0x49    Not used in current version */
    nas_5gs_exp_not_dissected_yet,              /* 0x4a    Not used in current version */
    nas_5gs_exp_not_dissected_yet,              /* 0x4b    Not used in current version */

    nas_5gs_mm_service_req,                     /* 0x4c    Service request */
    nas_5gs_mm_service_rej,                     /* 0x4d    Service reject */
    nas_5gs_mm_service_acc,                     /* 0x4e    Service accept */

    nas_5gs_exp_not_dissected_yet,              /* 0x4f    Not used in current version */
    nas_5gs_exp_not_dissected_yet,              /* 0x50    Not used in current version */
    nas_5gs_exp_not_dissected_yet,              /* 0x51    Not used in current version */
    nas_5gs_exp_not_dissected_yet,              /* 0x52    Not used in current version */
    nas_5gs_exp_not_dissected_yet,              /* 0x53    Not used in current version */

    nas_5gs_mm_conf_upd_cmd,                    /* 0x54    Configuration update command */
    nas_5gs_mm_conf_update_comp,                /* 0x55    Configuration update complete */
    nas_5gs_mm_authentication_req,              /* 0x56    Authentication request */
    nas_5gs_mm_authentication_resp,             /* 0x57    Authentication response */
    nas_5gs_mm_authentication_rej,              /* 0x58    Authentication reject */
    nas_5gs_mm_authentication_failure,          /* 0x59    Authentication failure */
    nas_5gs_mm_authentication_result,           /* 0x5a    Authentication result */
    nas_5gs_mm_id_req,                          /* 0x5b    Identity request */
    nas_5gs_mm_id_resp,                         /* 0x5c    Identity response */
    nas_5gs_mm_sec_mode_cmd,                    /* 0x5d    Security mode command */
    nas_5gs_mm_sec_mode_comp,                   /* 0x5e    Security mode complete */
    nas_5gs_mm_sec_mode_rej,                    /* 0x5f    Security mode reject */
    nas_5gs_exp_not_dissected_yet,              /* 0x60    Not used in current version */
    nas_5gs_exp_not_dissected_yet,              /* 0x61    Not used in current version */
    nas_5gs_exp_not_dissected_yet,              /* 0x62    Not used in current version */
    nas_5gs_exp_not_dissected_yet,              /* 0x63    Not used in current version */

    nas_5gs_mm_5gmm_status,                     /* 0x64    5GMM status */
    nas_5gs_mm_notification,                    /* 0x65    Notification */
    nas_5gs_mm_notification_resp,               /* 0x66    Notification */
    nas_5gs_mm_ul_nas_transp,                   /* 0x67    UL NAS transport */
    nas_5gs_mm_dl_nas_transp,                   /* 0x68    DL NAS transport */
    NULL,   /* NONE */

};


    /* 5GS session management messages */
    static const value_string nas_5gs_sm_message_type_vals[] = {

    { 0xc1,    "PDU session establishment request"},
    { 0xc2,    "PDU session establishment accept"},
    { 0xc3,    "PDU session establishment reject"},

    { 0xc4,    "Not used in current version"},
    { 0xc5,    "PDU session authentication command"},

    { 0xc6,    "PDU session authentication complete" },
    { 0xc7,    "PDU session authentication result" },
    { 0xc8,    "Not used in current version" },

    { 0xc9,    "PDU session modification request"},
    { 0xca,    "PDU session modification reject"},
    { 0xcb,    "PDU session modification command"},
    { 0xcc,    "PDU session modification complete" },
    { 0xcd,    "PDU session modification command reject"},

    { 0xce,    "Not used in current version" },
    { 0xcf,    "Not used in current version" },
    { 0xd0,    "Not used in current version" },

    { 0xd1,    "PDU session release request"},
    { 0xd2,    "PDU session release reject"},
    { 0xd3,    "PDU session release command"},
    { 0xd4,    "PDU session release complete"},

    { 0xd5,    "Not used in current version" },

    { 0xd6,    "5GSM status"},
    { 0,    NULL }
};
static value_string_ext nas_5gs_sm_msg_strings_ext = VALUE_STRING_EXT_INIT(nas_5gs_sm_message_type_vals);

#define NUM_NAS_5GS_SM_MSG (sizeof(nas_5gs_sm_message_type_vals)/sizeof(value_string))
static gint ett_nas_5gs_sm_msg[NUM_NAS_5GS_SM_MSG];

static void(*nas_5gs_sm_msg_fcn[])(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len) = {
    nas_5gs_sm_pdu_ses_est_req,            /* 0xc1     PDU session establishment request */
    nas_5gs_sm_pdu_ses_est_acc,            /* 0xc2     PDU session establishment accept */
    nas_5gs_sm_pdu_ses_est_rej,            /* 0xc3     PDU session establishment reject */

    nas_5gs_exp_not_dissected_yet,         /* 0xc4     Not used in current version */
    nas_5gs_sm_pdu_ses_auth_cmd,           /* 0xc5     PDU session authentication command */

    nas_5gs_sm_pdu_ses_auth_comp,          /* 0xc6     PDU session authentication complete */
    nas_5gs_exp_not_dissected_yet,         /* 0xc7     Not used in current version */
    nas_5gs_exp_not_dissected_yet,         /* 0xc8     Not used in current version */

    nas_5gs_sm_pdu_ses_mod_req,            /* 0xc9     PDU session modification request */
    nas_5gs_sm_pdu_ses_mod_rej,            /* 0xca     PDU session modification reject */
    nas_5gs_sm_pdu_ses_mod_cmd,            /* 0xcb     PDU session modification command */
    nas_5gs_sm_pdu_ses_mod_comp,           /* 0xcc     PDU session modification complete */
    nas_5gs_sm_pdu_ses_mod_com_rej,        /* 0xcd     PDU session modification command reject */

    nas_5gs_exp_not_dissected_yet,         /* 0xce     Not used in current version */
    nas_5gs_exp_not_dissected_yet,         /* 0xcf     Not used in current version */
    nas_5gs_exp_not_dissected_yet,         /* 0xd0     Not used in current version */

    nas_5gs_sm_pdu_ses_rel_req,            /* 0xd1     PDU session release request */
    nas_5gs_sm_pdu_ses_rel_rej,            /* 0xd2     PDU session release reject */
    nas_5gs_sm_pdu_ses_rel_cmd,            /* 0xd3     PDU session release command */
    nas_5gs_sm_pdu_ses_rel_comp,           /* 0xd4     PDU session release complete */

    nas_5gs_exp_not_dissected_yet,         /* 0xd5     Not used in current version */

    nas_5gs_sm_5gsm_status,                /* 0xd6     5GSM status */

    NULL,   /* NONE */

};



static void
get_nas_5gsmm_msg_params(guint8 oct, const gchar **msg_str, int *ett_tree, int *hf_idx, msg_fcn *msg_fcn_p)
{
    gint            idx;

    *msg_str = try_val_to_str_idx_ext((guint32)(oct & 0xff), &nas_5gs_mm_msg_strings_ext, &idx);
    *hf_idx = hf_nas_5gs_mm_msg_type;
    if (*msg_str != NULL) {
        *ett_tree = ett_nas_5gs_mm_msg[idx];
        *msg_fcn_p = nas_5gs_mm_msg_fcn[idx];
    }

    return;
}

static void
get_nas_5gssm_msg_params(guint8 oct, const gchar **msg_str, int *ett_tree, int *hf_idx, msg_fcn *msg_fcn_p)
{
    gint            idx;

    *msg_str = try_val_to_str_idx_ext((guint32)(oct & 0xff), &nas_5gs_sm_msg_strings_ext, &idx);
    *hf_idx = hf_nas_5gs_sm_msg_type;
    if (*msg_str != NULL) {
        *ett_tree = ett_nas_5gs_sm_msg[idx];
        *msg_fcn_p = nas_5gs_sm_msg_fcn[idx];
    }

    return;
}
static void
dissect_nas_5gs_sm_msg(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset)
{
    const gchar *msg_str;
    guint32      len;
    gint         ett_tree;
    int          hf_idx;
    void(*msg_fcn_p)(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len);
    guint8       oct;

    len = tvb_reported_length(tvb);

    /* Message type IE*/
    oct = tvb_get_guint8(tvb, offset);
    msg_fcn_p = NULL;
    ett_tree = -1;
    hf_idx = -1;
    msg_str = NULL;

    get_nas_5gssm_msg_params(oct, &msg_str, &ett_tree, &hf_idx, &msg_fcn_p);

    if (msg_str) {
        col_append_sep_str(pinfo->cinfo, COL_INFO, NULL, msg_str);
    }
    else {
        proto_tree_add_expert_format(tree, pinfo, &ei_nas_5gs_sm_unknown_msg_type, tvb, offset, 1, "Unknown Message Type 0x%02x", oct);
        return;
    }

    /*
    * Add NAS message name
    */
    proto_tree_add_item(tree, hf_idx, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /*
    * decode elements
    */
    if (msg_fcn_p == NULL)
    {
        if (tvb_reported_length_remaining(tvb, offset)) {
            proto_tree_add_item(tree, hf_nas_5gs_msg_elems, tvb, offset, len - offset, ENC_NA);
        }
    }
    else
    {
        (*msg_fcn_p)(tvb, tree, pinfo, offset, len - offset);
    }

}

static void
disect_nas_5gs_mm_msg(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset)
{

    const gchar *msg_str;
    guint32      len;
    gint         ett_tree;
    int          hf_idx;
    void(*msg_fcn_p)(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo, guint32 offset, guint len);
    guint8       oct;

    len = tvb_reported_length(tvb);

    /* Message type IE*/
    oct = tvb_get_guint8(tvb, offset);
    msg_fcn_p = NULL;
    ett_tree = -1;
    hf_idx = -1;
    msg_str = NULL;

    get_nas_5gsmm_msg_params(oct, &msg_str, &ett_tree, &hf_idx, &msg_fcn_p);

    if (msg_str) {
        col_append_sep_str(pinfo->cinfo, COL_INFO, NULL, msg_str);
    }
    else {
        proto_tree_add_expert_format(tree, pinfo, &ei_nas_5gs_mm_unknown_msg_type, tvb, offset, 1, "Unknown Message Type 0x%02x", oct);
        return;
    }

    /*
    * Add NAS message name
    */
    proto_tree_add_item(tree, hf_idx, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /*
    * decode elements
    */
    if (msg_fcn_p == NULL)
    {
        if (tvb_reported_length_remaining(tvb, offset)) {
            proto_tree_add_item(tree, hf_nas_5gs_msg_elems, tvb, offset, len - offset, ENC_NA);
        }
    }
    else
    {
        (*msg_fcn_p)(tvb, tree, pinfo, offset, len - offset);
    }

}

const value_string nas_5gs_pdu_session_id_vals[] = {
    { 0x00, "No PDU session identity assigned" },
    { 0x01, "Reserved" },
    { 0x02, "Reserved" },
    { 0x03, "Reserved" },
    { 0x04, "Reserved" },
    { 0x05, "PDU session identity value 5" },
    { 0x06, "PDU session identity value 6" },
    { 0x07, "PDU session identity value 7" },
    { 0x08, "PDU session identity value 8" },
    { 0x09, "PDU session identity value 9" },
    { 0x0a, "PDU session identity value 10" },
    { 0x0b, "PDU session identity value 11" },
    { 0x0c, "PDU session identity value 12" },
    { 0x0d, "PDU session identity value 13" },
    { 0x0e, "PDU session identity value 14" },
    { 0x0f, "PDU session identity value 15" },
    { 0, NULL }
};

static int
dissect_nas_5gs_common(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, void* data _U_)
{
    proto_tree *sub_tree;
    guint32 epd;

    /* Plain NAS 5GS Message */
    sub_tree = proto_tree_add_subtree(tree, tvb, offset, -1, ett_nas_5gs_plain, NULL, "Plain NAS 5GS Message");
    /* Extended protocol discriminator  octet 1 */
    proto_tree_add_item_ret_uint(sub_tree, hf_nas_5gs_epd, tvb, offset, 1, ENC_BIG_ENDIAN, &epd);
    offset++;
    /* Security header type associated with a spare half octet; or
     * PDU session identity octet 2
     */
    switch (epd) {
    case TGPP_PD_5GMM:
        /* 9.5  Spare half octet
        * Bits 5 to 8 of the second octet of every 5GMM message contains the spare half octet
        * which is filled with spare bits set to zero.
        */
        proto_tree_add_item(sub_tree, hf_nas_5gs_spare_half_octet, tvb, offset, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(sub_tree, hf_nas_5gs_security_header_type, tvb, offset, 1, ENC_BIG_ENDIAN);
        break;
    case TGPP_PD_5GSM:
        /* 9.4  PDU session identity
        * Bits 1 to 8 of the second octet of every 5GSM message contain the PDU session identity IE.
        * The PDU session identity and its use to identify a message flow are defined in 3GPP TS 24.007
        */
        proto_tree_add_item(sub_tree, hf_nas_5gs_pdu_session_id, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;
        /* 9.6  Procedure transaction identity
        * Bits 1 to 8 of the third octet of every 5GSM message contain the procedure transaction identity.
        * The procedure transaction identity and its use are defined in 3GPP TS 24.007
        * XXX Only 5GSM ?
        */
        proto_tree_add_item(sub_tree, hf_nas_5gs_proc_trans_id, tvb, offset, 1, ENC_BIG_ENDIAN);
        break;
    default:
        proto_tree_add_expert_format(sub_tree, pinfo, &ei_nas_5gs_unknown_pd, tvb, offset, -1, "Not a NAS 5GS PD %u (%s)",
            epd, val_to_str_const(epd, nas_5gs_epd_vals, "Unknown"));
        return 0;

    }
    offset++;

    switch (epd) {
    case TGPP_PD_5GMM:
        /* 5GS mobility management messages */
        disect_nas_5gs_mm_msg(tvb, pinfo, sub_tree, offset);
        break;
    case TGPP_PD_5GSM:
        /* 5GS session management messages. */
        dissect_nas_5gs_sm_msg(tvb, pinfo, sub_tree, offset);
        break;
    default:
        DISSECTOR_ASSERT_NOT_REACHED();
        break;
    }

    return tvb_reported_length(tvb);
}

static int
dissect_nas_5gs(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data)
{
    proto_item *item;
    proto_tree *nas_5gs_tree, *sub_tree;
    int offset = 0;
    guint8 seq_hdr_type, ext_pd;

    /* make entry in the Protocol column on summary display */
    col_append_sep_str(pinfo->cinfo, COL_PROTOCOL, "/", "NAS-5GS");

    item = proto_tree_add_item(tree, proto_nas_5gs, tvb, 0, -1, ENC_NA);
    nas_5gs_tree = proto_item_add_subtree(item, ett_nas_5gs);

    /* Extended protocol discriminator                              octet 1 */
    ext_pd = tvb_get_guint8(tvb, offset);
    if (ext_pd == TGPP_PD_5GSM) {
        return dissect_nas_5gs_common(tvb, pinfo, nas_5gs_tree, offset, data);
    }
    /* Security header type associated with a spare half octet; or
    * PDU session identity                                         octet 2 */
    /* Determine if it's a plain 5GS NAS Message or not */
    seq_hdr_type = tvb_get_guint8(tvb, offset + 1);
    if (seq_hdr_type == NAS_5GS_PLAN_NAS_MSG) {
        return dissect_nas_5gs_common(tvb, pinfo, nas_5gs_tree, offset, data);
    }
    /* Security protected NAS 5GS message*/
    sub_tree = proto_tree_add_subtree(nas_5gs_tree, tvb, offset, 7, ett_nas_5gs_sec, NULL, "Security protected NAS 5GS message");

    /* Extended protocol discriminator  octet 1 */
    proto_tree_add_item(sub_tree, hf_nas_5gs_epd, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;
    /* Security header type associated with a spare half octet    octet 2 */
    proto_tree_add_item(sub_tree, hf_nas_5gs_spare_half_octet, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(sub_tree, hf_nas_5gs_security_header_type, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;
    /* Message authentication code octet 3 - 6 */
    proto_tree_add_item(sub_tree, hf_nas_5gs_msg_auth_code, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset += 4;
    /* Sequence number    octet 7 */
    proto_tree_add_item(sub_tree, hf_nas_5gs_seq_no, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /* XXX Check if encryted or not and if not call dissect_nas_5gs_common()*/
    if (g_nas_5gs_null_decipher) {
        return dissect_nas_5gs_common(tvb, pinfo, nas_5gs_tree, offset, data);
    } else {
        proto_tree_add_subtree(nas_5gs_tree, tvb, offset, -1, ett_nas_5gs_enc, NULL, "Encrypted data");
    }

    return tvb_reported_length(tvb);
}

static true_false_string nas_5gs_kacf_tfs = {
    "A new K_AMF has been calculated by the network",
    "A new K_AMF has not been calculated by the network"
};

void
de_nas_5gs_intra_n1_mode_nas_transparent_cont(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_)
{
    int offset = 0;

    static const int * flags[] = {
        &hf_nas_5gs_spare_b7,
        &hf_nas_5gs_spare_b6,
        &hf_nas_5gs_spare_b5,
        &hf_nas_5gs_kacf,
        &hf_nas_5gs_mm_tsc,
        &hf_nas_5gs_mm_nas_key_set_id,
        NULL
    };

    proto_tree_add_item(tree, hf_nas_5gs_msg_auth_code, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset += 4;
    proto_tree_add_item(tree, hf_nas_5gs_mm_nas_sec_algo_enc, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_nas_5gs_mm_nas_sec_algo_ip, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;
    proto_tree_add_bitmask_list(tree, tvb, offset, 1, flags, ENC_NA);
    offset++;
    proto_tree_add_item(tree, hf_nas_5gs_seq_no, tvb, offset, 1, ENC_BIG_ENDIAN);
}

void
de_nas_5gs_n1_mode_to_s1_mode_nas_transparent_cont(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_)
{
    proto_tree_add_item(tree, hf_nas_5gs_seq_no, tvb, 0, 1, ENC_BIG_ENDIAN);
}

void
de_nas_5gs_s1_mode_to_n1_mode_nas_transparent_cont(tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo _U_)
{
    int offset = 0;

    static const int * oct8_flags[] = {
        &hf_nas_5gs_spare_b7,
        &hf_nas_5gs_ncc,
        &hf_nas_5gs_mm_tsc,
        &hf_nas_5gs_mm_nas_key_set_id,
        NULL
    };

    static const int * oct9_flags[] = {
        &hf_nas_5gs_mm_5g_ea0,
        &hf_nas_5gs_mm_128_5g_ea1,
        &hf_nas_5gs_mm_128_5g_ea2,
        &hf_nas_5gs_mm_128_5g_ea3,
        &hf_nas_5gs_mm_5g_ea4,
        &hf_nas_5gs_mm_5g_ea5,
        &hf_nas_5gs_mm_5g_ea6,
        &hf_nas_5gs_mm_5g_ea7,
        NULL
    };

    static const int * oct10_flags[] = {
        &hf_nas_5gs_mm_5g_ia0,
        &hf_nas_5gs_mm_5g_128_ia1,
        &hf_nas_5gs_mm_5g_128_ia2,
        &hf_nas_5gs_mm_5g_128_ia3,
        &hf_nas_5gs_mm_5g_ia4,
        &hf_nas_5gs_mm_5g_ia5,
        &hf_nas_5gs_mm_5g_ia6,
        &hf_nas_5gs_mm_5g_ia7,
        NULL
    };

    static const int * oct11_flags[] = {
        &hf_nas_5gs_mm_eea0,
        &hf_nas_5gs_mm_128eea1,
        &hf_nas_5gs_mm_128eea2,
        &hf_nas_5gs_mm_eea3,
        &hf_nas_5gs_mm_eea4,
        &hf_nas_5gs_mm_eea5,
        &hf_nas_5gs_mm_eea6,
        &hf_nas_5gs_mm_eea7,
        NULL
    };

    static const int * oct12_flags[] = {
        &hf_nas_5gs_mm_eia0,
        &hf_nas_5gs_mm_128eia1,
        &hf_nas_5gs_mm_128eia2,
        &hf_nas_5gs_mm_eia3,
        &hf_nas_5gs_mm_eia4,
        &hf_nas_5gs_mm_eia5,
        &hf_nas_5gs_mm_eia6,
        &hf_nas_5gs_mm_eia7,
        NULL
    };

    proto_tree_add_item(tree, hf_nas_5gs_msg_auth_code, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset += 4;
    proto_tree_add_item(tree, hf_nas_5gs_mm_nas_sec_algo_enc, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_nas_5gs_mm_nas_sec_algo_ip, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;
    proto_tree_add_bitmask_list(tree, tvb, offset, 1, oct8_flags, ENC_NA);
    offset++;
    proto_tree_add_bitmask_list(tree, tvb, offset, 1, oct9_flags, ENC_NA);
    offset++;
    proto_tree_add_bitmask_list(tree, tvb, offset, 1, oct10_flags, ENC_NA);
    offset++;
    if (tvb_reported_length_remaining(tvb, offset) > 0) {
        proto_tree_add_bitmask_list(tree, tvb, offset, 1, oct11_flags, ENC_NA);
        offset++;
        proto_tree_add_bitmask_list(tree, tvb, offset, 1, oct12_flags, ENC_NA);
    }
}

void
proto_register_nas_5gs(void)
{

    /* List of fields */

    static hf_register_info hf[] = {
        { &hf_nas_5gs_epd,
        { "Extended protocol discriminator",   "nas_5gs.epd",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_epd_vals), 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_spare_b7,
        { "Spare",   "nas_5gs.spare_b7",
            FT_UINT8, BASE_DEC, NULL, 0x80,
            NULL, HFILL }
        },
        { &hf_nas_5gs_spare_b6,
        { "Spare",   "nas_5gs.spare_b6",
            FT_UINT8, BASE_DEC, NULL, 0x40,
            NULL, HFILL }
        },
        { &hf_nas_5gs_spare_b5,
        { "Spare",   "nas_5gs.spare_b5",
            FT_UINT8, BASE_DEC, NULL, 0x20,
            NULL, HFILL }
        },
        { &hf_nas_5gs_spare_b4,
        { "Spare",   "nas_5gs.spare_b4",
            FT_UINT8, BASE_DEC, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_spare_b3,
        { "Spare",   "nas_5gs.spare_b3",
            FT_UINT8, BASE_DEC, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_spare_b2,
        { "Spare",   "nas_5gs.spare_b2",
            FT_UINT8, BASE_DEC, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_spare_b1,
        { "Spare",   "nas_5gs.spare_b1",
            FT_UINT8, BASE_DEC, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_rfu_b2,
        { "Reserved for Future Use(RFU)",   "nas_5gs.rfu.b2",
            FT_UINT8, BASE_DEC, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_rfu_b1,
        { "Reserved for Future Use(RFU)",   "nas_5gs.rfu.b1",
            FT_UINT8, BASE_DEC, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_rfu_b0,
        { "Reserved for Future Use(RFU)",   "nas_5gs.rfu.b0",
            FT_UINT8, BASE_DEC, NULL, 0x01,
            NULL, HFILL }
        },

        { &hf_nas_5gs_security_header_type,
        { "Security header type",   "nas_5gs.security_header_type",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_security_header_type_vals), 0x0f,
            NULL, HFILL }
        },
        { &hf_nas_5gs_msg_auth_code,
        { "Message authentication code",   "nas_5gs.msg_auth_code",
            FT_UINT32, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_seq_no,
        { "Sequence number",   "nas_5gs.seq_no",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_msg_type,
        { "Message type",   "nas_5gs.mm.message_type",
        FT_UINT8, BASE_HEX | BASE_EXT_STRING, &nas_5gs_mm_msg_strings_ext, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_msg_type,
        { "Message type",   "nas_5gs.sm.message_type",
        FT_UINT8, BASE_HEX | BASE_EXT_STRING, &nas_5gs_sm_msg_strings_ext, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_common_elem_id,
            { "Element ID", "nas_5gs.common.elem_id",
            FT_UINT8, BASE_HEX, NULL, 0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_elem_id,
            { "Element ID", "nas_5gs.mm.elem_id",
            FT_UINT8, BASE_HEX, NULL, 0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_elem_id,
            { "Element ID", "nas_5gs.sm.elem_id",
            FT_UINT8, BASE_HEX, NULL, 0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_proc_trans_id,
        { "Procedure transaction identity",   "nas_5gs.proc_trans_id",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_spare_half_octet,
        { "Spare Half Octet",   "nas_5gs.spare_half_octet",
            FT_UINT8, BASE_DEC, NULL, 0xf0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_session_id,
        { "PDU session identity",   "nas_5gs.pdu_session_id",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_pdu_session_id_vals), 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_msg_elems,
        { "Message Elements", "nas_5gs.message_elements",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_cmn_dnn,
        { "DNN", "nas_5gs.cmn.dnn",
            FT_STRING, BASE_NONE, NULL,0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_for,
        { "Follow-On Request bit (FOR)",   "nas_5gs.mm.for",
            FT_BOOLEAN, 8, TFS(&nas_5gs_for_tfs), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_sms_requested,
        { "SMS requested",   "nas_5gs.mm.sms_requested",
            FT_BOOLEAN, 8, TFS(&tfs_nas5gs_sms_requested), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_5gs_reg_type,
        { "5GS registration type",   "nas_5gs.mm.5gs_reg_type",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_registration_type_values), 0x07,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_tsc,
        { "Type of security context flag (TSC)",   "nas_5gs.mm.tsc",
            FT_BOOLEAN, 8, TFS(&nas_5gs_mm_tsc_tfs), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_nas_key_set_id,
        { "NAS key set identifier",   "nas_5gs.mm.nas_key_set_id",
            FT_UINT8, BASE_DEC, NULL, 0x07,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_5gmm_cause,
        { "5GMM cause",   "nas_5gs.mm.5gmm_cause",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_mm_cause_vals), 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_pld_cont_type,
        { "Payload container type",   "nas_5gs.mm.pld_cont_type",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_mm_pld_cont_type_vals), 0x0f,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_sst,
        { "Slice/service type (SST)",   "nas_5gs.mm.sst",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_sd,
        { "Slice differentiator (SD)",   "nas_5gs.mm.mm_sd",
            FT_UINT24, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_mapped_conf_sst,
        { "Mapped configured SST",   "nas_5gs.mm.mapped_conf_sst",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_mapped_conf_ssd,
        { "Mapped configured SD",   "nas_5gs.mm.mapped_conf_ssd",
            FT_UINT24, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_switch_off,
        { "Switch off",   "nas_5gs.mm.switch_off",
            FT_BOOLEAN, 8, TFS(&nas_5gs_mm_switch_off_tfs), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_re_reg_req,
        { "Re-registration required",   "nas_5gs.mm.re_reg_req",
            FT_BOOLEAN, 8, TFS(&nas_5gs_mm_re_reg_req_tfs), 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_acc_type,
        { "Access type",   "nas_5gs.mm.acc_type",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_mm_acc_type_vals), 0x03,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_raai_b0,
        { "Registration Area Allocation Indication (RAAI)",   "nas_5gs.mm.raai_b0",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_raai), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_conf_upd_ind_ack_b0,
        { "Acknowledgement",   "nas_5gs.mm.conf_upd_ind.ack",
            FT_BOOLEAN, 8, TFS(&tfs_requested_not_requested), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_conf_upd_ind_red_b1,
        { "Registration",   "nas_5gs.mm.conf_upd_ind.red",
            FT_BOOLEAN, 8, TFS(&tfs_requested_not_requested), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_nas_sec_algo_enc,
        { "Type of ciphering algorithm",   "nas_5gs.mm.nas_sec_algo_enc",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_mm_type_of_enc_algo_vals), 0xf0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_nas_sec_algo_ip,
        { "Type of integrity protection algorithm",   "nas_5gs.mm.nas_sec_algo_ip",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_mm_type_of_ip_algo_vals), 0x0f,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_s1_mode_b0,
        { "S1 mode",   "nas_5gs.mm.s1_mode_b0",
            FT_BOOLEAN, 8, TFS(&tfs_requested_not_requested), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_ho_attach_b1,
        { "HO attach",   "nas_5gs.mm.ho_attach_b1",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_lpp_cap_b2,
        { "LTE Positioning Protocol (LPP) capability",   "nas_5gs.mm.lpp_cap_b2",
            FT_BOOLEAN, 8, TFS(&tfs_requested_not_requested), 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_type_id,
        { "Type of identity",   "nas_5gs.mm.type_id",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_mm_type_id_vals), 0x07,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_odd_even,
        { "Odd/even indication","nas_5gs.mm.odd_even",
            FT_BOOLEAN, 8, TFS(&nas_5gs_odd_even_tfs), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_length,
        { "Length",   "nas_5gs.mm.length",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_abba,
        { "ABBA Contents",   "nas_5gs.mm.abba_contents",
            FT_UINT16, BASE_HEX, NULL, 0x00,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_pld_cont,
        { "Payload container",   "nas_5gs.mm.pld_cont",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_req_type,
        { "Request type",   "nas_5gs.mm.req_typ",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_mm_req_type_vals), 0x0f,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_serv_type,
        { "Service type",   "nas_5gs.mm.serv_type",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_mm_serv_type_vals), 0x70,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_5g_ea0,
        { "5G-EA0","nas_5gs.mm.5g_ea0",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x80,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_128_5g_ea1,
        { "128-5G-EA1","nas_5gs.mm.128_5g_ea1",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x40,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_128_5g_ea2,
        { "128-5G-EA2","nas_5gs.mm.128_5g_ea2",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x20,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_128_5g_ea3,
        { "128-5G-EA3","nas_5gs.mm.128_5g_ea3",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_5g_ea4,
        { "5G-EA4","nas_5gs.mm.5g_ea4",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_5g_ea5,
        { "5G-EA5","nas_5gs.mm.5g_ea5",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_5g_ea6,
        { "5G-EA6","nas_5gs.mm.5g_ea6",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_5g_ea7,
        { "5G-EA7","nas_5gs.mm.5g_ea7",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_5g_ia0,
        { "5G-IA0","nas_5gs.mm.ia0",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x80,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_5g_128_ia1,
        { "128-5G-IA1","nas_5gs.mm.5g_128_ia1",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x40,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_5g_128_ia2,
        { "128-5G-IA2","nas_5gs.mm.5g_128_ia2",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x20,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_5g_128_ia3,
        { "128-5G-IA3","nas_5gs.mm.5g_128_ia4",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_5g_ia4,
        { "5G-IA4","nas_5gs.mm.5g_128_ia4",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_5g_ia5,
        { "5G-IA5","nas_5gs.mm.5g_ia5",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_5g_ia6,
        { "5G-IA6","nas_5gs.mm.5g_ia6",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_5g_ia7,
        { "5G-IA7","nas_5gs.mm.5g_ia7",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_eea0,
        { "EEA0","nas_5gs.mm.eea0",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x80,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_128eea1,
        { "128-EEA1","nas_5gs.mm.128eea1",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x40,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_128eea2,
        { "128-EEA2","nas_5gs.mm.128eea2",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x20,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_eea3,
        { "128-EEA3","nas_5gs.mm.eea3",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_eea4,
        { "EEA4","nas_5gs.mm.eea4",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_eea5,
        { "EEA5","nas_5gs.mm.eea5",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_eea6,
        { "EEA6","nas_5gs.mm.eea6",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_eea7,
        { "EEA7","nas_5gs.mm.eea7",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_eia0,
        { "EIA0","nas_5gs.mm.eia0",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x80,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_128eia1,
        { "128-EIA1","nas_5gs.mm.128eia1",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x40,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_128eia2,
        { "128-EIA2","nas_5gs.mm.128eia2",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x20,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_eia3,
        { "128-EIA3","nas_5gs.mm.eia3",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_eia4,
        { "EIA4","nas_5gs.mm.eia4",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_eia5,
        { "EIA5","nas_5gs.mm.eia5",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_eia6,
        { "EIA6","nas_5gs.mm.eia6",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_eia7,
        { "EIA7","nas_5gs.mm.eia7",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_n1_mode_reg_b1,
        { "N1 mode reg","nas_5gs.mm.n1_mode_reg_b1",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_mm_n1_mod), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_s1_mode_reg_b0,
        { "S1 mode reg","nas_5gs.mm.s1_mode_reg_b0",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_mm_s1_mod), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_sal_al_t,
        { "Allowed type","nas_5gs.mm.sal_al_t",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_sal_al_t), 0x80,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_sal_t_li,
        { "Type of list",   "nas_5gs.mm.sal_t_li",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_mm_sal_t_li_values), 0x60,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_sal_num_e,
        { "Number of elements",   "nas_5gs.mm.sal_num_e",
            FT_UINT8, BASE_DEC, NULL, 0x1f,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_pdu_session_type,
        { "PDU session type",   "nas_5gs.sm.pdu_session_type",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_pdu_session_type_values), 0x0f,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_sts_psi_0_b0,
        { "Spare","nas_5gs.pdu_ses_sts_psi_0_b0",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_sts_psi_1_b1,
        { "PSI(1)","nas_5gs.pdu_ses_sts_psi_1_b1",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_sts_psi), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_sts_psi_2_b2,
        { "PSI(2)","nas_5gs.pdu_ses_sts_psi_2_b2",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_sts_psi), 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_sts_psi_3_b3,
        { "PSI(3)","nas_5gs.pdu_ses_sts_psi_3_b3",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_sts_psi), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_sts_psi_4_b4,
        { "PSI(4)","nas_5gs.pdu_ses_sts_psi_4_b4",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_sts_psi), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_sts_psi_5_b5,
        { "PSI(5)","nas_5gs.pdu_ses_sts_psi_5_b5",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_sts_psi), 0x20,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_sts_psi_6_b6,
        { "PSI(6)","nas_5gs.pdu_ses_sts_psi_6_b6",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_sts_psi), 0x40,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_sts_psi_7_b7,
        { "PSI(7)","nas_5gs.pdu_ses_sts_psi_7_b7",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_sts_psi), 0x80,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_sts_psi_8_b0,
        { "PSI(8)","nas_5gs.pdu_ses_sts_psi_8_b0",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_sts_psi), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_sts_psi_9_b1,
        { "PSI(9)","nas_5gs.pdu_ses_sts_psi_9_b1",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_sts_psi), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_sts_psi_10_b2,
        { "PSI(10)","nas_5gs.pdu_ses_sts_psi_10_b2",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_sts_psi), 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_sts_psi_11_b3,
        { "PSI(11)","nas_5gs.pdu_ses_sts_psi_11_b3",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_sts_psi), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_sts_psi_12_b4,
        { "PSI(12)","nas_5gs.pdu_ses_sts_psi_12_b4",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_sts_psi), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_sts_psi_13_b5,
        { "PSI(13)","nas_5gs.pdu_ses_sts_psi_13_b5",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_sts_psi), 0x20,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_sts_psi_14_b6,
        { "PSI(14)","nas_5gs.pdu_ses_sts_psi_14_b6",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_sts_psi), 0x40,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_sts_psi_15_b7,
        { "PSI(15)","nas_5gs.pdu_ses_sts_psi_15_b7",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_sts_psi), 0x80,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_rect_res_psi_0_b0,
        { "PSI(0) Spare","nas_5gs.pdu_ses_rect_res_psi_0_b0",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_rect_res_psi), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_rect_res_psi_1_b1,
        { "PSI(1)","nas_5gs.pdu_ses_rect_res_psi_1_b1",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_rect_res_psi), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_rect_res_psi_2_b2,
        { "PSI(2)","nas_5gs.pdu_ses_rect_res_psi_2_b2",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_rect_res_psi), 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_rect_res_psi_3_b3,
        { "PSI(3)","nas_5gs.pdu_ses_rect_res_psi_3_b3",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_rect_res_psi), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_rect_res_psi_4_b4,
        { "PSI(4)","nas_5gs.pdu_ses_rect_res_psi_3_b4",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_rect_res_psi), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_rect_res_psi_5_b5,
        { "PSI(5)","nas_5gs.pdu_ses_rect_res_psi_3_b5",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_rect_res_psi), 0x20,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_rect_res_psi_6_b6,
        { "PSI(6)","nas_5gs.pdu_ses_rect_res_psi_3_b6",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_rect_res_psi), 0x40,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_rect_res_psi_7_b7,
        { "PSI(7)","nas_5gs.pdu_ses_rect_res_psi_3_b7",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_rect_res_psi), 0x80,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_rect_res_psi_8_b0,
        { "PSI(8)","nas_5gs.pdu_ses_rect_res_psi_8_b0",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_rect_res_psi), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_rect_res_psi_9_b1,
        { "PSI(9)","nas_5gs.pdu_ses_rect_res_psi_9_b1",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_rect_res_psi), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_rect_res_psi_10_b2,
        { "PSI(10)","nas_5gs.pdu_ses_rect_res_psi_10_b2",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_rect_res_psi), 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_rect_res_psi_11_b3,
        { "PSI(11)","nas_5gs.pdu_ses_rect_res_psi_11_b3",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_rect_res_psi), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_rect_res_psi_12_b4,
        { "PSI(12)","nas_5gs.pdu_ses_rect_res_psi_12_b4",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_rect_res_psi), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_rect_res_psi_13_b5,
        { "PSI(13)","nas_5gs.pdu_ses_sts_psi_13_b5",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_rect_res_psi), 0x20,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_rect_res_psi_14_b6,
        { "PSI(14)","nas_5gs.pdu_ses_sts_psi_14_b6",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_rect_res_psi), 0x40,
            NULL, HFILL }
        },
        { &hf_nas_5gs_pdu_ses_rect_res_psi_15_b7,
        { "PSI(15)","nas_5gs.pdu_ses_sts_psi_15_b7",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_pdu_ses_rect_res_psi), 0x80,
            NULL, HFILL }
        },
        { &hf_nas_5gs_ul_data_sts_psi_0_b0,
        { "Spare","nas_5gs.ul_data_sts_psi_0_b0",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_ul_data_sts_psi_1_b1,
        { "PSI(1)","nas_5gs.ul_data_sts_psi_1_b1",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_ul_data_sts_psi), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_ul_data_sts_psi_2_b2,
        { "PSI(2)","nas_5gs.ul_data_sts_psi_2_b2",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_ul_data_sts_psi), 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_ul_data_sts_psi_3_b3,
        { "PSI(3)","nas_5gs.ul_data_sts_psi_3_b3",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_ul_data_sts_psi), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_ul_data_sts_psi_4_b4,
        { "PSI(4)","nas_5gs.ul_data_sts_psi_4_b4",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_ul_data_sts_psi), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_ul_data_sts_psi_5_b5,
        { "PSI(5)","nas_5gs.ul_data_sts_psi_5_b5",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_ul_data_sts_psi), 0x20,
            NULL, HFILL }
        },
        { &hf_nas_5gs_ul_data_sts_psi_6_b6,
        { "PSI(6)","nas_5gs.ul_data_sts_psi_6_b6",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_ul_data_sts_psi), 0x40,
            NULL, HFILL }
        },
        { &hf_nas_5gs_ul_data_sts_psi_7_b7,
        { "PSI(7)","nas_5gs.ul_data_sts_psi_7_b7",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_ul_data_sts_psi), 0x80,
            NULL, HFILL }
        },
        { &hf_nas_5gs_ul_data_sts_psi_8_b0,
        { "PSI(8)","nas_5gs.ul_data_sts_psi_8_b0",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_ul_data_sts_psi), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_ul_data_sts_psi_9_b1,
        { "PSI(9)","nas_5gs.ul_data_sts_psi_9_b1",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_ul_data_sts_psi), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_ul_data_sts_psi_10_b2,
        { "PSI(10)","nas_5gs.ul_data_sts_psi_10_b2",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_ul_data_sts_psi), 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_ul_data_sts_psi_11_b3,
        { "PSI(11)","nas_5gs.ul_data_sts_psi_11_b3",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_ul_data_sts_psi), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_ul_data_sts_psi_12_b4,
        { "PSI(12)","nas_5gs.ul_data_sts_psi_12_b4",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_ul_data_sts_psi), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_ul_data_sts_psi_13_b5,
        { "PSI(13)","nas_5gs.ul_data_sts_psi_13_b5",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_ul_data_sts_psi), 0x20,
            NULL, HFILL }
        },
        { &hf_nas_5gs_ul_data_sts_psi_14_b6,
        { "PSI(14)","nas_5gs.ul_data_sts_psi_14_b6",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_ul_data_sts_psi), 0x40,
            NULL, HFILL }
        },
        { &hf_nas_5gs_ul_data_sts_psi_15_b7,
        { "PSI(15)","nas_5gs.ul_data_sts_psi_15_b7",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_ul_data_sts_psi), 0x80,
            NULL, HFILL }
        },
        { &hf_nas_5gs_allow_pdu_ses_sts_psi_0_b0,
        { "Spare","nas_5gs.allow_pdu_ses_sts_psi_0_b0",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_allow_pdu_ses_sts_psi_1_b1,
        { "PSI(1)","nas_5gs.allow_pdu_ses_sts_psi_1_b1",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_allow_pdu_ses_sts_psi), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_allow_pdu_ses_sts_psi_2_b2,
        { "PSI(2)","nas_5gs.allow_pdu_ses_sts_psi_2_b2",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_allow_pdu_ses_sts_psi), 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_allow_pdu_ses_sts_psi_3_b3,
        { "PSI(3)","nas_5gs.allow_pdu_ses_sts_psi_3_b3",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_allow_pdu_ses_sts_psi), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_allow_pdu_ses_sts_psi_4_b4,
        { "PSI(4)","nas_5gs.allow_pdu_ses_sts_psi_4_b4",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_allow_pdu_ses_sts_psi), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_allow_pdu_ses_sts_psi_5_b5,
        { "PSI(5)","nas_5gs.allow_pdu_ses_sts_psi_5_b5",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_allow_pdu_ses_sts_psi), 0x20,
            NULL, HFILL }
        },
        { &hf_nas_5gs_allow_pdu_ses_sts_psi_6_b6,
        { "PSI(6)","nas_5gs.allow_pdu_ses_sts_psi_6_b6",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_allow_pdu_ses_sts_psi), 0x40,
            NULL, HFILL }
        },
        { &hf_nas_5gs_allow_pdu_ses_sts_psi_7_b7,
        { "PSI(7)","nas_5gs.allow_pdu_ses_sts_psi_7_b7",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_allow_pdu_ses_sts_psi), 0x80,
            NULL, HFILL }
        },
        { &hf_nas_5gs_allow_pdu_ses_sts_psi_8_b0,
        { "PSI(8)","nas_5gs.allow_pdu_ses_sts_psi_8_b0",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_allow_pdu_ses_sts_psi), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_allow_pdu_ses_sts_psi_9_b1,
        { "PSI(9)","nas_5gs.allow_pdu_ses_sts_psi_9_b1",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_allow_pdu_ses_sts_psi), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_allow_pdu_ses_sts_psi_10_b2,
        { "PSI(10)","nas_5gs.allow_pdu_ses_sts_psi_10_b2",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_allow_pdu_ses_sts_psi), 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_allow_pdu_ses_sts_psi_11_b3,
        { "PSI(11)","nas_5gs.allow_pdu_ses_sts_psi_11_b3",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_allow_pdu_ses_sts_psi), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_allow_pdu_ses_sts_psi_12_b4,
        { "PSI(12)","nas_5gs.allow_pdu_ses_sts_psi_12_b4",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_allow_pdu_ses_sts_psi), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_allow_pdu_ses_sts_psi_13_b5,
        { "PSI(13)","nas_5gs.allow_pdu_ses_sts_psi_13_b5",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_allow_pdu_ses_sts_psi), 0x20,
            NULL, HFILL }
        },
        { &hf_nas_5gs_allow_pdu_ses_sts_psi_14_b6,
        { "PSI(14)","nas_5gs.allow_pdu_ses_sts_psi_14_b6",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_allow_pdu_ses_sts_psi), 0x40,
            NULL, HFILL }
        },
        { &hf_nas_5gs_allow_pdu_ses_sts_psi_15_b7,
        { "PSI(15)","nas_5gs.allow_pdu_ses_sts_psi_15_b7",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_allow_pdu_ses_sts_psi), 0x80,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_sc_mode,
        { "SSC mode",   "nas_5gs.sm.sc_mode",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_sc_mode_values), 0x0f,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_sel_sc_mode,
        { "Selected SSC mode",   "nas_5gs.sm.sel_sc_mode",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_sc_mode_values), 0xf0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_rqos_b0,
        { "Reflective QoS(RqoS)",   "nas_5gs.sm.rqos",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_5gsm_cause,
        { "5GSM cause",   "nas_5gs.sm.5gsm_cause",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_sm_cause_vals), 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_pdu_ses_type,
        { "PDU session type",   "nas_5gs.sm.pdu_ses_type",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_sm_pdu_ses_type_vals), 0x0f,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_pdu_addr_inf_ipv4,
        { "PDU address information", "nas_5gs.sm.pdu_addr_inf_ipv4",
            FT_IPv4, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_pdu_addr_inf_ipv6,
        { "PDU address information", "nas_5gs.sm.pdu_addr_inf_ipv6",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_qos_rule_id,
        { "QoS rule identifier",   "nas_5gs.sm.qos_rule_id",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_length,
        { "Length",   "nas_5gs.sm.length",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_rop,
        { "Rule operation code",   "nas_5gs.sm.rop",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_rule_operation_code_values), 0xe0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_dqr,
        { "DQR",   "nas_5gs.sm.dqr",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_sm_dqr), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_nof_pkt_filters,
        { "Number of packet filters",   "nas_5gs.sm.nof_pkt_filters",
            FT_UINT8, BASE_DEC, NULL, 0x0f,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_pkt_flt_dir,
        { "Packet filter direction",   "nas_5gs.sm.pkt_flt_dir",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_sm_pkt_flt_dir_values), 0x30,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_pkt_flt_id,
        { "Packet filter identifier",   "nas_5gs.sm.pkt_flt_id",
            FT_UINT8, BASE_DEC, NULL, 0x0f,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_pf_len,
        { "Length",   "nas_5gs.sm.pf_len",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_pf_type,
        { "Packet filter component type",   "nas_5gs.sm.pf_type",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_sm_pf_type_values), 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_e,
        { "E bit",   "nas_5gs.sm.e",
            FT_UINT8, BASE_DEC, NULL, 0x40,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_nof_params,
        { "Number of parameters",   "nas_5gs.sm.nof_params",
            FT_UINT8, BASE_DEC, NULL, 0x3f,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_param_id,
        { "Parameter identifier",   "nas_5gs.sm.param_id",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_sm_param_id_values), 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_param_len,
        { "Length",   "nas_5gs.sm.param_len",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_addr_mask_ipv4,
        { "IPv4 address mask", "nas_5gs.ipv4_address_mask",
            FT_IPv4, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_protocol_identifier_or_next_hd,
        { "packet filter component type", "nas_5gs.protocol_identifier_or_next_hd",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_sm_pf_type_values), 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_protocol_identifier_or_next_hd_val,
        { "packet filter component value", "nas_5gs.protocol_identifier_or_next_hd_val",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_qos_rule_precedence,
        { "QoS rule precedence",   "nas_5gs.sm.qos_rule_precedence",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_pal_cont,
        { "Parameters content",   "nas_5gs.sm.pal_cont",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_rule_param_cont), 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_qfi,
        { "Qos flow identifier",   "nas_5gs.sm.qfi",
            FT_UINT8, BASE_DEC, NULL, 0x3f,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_mapd_eps_b_cont_id,
        { "EPS bearer identity",   "nas_5gs.sm.mapd_eps_b_cont_id",
            FT_UINT8, BASE_DEC, NULL, 0xf0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_mapd_eps_b_cont_opt_code,
        { "Operation code",   "nas_5gs.sm.mapd_eps_b_cont_opt_code",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_sm_mapd_eps_b_cont_opt_code_vals), 0xc0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_qos_des_flow_opt_code,
        { "Operation code",   "nas_5gs.sm.hf_nas_5gs_sm_qos_des_flow_opt_code",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_sm_qos_des_flow_opt_code_vals), 0xe0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_mapd_eps_b_cont_DEB,
        { "DEB bit",   "nas_5gs.sm.mapd_eps_b_cont_DEB",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_sm_mapd_eps_b_cont_DEB_vals), 0x20,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_mapd_eps_b_cont_E,
        { "E bit",   "nas_5gs.sm.mapd_eps_b_cont_E",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_sm_mapd_eps_b_cont_E_vals), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_mapd_eps_b_cont_E_mod,
        { "E bit",   "nas_5gs.sm.mapd_eps_b_cont_E_mod",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_sm_mapd_eps_b_cont_E_Modify_vals), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_mapd_eps_b_cont_num_eps_parms,
        { "Number of EPS parameters",   "nas_5gs.sm.mapd_eps_b_cont_num_eps_parms",
            FT_UINT8, BASE_DEC, NULL, 0x0f,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_mapd_eps_b_cont_param_id,
        { "EPS parameter identity",   "nas_5gs.sm.mapd_eps_b_cont_param_id",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_sm_mapd_eps_b_cont_param_id_vals), 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_unit_for_session_ambr_dl,
         { "Unit for Session-AMBR for downlink",   "nas_5gs.sm.unit_for_session_ambr_dl",
             FT_UINT8, BASE_DEC, VALS(nas_5gs_sm_unit_for_session_ambr_values), 0x0,
             NULL, HFILL }
        },
        { &hf_nas_5gs_sm_unit_for_session_ambr_ul,
        { "Unit for Session-AMBR for uplink",   "nas_5gs.sm.unit_for_session_ambr_ul",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_sm_unit_for_session_ambr_values), 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_session_ambr_dl,
        { "Session-AMBR for downlink",   "nas_5gs.sm.session_ambr_dl",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_session_ambr_ul,
        { "Session-AMBR for uplink",   "nas_5gs.sm.session_ambr_ul",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_all_ssc_mode_b0,
        { "SSC mode 1",   "nas_5gs.sm.all_ssc_mode_b0",
            FT_BOOLEAN, 8, TFS(&tfs_allowed_not_allowed), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_all_ssc_mode_b1,
        { "SSC mode 2",   "nas_5gs.sm.all_ssc_mode_b1",
            FT_BOOLEAN, 8, TFS(&tfs_allowed_not_allowed), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_all_ssc_mode_b2,
        { "SSC mode 3",   "nas_5gs.sm.all_ssc_mode_b2",
            FT_BOOLEAN, 8, TFS(&tfs_allowed_not_allowed), 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_suci,
        { "SUCI",   "nas_5gs.mm.suci",
            FT_STRING, BASE_NONE, NULL, 0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_imei,
        { "IMEI", "nas_5gs.mm.imei",
            FT_STRING, BASE_NONE, NULL, 0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_imeisv,
        { "IMEISV", "nas_5gs.mm.imeisv",
            FT_STRING, BASE_NONE, NULL, 0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_reg_res_sms_allowed,
        { "SMS over NAS",   "nas_5gs.mm.reg_res.sms_all",
            FT_BOOLEAN, 8, TFS(&tfs_allowed_not_allowed), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_reg_res_res,
        { "5GS registration result",   "nas_5gs.mm.reg_res.res",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_mm_reg_res_values), 0x07,
            NULL, HFILL }
        },
        { &hf_nas_5gs_amf_region_id,
        { "AMF Region ID",   "nas_5gs.amf_region_id",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_amf_set_id,
        { "AMF Set ID",   "nas_5gs.amf_set_id",
            FT_UINT16, BASE_DEC, NULL, 0xff03,
            NULL, HFILL }
        },
        { &hf_nas_5gs_amf_pointer,
        { "AMF Pointer",   "nas_5gs.amf_pointer",
            FT_UINT8, BASE_DEC, NULL, 0xfc,
            NULL, HFILL }
        },
        { &hf_nas_5gs_5g_tmsi,
        { "5G-TMSI",   "nas_5gs.5g_tmsi",
            FT_UINT32, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_nw_feat_sup_ims_emf_b5b4,
        { "Emergency service fallback indicator (EMF)",   "nas_5gs.nw_feat_sup.emf",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_nw_feat_sup_emf_values), 0x30,
            NULL, HFILL }
        },
        { &hf_nas_5gs_nw_feat_sup_ims_emc_b3b2,
        { "Emergency service support indicator (EMC)",   "nas_5gs.nw_feat_sup.emc",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_nw_feat_sup_emc_values), 0x0c,
            NULL, HFILL }
        },
        { &hf_nas_5gs_nw_feat_sup_ims_vops_b1b0,
        { "IMS voice over PS session indicator (IMS VoPS)",   "nas_5gs.nw_feat_sup.vops",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_nw_feat_sup_ims_vops_values), 0x03,
            NULL, HFILL }
        },
        { &hf_nas_5gs_nw_feat_sup_ims_iwk_n26_b6,
        { "Interworking without N26",   "nas_5gs.nw_feat_sup.iwk_n26",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x40,
            NULL, HFILL }
        },
        { &hf_nas_5gs_nw_feat_sup_mpsi_b7,
        { "MPS indicator (MPSI)",   "nas_5gs.nw_feat_sup.mpsi",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_nw_feat_sup_mpsi), 0x80,
            NULL, HFILL }
        },
        { &hf_nas_5gs_tac,
        { "TAC",   "nas_5gs.tac",
            FT_UINT24, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_tal_t_li,
        { "Type of list",   "nas_5gs.mm.tal_t_li",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_mm_tal_t_li_values), 0x60,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_tal_num_e,
        { "Number of elements",   "nas_5gs.mm.tal_num_e",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_mm_tal_num_e), 0x1f,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sm_mapd_eps_b_cont_eps_param_cont,
        { "EPS parameter contents",   "nas_5gs.sm.mapd_eps_b_cont_eps_param_cont",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_kacf,
        { "K_AMF change flag", "nas_5gs.kacf",
            FT_BOOLEAN, 8, TFS(&nas_5gs_kacf_tfs), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_ncc,
        { "NCC", "nas_5gs.ncc",
            FT_UINT8, BASE_DEC, NULL, 0x70,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_rinmr,
        { "Retransmission of initial NAS message request(RINMR)", "nas_5gs.mm.rinmr",
            FT_BOOLEAN, 8, TFS(&tfs_requested_not_requested), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_hdp,
        { "Horizontal derivation parameter (HDP)", "nas_5gs.mm.hdp",
            FT_BOOLEAN, 8, TFS(&tfs_required_not_required), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_dcni,
        { "Default configured NSSAI indication (DCNI)", "nas_5gs.mm.dcni",
            FT_BOOLEAN, 8, TFS(&nas_5gs_mm_dcni_tfs), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_nssci,
        { "Network slicing subscription change indication (NSSCI)", "nas_5gs.mm.nssci",
            FT_BOOLEAN, 8, TFS(&tfs_changed_not_changed), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_nssai_inc_mode,
        { "NSSAI inclusion mode", "nas_5gs.mm.nssai_inc_mode",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_mm_nssai_inc_mode_vals), 0x03,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_ue_usage_setting,
        { "UE's usage setting", "nas_5gs.mm.ue_usage_setting",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_mm_ue_usage_setting), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_ng_ran_rcu,
        { "NG-RAN Radio Capability Update (NG-RAN-RCU)", "nas_5gs.mm.ng_ran_rcu",
            FT_BOOLEAN, 8, TFS(&tfs_needed_not_needed), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_5gs_drx_param,
        { "DRX value", "nas_5gs.mm.drx_value",
            FT_UINT8, BASE_DEC, VALS(nas_5gs_mm_drx_vals), 0x0f,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_precedence,
        { "Precedence", "nas_5gs.mm.precedence",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_mm_sms_indic_sai,
        { "SMS over NAS",   "nas_5gs.mm.ms_indic.sai",
            FT_BOOLEAN, 8, TFS(&tfs_allowed_not_allowed), 0x01,
            "SMS availability indication (SAI)", HFILL }
        },
        { &hf_nas_5gs_sor_hdr0_ack,
        { "Acknowledgement (ACK)",   "nas_5gs.sor_hdr0.ack",
            FT_BOOLEAN, 8, TFS(&tfs_requested_not_requested), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sor_hdr0_list_type,
        { "List type",   "nas_5gs.sor_hdr0.list_type",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_list_type), 0x04,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sor_hdr0_list_ind,
        { "List indication",   "nas_5gs.sor_hdr0.list_ind",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_list_ind), 0x02,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sor_hdr0_sor_data_type,
        { "SOR data type",   "nas_5gs.sor.sor_data_type",
            FT_BOOLEAN, 8, TFS(&tfs_nas_5gs_sor_data_type), 0x01,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sor_mac_iue,
        { "SOR-MAC-IUE", "nas_5gs.mm.sor_mac_iue",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sor_mac_iausf,
        { "SOR-MAC-IAUSF", "nas_5gs.mm.sor_mac_iausf",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_counter_sor,
        { "CounterSOR", "nas_5gs.mm.counter_sor",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_sor_sec_pkt,
        { "Secured packet", "nas_5gs.mm.sor_sec_pkt",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_nas_5gs_acces_tech_o1_b7,
        { "Access technology UTRAN",   "nas_5gs.cces_tech_o1_b7.utran",
            FT_BOOLEAN, 8, TFS(&tfs_selected_not_selected), 0x80,
            NULL, HFILL }
        },
        { &hf_nas_5gs_acces_tech_o1_b6,
        { "Access technology E-UTRAN",   "nas_5gs.cces_tech_o1_b6.e_utran",
            FT_BOOLEAN, 8, TFS(&tfs_selected_not_selected), 0x40,
            NULL, HFILL }
        },
        { &hf_nas_5gs_acces_tech_o1_b5,
        { "Access technology E-UTRAN in WB-S1 mode",   "nas_5gs.cces_tech_o1_b5.e_utran_in_wb_s1_mode",
            FT_BOOLEAN, 8, TFS(&tfs_selected_not_selected), 0x20,
            NULL, HFILL }
        },
        { &hf_nas_5gs_acces_tech_o1_b4,
        { "Access technology E-UTRAN in NB-S1 mode",   "nas_5gs.cces_tech_o1_b4.e_utran_in_nb_s1_mode",
            FT_BOOLEAN, 8, TFS(&tfs_selected_not_selected), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_acces_tech_o1_b3,
        { "Access technology NG-RAN",   "nas_5gs.cces_tech_o1_b3.ng_ran",
            FT_BOOLEAN, 8, TFS(&tfs_selected_not_selected), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_acces_tech_o2_b7,
        { "Access technology GSM",   "nas_5gs.cces_tech_o2_b7.gsm",
            FT_BOOLEAN, 8, TFS(&tfs_selected_not_selected), 0x80,
            NULL, HFILL }
        },
        { &hf_nas_5gs_acces_tech_o2_b6,
        { "Access technology GSM COMPACT",   "nas_5gs.cces_tech_o2_b6.gsm_compact",
            FT_BOOLEAN, 8, TFS(&tfs_selected_not_selected), 0x40,
            NULL, HFILL }
        },
        { &hf_nas_5gs_acces_tech_o2_b5,
        { "Access technology CDMA2000 HRPD",   "nas_5gs.cces_tech_o2_b5.cdma2000_hrpd",
            FT_BOOLEAN, 8, TFS(&tfs_selected_not_selected), 0x20,
            NULL, HFILL }
        },
        { &hf_nas_5gs_acces_tech_o2_b4,
        { "Access technology CDMA2000 1xRTT",   "nas_5gs.cces_tech_o2_b4.cdma2000_1x_rtt",
            FT_BOOLEAN, 8, TFS(&tfs_selected_not_selected), 0x10,
            NULL, HFILL }
        },
        { &hf_nas_5gs_acces_tech_o2_b3,
        { "Access technology EC-GSM-IoT",   "nas_5gs.cces_tech_o2_b3.ec_gsm_iot",
            FT_BOOLEAN, 8, TFS(&tfs_selected_not_selected), 0x08,
            NULL, HFILL }
        },
        { &hf_nas_5gs_acces_tech_o2_b2,
        { "Access technology GSM",   "nas_5gs.cces_tech_o2_b2.gsm",
            FT_BOOLEAN, 8, TFS(&tfs_selected_not_selected), 0x04,
            NULL, HFILL }
        },
    };

    guint     i;
    guint     last_offset;

    /* Setup protocol subtree array */
#define NUM_INDIVIDUAL_ELEMS    14
    gint *ett[NUM_INDIVIDUAL_ELEMS +
        NUM_NAS_5GS_COMMON_ELEM +
        NUM_NAS_5GS_MM_MSG + NUM_NAS_5GS_MM_ELEM +
        NUM_NAS_5GS_SM_MSG + NUM_NAS_5GS_SM_ELEM
    ];

    ett[0] = &ett_nas_5gs;
    ett[1] = &ett_nas_5gs_mm_nssai;
    ett[2] = &ett_nas_5gs_mm_pdu_ses_id;
    ett[3] = &ett_nas_5gs_sm_qos_rules;
    ett[4] = &ett_nas_5gs_sm_qos_params;
    ett[5] = &ett_nas_5gs_plain;
    ett[6] = &ett_nas_5gs_sec;
    ett[7] = &ett_nas_5gs_mm_part_sal;
    ett[8] = &ett_nas_5gs_mm_part_tal;
    ett[9] = &ett_nas_5gs_sm_mapd_eps_b_cont;
    ett[10] = &ett_nas_5gs_sm_mapd_eps_b_cont_params_list;
    ett[11] = &ett_nas_5gs_enc;
    ett[12] = &ett_nas_5gs_mm_ladn_indic;
    ett[13] = &ett_nas_5gs_mm_sor;

    last_offset = NUM_INDIVIDUAL_ELEMS;

    for (i = 0; i < NUM_NAS_5GS_COMMON_ELEM; i++, last_offset++)
    {
        ett_nas_5gs_common_elem[i] = -1;
        ett[last_offset] = &ett_nas_5gs_common_elem[i];
    }

    /* MM */
    for (i = 0; i < NUM_NAS_5GS_MM_MSG; i++, last_offset++)
    {
        ett_nas_5gs_mm_msg[i] = -1;
        ett[last_offset] = &ett_nas_5gs_mm_msg[i];
    }

    for (i = 0; i < NUM_NAS_5GS_MM_ELEM; i++, last_offset++)
    {
        ett_nas_5gs_mm_elem[i] = -1;
        ett[last_offset] = &ett_nas_5gs_mm_elem[i];
    }

    for (i = 0; i < NUM_NAS_5GS_SM_MSG; i++, last_offset++)
    {
        ett_nas_5gs_sm_msg[i] = -1;
        ett[last_offset] = &ett_nas_5gs_sm_msg[i];
    }

    for (i = 0; i < NUM_NAS_5GS_SM_ELEM; i++, last_offset++)
    {
        ett_nas_5gs_sm_elem[i] = -1;
        ett[last_offset] = &ett_nas_5gs_sm_elem[i];
    }

    static ei_register_info ei[] = {
    { &ei_nas_5gs_extraneous_data, { "nas_5gs.extraneous_data", PI_PROTOCOL, PI_NOTE, "Extraneous Data, dissector bug or later version spec(report to wireshark.org)", EXPFILL }},
    { &ei_nas_5gs_unknown_pd,{ "nas_5gs.unknown_pd", PI_PROTOCOL, PI_ERROR, "Unknown protocol discriminator", EXPFILL } },
    { &ei_nas_5gs_mm_unknown_msg_type,{ "nas_5gs.mm.unknown_msg_type", PI_PROTOCOL, PI_WARN, "Unknown Message Type", EXPFILL } },
    { &ei_nas_5gs_sm_unknown_msg_type,{ "nas_5gs.sm.unknown_msg_type", PI_PROTOCOL, PI_WARN, "Unknown Message Type", EXPFILL } },
    { &ei_nas_5gs_msg_not_dis,{ "nas_5gs.msg_not_dis", PI_PROTOCOL, PI_WARN, "MSG IEs not dissected yet", EXPFILL } },
    { &ei_nas_5gs_ie_not_dis,{ "nas_5gs.ie_not_dis", PI_PROTOCOL, PI_WARN, "IE not dissected yet", EXPFILL } },
    { &ei_nas_5gs_missing_mandatory_elemen,{ "nas_5gs.missing_mandatory_element", PI_PROTOCOL, PI_ERROR, "Missing Mandatory element, rest of dissection is suspect", EXPFILL } },
    { &ei_nas_5gs_dnn_too_long,{ "nas_5gs.dnn_to_long", PI_PROTOCOL, PI_ERROR, "DNN encoding has more than 100 octets", EXPFILL } },
    { &ei_nas_5gs_unknown_value,{ "nas_5gs.unknown_value", PI_PROTOCOL, PI_ERROR, "Value not according to (decoded)specification", EXPFILL } },
    { &ei_nas_5gs_num_pkt_flt,{ "nas_5gs.num_pkt_flt", PI_PROTOCOL, PI_ERROR, "num_pkt_flt != 0", EXPFILL } },
    { &ei_nas_5gs_not_diss,{ "nas_5gs.not_diss", PI_PROTOCOL, PI_NOTE, "Not dissected yet", EXPFILL } },
    };

    expert_module_t* expert_nas_5gs;
    module_t *nas_5GS_module;

    /* Register protocol */
    proto_nas_5gs = proto_register_protocol(PNAME, PSNAME, PFNAME);
    /* Register fields and subtrees */
    proto_register_field_array(proto_nas_5gs, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    expert_nas_5gs = expert_register_protocol(proto_nas_5gs);
    expert_register_field_array(expert_nas_5gs, ei, array_length(ei));

    /* Register dissector */
    nas_5gs_handle = register_dissector(PFNAME, dissect_nas_5gs, proto_nas_5gs);

    nas_5GS_module = prefs_register_protocol(proto_nas_5gs, NULL);

    prefs_register_bool_preference(nas_5GS_module,
        "null_decipher",
        "Try to detect and decode EEA0 ciphered messages",
        "This should work when the NAS ciphering algorithm is NULL (128-EEA0)",
        &g_nas_5gs_null_decipher);


}

void
proto_reg_handoff_nas_5gs(void)
{
    eap_handle = find_dissector("eap");
    nas_eps_handle = find_dissector("nas-eps");
    nas_eps_plain_handle = find_dissector("nas-eps_plain");
    dissector_add_string("media_type", "application/vnd.3gpp.5gnas", nas_5gs_handle);
}

/*
* Editor modelines
*
* Local Variables:
* c-basic-offset: 4
* tab-width: 8
* indent-tabs-mode: nil
* End:
*
* ex: set shiftwidth=4 tabstop=8 expandtab:
* :indentSize=4:tabSize=8:noTabs=true:
*/
