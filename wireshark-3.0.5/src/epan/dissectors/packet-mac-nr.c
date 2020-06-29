/* Routines for 5G/NR MAC disassembly
 *
 * Based on packet-mac-lte.c
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <stdio.h>
#include <epan/packet.h>
#include <epan/exceptions.h>
#include <epan/expert.h>
#include <epan/proto_data.h>
#include <epan/tfs.h>
#include <epan/uat.h>

#include "packet-mac-nr.h"
#include "packet-rlc-nr.h"

void proto_register_mac_nr(void);
void proto_reg_handoff_mac_nr(void);

/* Described in:
 * 3GPP TS 38.321 NR; Medium Access Control (MAC) protocol specification v15.3.0
 */

/* Initialize the protocol and registered fields. */
int proto_mac_nr = -1;

static dissector_handle_t rlc_nr_handle;

/* Decoding context */
static int hf_mac_nr_context = -1;
static int hf_mac_nr_context_radio_type = -1;
static int hf_mac_nr_context_direction = -1;
static int hf_mac_nr_context_rnti = -1;
static int hf_mac_nr_context_rnti_type = -1;
static int hf_mac_nr_context_ueid = -1;
static int hf_mac_nr_context_harqid = -1;
static int hf_mac_nr_context_bcch_transport_channel = -1;
static int hf_mac_nr_context_phr_type2_othercell = -1;


static int hf_mac_nr_subheader = -1;
static int hf_mac_nr_subheader_reserved = -1;
static int hf_mac_nr_subheader_f = -1;
static int hf_mac_nr_subheader_length_1_byte = -1;
static int hf_mac_nr_subheader_length_2_bytes = -1;
static int hf_mac_nr_ulsch_lcid = -1;
static int hf_mac_nr_dlsch_lcid = -1;
static int hf_mac_nr_dlsch_sdu = -1;
static int hf_mac_nr_ulsch_sdu = -1;
static int hf_mac_nr_bcch_pdu = -1;
static int hf_mac_nr_pcch_pdu = -1;

static int hf_mac_nr_control_crnti = -1;
static int hf_mac_nr_control_ue_contention_resolution_identity = -1;
static int hf_mac_nr_control_timing_advance_tagid = -1;
static int hf_mac_nr_control_timing_advance_command = -1;
static int hf_mac_nr_control_se_phr_reserved = -1;
static int hf_mac_nr_control_se_phr_ph = -1;
static int hf_mac_nr_control_se_phr_pcmax_f_c = -1;
static int hf_mac_nr_control_recommended_bit_rate_query_lcid = -1;
static int hf_mac_nr_control_recommended_bit_rate_query_dir = -1;
static int hf_mac_nr_control_recommended_bit_rate_query_bit_rate = -1;
static int hf_mac_nr_control_recommended_bit_rate_query_reserved = -1;
static int hf_mac_nr_control_me_phr_c7_flag = -1;
static int hf_mac_nr_control_me_phr_c6_flag = -1;
static int hf_mac_nr_control_me_phr_c5_flag = -1;
static int hf_mac_nr_control_me_phr_c4_flag = -1;
static int hf_mac_nr_control_me_phr_c3_flag = -1;
static int hf_mac_nr_control_me_phr_c2_flag = -1;
static int hf_mac_nr_control_me_phr_c1_flag = -1;
static int hf_mac_nr_control_me_phr_c15_flag = -1;
static int hf_mac_nr_control_me_phr_c14_flag = -1;
static int hf_mac_nr_control_me_phr_c13_flag = -1;
static int hf_mac_nr_control_me_phr_c12_flag = -1;
static int hf_mac_nr_control_me_phr_c11_flag = -1;
static int hf_mac_nr_control_me_phr_c10_flag = -1;
static int hf_mac_nr_control_me_phr_c9_flag = -1;
static int hf_mac_nr_control_me_phr_c8_flag = -1;
static int hf_mac_nr_control_me_phr_c23_flag = -1;
static int hf_mac_nr_control_me_phr_c22_flag = -1;
static int hf_mac_nr_control_me_phr_c21_flag = -1;
static int hf_mac_nr_control_me_phr_c20_flag = -1;
static int hf_mac_nr_control_me_phr_c19_flag = -1;
static int hf_mac_nr_control_me_phr_c18_flag = -1;
static int hf_mac_nr_control_me_phr_c17_flag = -1;
static int hf_mac_nr_control_me_phr_c16_flag = -1;
static int hf_mac_nr_control_me_phr_c31_flag = -1;
static int hf_mac_nr_control_me_phr_c30_flag = -1;
static int hf_mac_nr_control_me_phr_c29_flag = -1;
static int hf_mac_nr_control_me_phr_c28_flag = -1;
static int hf_mac_nr_control_me_phr_c27_flag = -1;
static int hf_mac_nr_control_me_phr_c26_flag = -1;
static int hf_mac_nr_control_me_phr_c25_flag = -1;
static int hf_mac_nr_control_me_phr_c24_flag = -1;
static int hf_mac_nr_control_me_phr_entry = -1;
static int hf_mac_nr_control_me_phr_p = -1;
static int hf_mac_nr_control_me_phr_v = -1;
static int hf_mac_nr_control_me_phr_reserved_2 = -1;
static int hf_mac_nr_control_me_phr_ph_type2_spcell = -1;
static int hf_mac_nr_control_me_phr_ph_type1_pcell = -1;
static int hf_mac_nr_control_me_phr_ph_c31 = -1;
static int hf_mac_nr_control_me_phr_ph_c30 = -1;
static int hf_mac_nr_control_me_phr_ph_c29 = -1;
static int hf_mac_nr_control_me_phr_ph_c28 = -1;
static int hf_mac_nr_control_me_phr_ph_c27 = -1;
static int hf_mac_nr_control_me_phr_ph_c26 = -1;
static int hf_mac_nr_control_me_phr_ph_c25 = -1;
static int hf_mac_nr_control_me_phr_ph_c24 = -1;
static int hf_mac_nr_control_me_phr_ph_c23 = -1;
static int hf_mac_nr_control_me_phr_ph_c22 = -1;
static int hf_mac_nr_control_me_phr_ph_c21 = -1;
static int hf_mac_nr_control_me_phr_ph_c20 = -1;
static int hf_mac_nr_control_me_phr_ph_c19 = -1;
static int hf_mac_nr_control_me_phr_ph_c18 = -1;
static int hf_mac_nr_control_me_phr_ph_c17 = -1;
static int hf_mac_nr_control_me_phr_ph_c16 = -1;
static int hf_mac_nr_control_me_phr_ph_c15 = -1;
static int hf_mac_nr_control_me_phr_ph_c14 = -1;
static int hf_mac_nr_control_me_phr_ph_c13 = -1;
static int hf_mac_nr_control_me_phr_ph_c12 = -1;
static int hf_mac_nr_control_me_phr_ph_c11 = -1;
static int hf_mac_nr_control_me_phr_ph_c10 = -1;
static int hf_mac_nr_control_me_phr_ph_c9 = -1;
static int hf_mac_nr_control_me_phr_ph_c8 = -1;
static int hf_mac_nr_control_me_phr_ph_c7 = -1;
static int hf_mac_nr_control_me_phr_ph_c6 = -1;
static int hf_mac_nr_control_me_phr_ph_c5 = -1;
static int hf_mac_nr_control_me_phr_ph_c4 = -1;
static int hf_mac_nr_control_me_phr_ph_c3 = -1;
static int hf_mac_nr_control_me_phr_ph_c2 = -1;
static int hf_mac_nr_control_me_phr_ph_c1 = -1;
static int hf_mac_nr_control_me_phr_reserved = -1;
static int hf_mac_nr_control_me_phr_pcmax_f_c_type2_spcell = -1;
static int hf_mac_nr_control_me_phr_pcmax_f_c_type1_pcell = -1;
/* TODO: is it worth having separate fields for each SCellIndex for this field too? */
static int hf_mac_nr_control_me_phr_pcmax_f_c_typeX = -1;
static int hf_mac_nr_control_recommended_bit_rate_lcid = -1;
static int hf_mac_nr_control_recommended_bit_rate_dir = -1;
static int hf_mac_nr_control_recommended_bit_rate_bit_rate = -1;
static int hf_mac_nr_control_recommended_bit_rate_reserved = -1;
static int hf_mac_control_sp_zp_csi_rs_resource_set_act_deact_ad = -1;
static int hf_mac_control_sp_zp_csi_rs_resource_set_act_deact_serving_cell_id = -1;
static int hf_mac_control_sp_zp_csi_rs_resource_set_act_deact_bwp_id = -1;
static int hf_mac_control_sp_zp_csi_rs_resource_set_act_deact_reserved_2 = -1;
static int hf_mac_control_sp_zp_csi_rs_resource_set_act_deact_sp_zp_rs_resource_set_id = -1;
static int hf_mac_nr_control_pucch_spatial_rel_act_deact_reserved = -1;
static int hf_mac_nr_control_pucch_spatial_rel_act_deact_serving_cell_id = -1;
static int hf_mac_nr_control_pucch_spatial_rel_act_deact_bwp_id = -1;
static int hf_mac_nr_control_pucch_spatial_rel_act_deact_pucch_resource_id = -1;
static int hf_mac_nr_control_pucch_spatial_rel_act_deact_s7 = -1;
static int hf_mac_nr_control_pucch_spatial_rel_act_deact_s6 = -1;
static int hf_mac_nr_control_pucch_spatial_rel_act_deact_s5 = -1;
static int hf_mac_nr_control_pucch_spatial_rel_act_deact_s4 = -1;
static int hf_mac_nr_control_pucch_spatial_rel_act_deact_s3 = -1;
static int hf_mac_nr_control_pucch_spatial_rel_act_deact_s2 = -1;
static int hf_mac_nr_control_pucch_spatial_rel_act_deact_s1 = -1;
static int hf_mac_nr_control_pucch_spatial_rel_act_deact_s0 = -1;
static int hf_mac_nr_control_sp_srs_act_deact_ad = -1;
static int hf_mac_nr_control_sp_srs_act_deact_srs_resource_set_cell_id = -1;
static int hf_mac_nr_control_sp_srs_act_deact_srs_resource_set_bwp_id = -1;
static int hf_mac_nr_control_sp_srs_act_deact_reserved = -1;
static int hf_mac_nr_control_sp_srs_act_deact_c = -1;
static int hf_mac_nr_control_sp_srs_act_deact_sul = -1;
static int hf_mac_nr_control_sp_srs_act_deact_sp_srs_resource_set_id = -1;
static int hf_mac_nr_control_sp_srs_act_deact_f = -1;
static int hf_mac_nr_control_sp_srs_act_deact_resource_id = -1;
static int hf_mac_nr_control_sp_srs_act_deact_resource_serving_cell_id = -1;
static int hf_mac_nr_control_sp_srs_act_deact_resource_bwp_id = -1;
static int hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_reserved = -1;
static int hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_serving_cell_id = -1;
static int hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_bwp_id = -1;
static int hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s7 = -1;
static int hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s6 = -1;
static int hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s5 = -1;
static int hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s4 = -1;
static int hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s3 = -1;
static int hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s2 = -1;
static int hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s1 = -1;
static int hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s0 = -1;
static int hf_mac_nr_control_tci_state_ind_for_ue_spec_pdcch_serving_cell_id = -1;
static int hf_mac_nr_control_tci_state_ind_for_ue_spec_pdcch_coreset_id = -1;
static int hf_mac_nr_control_tci_state_ind_for_ue_spec_pdcch_tci_state_id = -1;
static int hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_reserved = -1;
static int hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_serving_cell_id = -1;
static int hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_bwp_id = -1;
static int hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t7 = -1;
static int hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t6 = -1;
static int hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t5 = -1;
static int hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t4 = -1;
static int hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t3 = -1;
static int hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t2 = -1;
static int hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t1 = -1;
static int hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t0 = -1;
static int hf_mac_nr_control_aper_csi_trigger_state_subselect_reserved = -1;
static int hf_mac_nr_control_aper_csi_trigger_state_subselect_serving_cell_id = -1;
static int hf_mac_nr_control_aper_csi_trigger_state_subselect_bwp_id = -1;
static int hf_mac_nr_control_aper_csi_trigger_state_subselect_t7 = -1;
static int hf_mac_nr_control_aper_csi_trigger_state_subselect_t6 = -1;
static int hf_mac_nr_control_aper_csi_trigger_state_subselect_t5 = -1;
static int hf_mac_nr_control_aper_csi_trigger_state_subselect_t4 = -1;
static int hf_mac_nr_control_aper_csi_trigger_state_subselect_t3 = -1;
static int hf_mac_nr_control_aper_csi_trigger_state_subselect_t2 = -1;
static int hf_mac_nr_control_aper_csi_trigger_state_subselect_t1 = -1;
static int hf_mac_nr_control_aper_csi_trigger_state_subselect_t0 = -1;
static int hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_ad = -1;
static int hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_serving_cell_id = -1;
static int hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_bwp_id = -1;
static int hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_reserved = -1;
static int hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_im = -1;
static int hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_sp_csi_rs_res_set_id = -1;
static int hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_reserved2 = -1;
static int hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_sp_csi_im_res_set_id = -1;
static int hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_reserved3 = -1;
static int hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_tci_state_id = -1;
static int hf_mac_nr_control_dupl_act_deact_drb7 = -1;
static int hf_mac_nr_control_dupl_act_deact_drb6 = -1;
static int hf_mac_nr_control_dupl_act_deact_drb5 = -1;
static int hf_mac_nr_control_dupl_act_deact_drb4 = -1;
static int hf_mac_nr_control_dupl_act_deact_drb3 = -1;
static int hf_mac_nr_control_dupl_act_deact_drb2 = -1;
static int hf_mac_nr_control_dupl_act_deact_drb1 = -1;
static int hf_mac_nr_control_dupl_act_deact_reserved = -1;
static int hf_mac_nr_control_scell_act_deact_cell7 = -1;
static int hf_mac_nr_control_scell_act_deact_cell6 = -1;
static int hf_mac_nr_control_scell_act_deact_cell5 = -1;
static int hf_mac_nr_control_scell_act_deact_cell4 = -1;
static int hf_mac_nr_control_scell_act_deact_cell3 = -1;
static int hf_mac_nr_control_scell_act_deact_cell2 = -1;
static int hf_mac_nr_control_scell_act_deact_cell1 = -1;
static int hf_mac_nr_control_scell_act_deact_reserved = -1;
static int hf_mac_nr_control_scell_act_deact_cell15 = -1;
static int hf_mac_nr_control_scell_act_deact_cell14 = -1;
static int hf_mac_nr_control_scell_act_deact_cell13 = -1;
static int hf_mac_nr_control_scell_act_deact_cell12 = -1;
static int hf_mac_nr_control_scell_act_deact_cell11 = -1;
static int hf_mac_nr_control_scell_act_deact_cell10 = -1;
static int hf_mac_nr_control_scell_act_deact_cell9 = -1;
static int hf_mac_nr_control_scell_act_deact_cell8 = -1;
static int hf_mac_nr_control_scell_act_deact_cell23 = -1;
static int hf_mac_nr_control_scell_act_deact_cell22 = -1;
static int hf_mac_nr_control_scell_act_deact_cell21 = -1;
static int hf_mac_nr_control_scell_act_deact_cell20 = -1;
static int hf_mac_nr_control_scell_act_deact_cell19 = -1;
static int hf_mac_nr_control_scell_act_deact_cell18 = -1;
static int hf_mac_nr_control_scell_act_deact_cell17 = -1;
static int hf_mac_nr_control_scell_act_deact_cell16 = -1;
static int hf_mac_nr_control_scell_act_deact_cell31 = -1;
static int hf_mac_nr_control_scell_act_deact_cell30 = -1;
static int hf_mac_nr_control_scell_act_deact_cell29 = -1;
static int hf_mac_nr_control_scell_act_deact_cell28 = -1;
static int hf_mac_nr_control_scell_act_deact_cell27 = -1;
static int hf_mac_nr_control_scell_act_deact_cell26 = -1;
static int hf_mac_nr_control_scell_act_deact_cell25 = -1;
static int hf_mac_nr_control_scell_act_deact_cell24 = -1;
static int hf_mac_nr_control_bsr_short_lcg = -1;
static int hf_mac_nr_control_bsr_short_bs_lcg0 = -1;
static int hf_mac_nr_control_bsr_short_bs_lcg1 = -1;
static int hf_mac_nr_control_bsr_short_bs_lcg2 = -1;
static int hf_mac_nr_control_bsr_short_bs_lcg3 = -1;
static int hf_mac_nr_control_bsr_short_bs_lcg4 = -1;
static int hf_mac_nr_control_bsr_short_bs_lcg5 = -1;
static int hf_mac_nr_control_bsr_short_bs_lcg6 = -1;
static int hf_mac_nr_control_bsr_short_bs_lcg7 = -1;
static int hf_mac_nr_control_bsr_long_lcg7 = -1;
static int hf_mac_nr_control_bsr_long_lcg6 = -1;
static int hf_mac_nr_control_bsr_long_lcg5 = -1;
static int hf_mac_nr_control_bsr_long_lcg4 = -1;
static int hf_mac_nr_control_bsr_long_lcg3 = -1;
static int hf_mac_nr_control_bsr_long_lcg2 = -1;
static int hf_mac_nr_control_bsr_long_lcg1 = -1;
static int hf_mac_nr_control_bsr_long_lcg0 = -1;
static int hf_mac_nr_control_bsr_trunc_long_bs = -1;
static int hf_mac_nr_control_bsr_long_bs_lcg0 = -1;
static int hf_mac_nr_control_bsr_long_bs_lcg1 = -1;
static int hf_mac_nr_control_bsr_long_bs_lcg2 = -1;
static int hf_mac_nr_control_bsr_long_bs_lcg3 = -1;
static int hf_mac_nr_control_bsr_long_bs_lcg4 = -1;
static int hf_mac_nr_control_bsr_long_bs_lcg5 = -1;
static int hf_mac_nr_control_bsr_long_bs_lcg6 = -1;
static int hf_mac_nr_control_bsr_long_bs_lcg7 = -1;

static int hf_mac_nr_rar = -1;
static int hf_mac_nr_rar_subheader = -1;
static int hf_mac_nr_rar_e = -1;
static int hf_mac_nr_rar_t = -1;
static int hf_mac_nr_rar_reserved = -1;
static int hf_mac_nr_rar_reserved1 = -1;

static int hf_mac_nr_rar_bi = -1;
static int hf_mac_nr_rar_rapid = -1;
static int hf_mac_nr_rar_ta = -1;
static int hf_mac_nr_rar_grant = -1;
static int hf_mac_nr_rar_grant_hopping = -1;
static int hf_mac_nr_rar_grant_fra = -1;
static int hf_mac_nr_rar_grant_tsa = -1;
static int hf_mac_nr_rar_grant_mcs = -1;
static int hf_mac_nr_rar_grant_tcsp = -1;
static int hf_mac_nr_rar_grant_csi = -1;

static int hf_mac_nr_rar_temp_crnti = -1;

static int hf_mac_nr_padding = -1;

/* Subtrees. */
static int ett_mac_nr = -1;
static int ett_mac_nr_context = -1;
static int ett_mac_nr_subheader = -1;
static int ett_mac_nr_rar_subheader = -1;
static int ett_mac_nr_rar_grant = -1;
static int ett_mac_nr_me_phr_entry = -1;

static expert_field ei_mac_nr_no_per_frame_data = EI_INIT;
static expert_field ei_mac_nr_sdu_length_different_from_dissected = EI_INIT;
static expert_field ei_mac_nr_unknown_udp_framing_tag = EI_INIT;
static expert_field ei_mac_nr_dl_sch_control_subheader_after_data_subheader = EI_INIT;
static expert_field ei_mac_nr_ul_sch_control_subheader_before_data_subheader = EI_INIT;


static dissector_handle_t nr_rrc_bcch_bch_handle;
static dissector_handle_t nr_rrc_bcch_dl_sch_handle;
static dissector_handle_t nr_rrc_pcch_handle;
static dissector_handle_t nr_rrc_dl_ccch_handle;
static dissector_handle_t nr_rrc_ul_ccch_handle;
static dissector_handle_t nr_rrc_ul_ccch1_handle;


/**************************************************************************/
/* Preferences state                                                      */
/**************************************************************************/

/* By default try to decode transparent data (BCCH, PCCH and CCCH) data using NR RRC dissector */
static gboolean global_mac_nr_attempt_rrc_decode = TRUE;

/* Which layer info to show in the info column */
enum layer_to_show {
    ShowPHYLayer, ShowMACLayer, ShowRLCLayer
};

/* Which layer's details to show in Info column */
static gint     global_mac_nr_layer_to_show = (gint)ShowRLCLayer;


/***********************************************************************/
/* How to dissect lcid 3-32 (presume drb logical channels)             */

static const value_string drb_lcid_vals[] = {
    { 3,  "LCID 3"},
    { 4,  "LCID 4"},
    { 5,  "LCID 5"},
    { 6,  "LCID 6"},
    { 7,  "LCID 7"},
    { 8,  "LCID 8"},
    { 9,  "LCID 9"},
    { 10, "LCID 10"},
    { 11, "LCID 11"},
    { 12, "LCID 12"},
    { 13, "LCID 13"},
    { 14, "LCID 14"},
    { 15, "LCID 15"},
    { 16, "LCID 16"},
    { 17, "LCID 17"},
    { 18, "LCID 18"},
    { 19, "LCID 19"},
    { 20, "LCID 20"},
    { 21, "LCID 21"},
    { 22, "LCID 22"},
    { 23, "LCID 23"},
    { 24, "LCID 24"},
    { 25, "LCID 25"},
    { 26, "LCID 26"},
    { 27, "LCID 27"},
    { 28, "LCID 28"},
    { 29, "LCID 29"},
    { 30, "LCID 30"},
    { 31, "LCID 31"},
    { 32, "LCID 32"},
    { 0, NULL }
};

/* N.B. for now, only doing static config, and assume channel has same SN length in both directions */
typedef enum rlc_bearer_type_t {
    rlcRaw,
    rlcTM,
    rlcUM6,
    rlcUM12,
    rlcAM12,
    rlcAM18
} rlc_bearer_type_t;

static const value_string rlc_bearer_type_vals[] = {
    { rlcTM                , "TM"},
    { rlcUM6               , "UM, SN Len=6"},
    { rlcUM12              , "UM, SN Len=12"},
    { rlcAM12              , "AM, SN Len=12"},
    { rlcAM18              , "AM, SN Len=18"},
    { 0, NULL }
};


/* Mapping type */
typedef struct lcid_drb_mapping_t {
    guint32           lcid;
    guint32           drbid;
    rlc_bearer_type_t bearer_type;
} lcid_drb_mapping_t;

/* Mapping entity */
static lcid_drb_mapping_t *lcid_drb_mappings = NULL;
static guint num_lcid_drb_mappings = 0;

UAT_VS_DEF(lcid_drb_mappings, lcid, lcid_drb_mapping_t, guint8, 3, "LCID 3")
UAT_DEC_CB_DEF(lcid_drb_mappings, drbid, lcid_drb_mapping_t)
UAT_VS_DEF(lcid_drb_mappings, bearer_type, lcid_drb_mapping_t, rlc_bearer_type_t, rlcAM12, "AM")

/* UAT object */
static uat_t* lcid_drb_mappings_uat;

/* When showing RLC info, count PDUs so can append info column properly */
static guint8   s_number_of_rlc_pdus_shown = 0;


extern int proto_rlc_nr;


/* Constants and value strings */

static const value_string radio_type_vals[] =
{
    { FDD_RADIO,      "FDD"},
    { TDD_RADIO,      "TDD"},
    { 0, NULL }
};

static const value_string direction_vals[] =
{
    { DIRECTION_UPLINK,      "Uplink"},
    { DIRECTION_DOWNLINK,    "Downlink"},
    { 0, NULL }
};

static const value_string rnti_type_vals[] =
{
    { NO_RNTI,     "NO-RNTI"},
    { P_RNTI,      "P-RNTI"},
    { RA_RNTI,     "RA-RNTI"},
    { C_RNTI,      "C-RNTI"},
    { SI_RNTI,     "SI-RNTI"},
    { CS_RNTI,     "CS-RNTI"},
    { 0, NULL }
};

static const value_string bcch_transport_channel_vals[] =
{
    { SI_RNTI,      "DL-SCH"},
    { NO_RNTI,      "BCH"},
    { 0, NULL }
};

#define CCCH_LCID                                   0x00
#define RECOMMENDED_BIT_RATE_LCID                   0x2f
#define SP_ZP_CSI_RS_RESOURCE_SET_ACT_DEACT_LCID    0x30
#define PUCCH_SPATIAL_REL_ACT_DEACT_LCID            0x31
#define SP_SRS_ACT_DEACT_LCID                       0x32
#define SP_CSI_REPORT_ON_PUCCH_ACT_DEACT_LCID       0x33
#define TCI_STATE_IND_FOR_UE_SPEC_PDCCH_LCID        0x34
#define TCI_STATES_ACT_DEACT_FOR_UE_SPEC_PDSCH_LCID 0x35
#define APER_CSI_TRIGGER_STATE_SUBSELECT_LCID       0x36
#define SP_CSI_RS_CSI_IM_RES_SET_ACT_DEACT_LCID     0x37
#define DUPLICATION_ACTIVATION_DEACTIVATION_LCID    0x38
#define SCELL_ACTIVATION_DEACTIVATION_4_LCID        0x39
#define SCELL_ACTIVATION_DEACTIVATION_1_LCID        0x3a
#define LONG_DRX_COMMAND_LCID                       0x3b
#define DRX_COMMAND_LCID                            0x3c
#define TIMING_ADVANCE_COMMAND_LCID                 0x3d
#define UE_CONTENTION_RESOLUTION_IDENTITY_LCID      0x3e
#define PADDING_LCID                                0x3f

static const value_string dlsch_lcid_vals[] =
{
    { CCCH_LCID,                                   "CCCH"},
    { 1,                                           "1"},
    { 2,                                           "2"},
    { 3,                                           "3"},
    { 4,                                           "4"},
    { 5,                                           "5"},
    { 6,                                           "6"},
    { 7,                                           "7"},
    { 8,                                           "8"},
    { 9,                                           "9"},
    { 10,                                          "10"},
    { 11,                                          "11"},
    { 12,                                          "12"},
    { 13,                                          "13"},
    { 14,                                          "14"},
    { 15,                                          "15"},
    { 16,                                          "16"},
    { 17,                                          "17"},
    { 18,                                          "18"},
    { 19,                                          "19"},
    { 20,                                          "20"},
    { 21,                                          "21"},
    { 22,                                          "22"},
    { 23,                                          "23"},
    { 24,                                          "24"},
    { 25,                                          "25"},
    { 26,                                          "26"},
    { 27,                                          "27"},
    { 28,                                          "28"},
    { 29,                                          "29"},
    { 30,                                          "30"},
    { 31,                                          "31"},
    { 32,                                          "32"},
    { RECOMMENDED_BIT_RATE_LCID,                   "Recommended Bit Rate"},
    { SP_ZP_CSI_RS_RESOURCE_SET_ACT_DEACT_LCID,    "SP ZP CSI-RS Resource Set Activation/Deactivation"},
    { PUCCH_SPATIAL_REL_ACT_DEACT_LCID,            "PUCCH spatial relation Activation/Deactivation"},
    { SP_SRS_ACT_DEACT_LCID,                       "SP SRS Activation/Deactivation"},
    { SP_CSI_REPORT_ON_PUCCH_ACT_DEACT_LCID,       "SP CSI reporting on PUCCH Activation/Deactivation"},
    { TCI_STATE_IND_FOR_UE_SPEC_PDCCH_LCID,        "TCI State Indication for UE-specific PDCCH"},
    { TCI_STATES_ACT_DEACT_FOR_UE_SPEC_PDSCH_LCID, "TCI States Activation/Deactivation for UE-specific PDSCH"},
    { APER_CSI_TRIGGER_STATE_SUBSELECT_LCID,       "Aperiodic CSI Trigger State Subselection"},
    { SP_CSI_RS_CSI_IM_RES_SET_ACT_DEACT_LCID,     "SP CSI-RS / CSI-IM Resource Set Activation/Deactivation"},
    { DUPLICATION_ACTIVATION_DEACTIVATION_LCID,    "Duplication Activation/Deactivation"},
    { SCELL_ACTIVATION_DEACTIVATION_4_LCID,        "SCell Activation/Deactivation (4 octet)"},
    { SCELL_ACTIVATION_DEACTIVATION_1_LCID,        "SCell Activation/Deactivation (1 octet)"},
    { LONG_DRX_COMMAND_LCID,                       "Long DRX Command"},
    { DRX_COMMAND_LCID,                            "DRX Command"},
    { TIMING_ADVANCE_COMMAND_LCID,                 "Timing Advance Command"},
    { UE_CONTENTION_RESOLUTION_IDENTITY_LCID,      "UE Contention Resolution Identity"},
    { PADDING_LCID,                                "Padding"},
    { 0, NULL }
};
static value_string_ext dlsch_lcid_vals_ext = VALUE_STRING_EXT_INIT(dlsch_lcid_vals);

#define CCCH_48_BITS_LCID                    0x34
#define RECOMMENDED_BIT_RATE_QUERY_LCID      0x35
#define MULTIPLE_ENTRY_PHR_4_LCID            0x36
#define CONFIGURED_GRANT_CONFIGURATION_LCID  0x37
#define MULTIPLE_ENTRY_PHR_1_LCID            0x38
#define SINGLE_ENTRY_PHR_LCID                0x39
#define C_RNTI_LCID                          0x3a
#define SHORT_TRUNCATED_BSR_LCID             0x3b
#define LONG_TRUNCATED_BSR_LCID              0x3c
#define SHORT_BSR_LCID                       0x3d
#define LONG_BSR_LCID                        0x3e
#define PADDING_LCID                         0x3f

static const value_string ulsch_lcid_vals[] =
{
    { CCCH_LCID,                            "CCCH (64 bits)"},
    { 1,                                    "1"},
    { 2,                                    "2"},
    { 3,                                    "3"},
    { 4,                                    "4"},
    { 5,                                    "5"},
    { 6,                                    "6"},
    { 7,                                    "7"},
    { 8,                                    "8"},
    { 9,                                    "9"},
    { 10,                                   "10"},
    { 11,                                   "11"},
    { 12,                                   "12"},
    { 13,                                   "13"},
    { 14,                                   "14"},
    { 15,                                   "15"},
    { 16,                                   "16"},
    { 17,                                   "17"},
    { 18,                                   "18"},
    { 19,                                   "19"},
    { 20,                                   "20"},
    { 21,                                   "21"},
    { 22,                                   "22"},
    { 23,                                   "23"},
    { 24,                                   "24"},
    { 25,                                   "25"},
    { 26,                                   "26"},
    { 27,                                   "27"},
    { 28,                                   "28"},
    { 29,                                   "29"},
    { 30,                                   "30"},
    { 31,                                   "31"},
    { 32,                                   "32"},
    { CCCH_48_BITS_LCID,                    "CCCH (48 bits)"},
    { RECOMMENDED_BIT_RATE_QUERY_LCID,      "Recommended Bit Rate Query"},
    { MULTIPLE_ENTRY_PHR_4_LCID,            "Multiple Entry PHR (4 octet C)"},
    { CONFIGURED_GRANT_CONFIGURATION_LCID,  "Configured Grant Confirmation"},
    { MULTIPLE_ENTRY_PHR_1_LCID,            "Multiple Entry PHR (1 octet C)"},
    { SINGLE_ENTRY_PHR_LCID,                "Single Entry PHR"},
    { C_RNTI_LCID,                          "C-RNTI"},
    { SHORT_TRUNCATED_BSR_LCID,             "Short Truncated BSR"},
    { LONG_TRUNCATED_BSR_LCID,              "Long Truncated BSR"},
    { SHORT_BSR_LCID,                       "Short BSR"},
    { LONG_BSR_LCID,                        "Long BSR"},
    { PADDING_LCID,                         "Padding"},
    { 0, NULL }
};
static value_string_ext ulsch_lcid_vals_ext = VALUE_STRING_EXT_INIT(ulsch_lcid_vals);

static const true_false_string rar_ext_vals =
{
    "Another MAC subPDU follows",
    "Last MAC subPDU"
};

static const true_false_string rar_type_vals =
{
    "RAPID present",
    "Backoff Indicator present"
};

static const value_string rar_bi_vals[] =
{
    { 0,  "5ms"},
    { 1,  "10ms"},
    { 2,  "20ms"},
    { 3,  "30ms"},
    { 4,  "40ms"},
    { 5,  "60ms"},
    { 6,  "80ms"},
    { 7,  "120ms"},
    { 8,  "160ms"},
    { 9,  "240ms"},
    { 10, "320ms"},
    { 11, "480ms"},
    { 12, "960ms"},
    { 13, "1920ms"},
    { 14, "Reserved"},
    { 15, "Reserved"},
    { 0, NULL }
};

static const value_string buffer_size_5bits_vals[] =
{
    { 0,  "BS = 0"},
    { 1,  "0 < BS <= 10"},
    { 2,  "10 < BS <= 14"},
    { 3,  "14 < BS <= 20"},
    { 4,  "20 < BS <= 28"},
    { 5,  "28 < BS <= 38"},
    { 6,  "38 < BS <= 53"},
    { 7,  "53 < BS <= 74"},
    { 8,  "74 < BS <= 102"},
    { 9,  "102 < BS <= 142"},
    { 10, "142 < BS <= 198"},
    { 11, "198 < BS <= 276"},
    { 12, "276 < BS <= 384"},
    { 13, "384 < BS <= 535"},
    { 14, "535 < BS <= 745"},
    { 15, "745 < BS <= 1038"},
    { 16, "1038 < BS <= 1446"},
    { 17, "1446 < BS <= 2014"},
    { 18, "2014 < BS <= 2806"},
    { 19, "2806 < BS <= 3909"},
    { 20, "3909 < BS <= 5446"},
    { 21, "5446 < BS <= 7587"},
    { 22, "7587 < BS <= 10570"},
    { 23, "10570 < BS <= 14726"},
    { 24, "14726 < BS <= 20516"},
    { 25, "20516 < BS <= 28581"},
    { 26, "28581 < BS <= 39818"},
    { 27, "39818 < BS <= 55474"},
    { 28, "55474 < BS <= 77284"},
    { 29, "77284 < BS <= 107669"},
    { 30, "107669 < BS <= 150000"},
    { 31, "BS > 150000"},
    { 0, NULL }
};
static value_string_ext buffer_size_5bits_vals_ext = VALUE_STRING_EXT_INIT(buffer_size_5bits_vals);


static const value_string buffer_size_8bits_vals[] =
{
    { 0,   "BS = 0"},
    { 1,   "0 < BS <= 10"},
    { 2,   "10 < BS <= 11"},
    { 3,   "11 < BS <= 12"},
    { 4,   "12 < BS <= 13"},
    { 5,   "13 < BS <= 14"},
    { 6,   "14 < BS <= 15"},
    { 7,   "15 < BS <= 16"},
    { 8,   "16 < BS <= 17"},
    { 9,  "17 < BS <= 18"},
    { 10,  "18 < BS <= 19"},
    { 11,  "19 < BS <= 20"},
    { 12,  "20 < BS <= 22"},
    { 13,  "22 < BS <= 23"},
    { 14,  "23 < BS <= 25"},
    { 15,  "25 < BS <= 26"},
    { 16,  "26 < BS <= 28"},
    { 17,  "28 < BS <= 30"},
    { 18,  "30 < BS <= 32"},
    { 19,  "32 < BS <= 34"},
    { 20,  "34 < BS <= 36"},
    { 21,  "36 < BS <= 38"},
    { 22,  "38 < BS <= 40"},
    { 23,  "40 < BS <= 43"},
    { 24,  "43 < BS <= 46"},
    { 25,  "46 < BS <= 49"},
    { 26,  "49 < BS <= 52"},
    { 27,  "52 < BS <= 55"},
    { 28,  "52 < BS <= 59"},
    { 29,  "59 < BS <= 62"},
    { 30,  "62 < BS <= 66"},
    { 31,  "66 < BS <= 71"},
    { 32,  "71 < BS <= 75"},
    { 33,  "75 < BS <= 80"},
    { 34,  "80 < BS <= 85"},
    { 35,  "85 < BS <= 91"},
    { 36,  "91 < BS <= 97"},
    { 37,  "97 < BS <= 103"},
    { 38,  "103 < BS <= 110"},
    { 39,  "110 < BS <= 117"},
    { 40,  "117 < BS <= 124"},
    { 41,  "124 < BS <= 132"},
    { 42,  "132 < BS <= 141"},
    { 43,  "141 < BS <= 150"},
    { 44,  "150 < BS <= 160"},
    { 45,  "160 < BS <= 170"},
    { 46,  "170 < BS <= 181"},
    { 47,  "181 < BS <= 193"},
    { 48,  "193 < BS <= 205"},
    { 49,  "205 < BS <= 218"},
    { 50,  "218 < BS <= 233"},
    { 51,  "233 < BS <= 248"},
    { 52,  "248 < BS <= 264"},
    { 53,  "264 < BS <= 281"},
    { 54,  "281 < BS <= 299"},
    { 55,  "299 < BS <= 318"},
    { 56,  "318 < BS <= 339"},
    { 57,  "339 < BS <= 361"},
    { 58,  "361 < BS <= 384"},
    { 59,  "384 < BS <= 409"},
    { 60,  "409 < BS <= 436"},
    { 61,  "436 < BS <= 464"},
    { 62,  "464 < BS <= 494"},
    { 63,  "494 < BS <= 526"},
    { 64,  "526 < BS <= 560"},
    { 65,  "560 < BS <= 597"},
    { 66,  "597 < BS <= 635"},
    { 67,  "635 < BS <= 677"},
    { 68,  "677 < BS <= 720"},
    { 69,  "720 < BS <= 767"},
    { 70,  "767 < BS <= 817"},
    { 71,  "817 < BS <= 870"},
    { 72,  "870 < BS <= 926"},
    { 73,  "926 < BS <= 987"},
    { 74,  "987 < BS <= 1051"},
    { 75,  "1051 < BS <= 1119"},
    { 76,  "1119 < BS <= 1191"},
    { 77,  "1191 < BS <= 1269"},
    { 78,  "1269 < BS <= 1351"},
    { 79,  "1351 < BS <= 1439"},
    { 80,  "1439 < BS <= 1532"},
    { 81,  "1532 < BS <= 1631"},
    { 82,  "1631 < BS <= 1737"},
    { 83,  "1737 < BS <= 1850"},
    { 84,  "1850 < BS <= 1970"},
    { 85,  "1970 < BS <= 2098"},
    { 86,  "2098 < BS <= 2234"},
    { 87,  "2234 < BS <= 2379"},
    { 88,  "2379 < BS <= 2533"},
    { 89,  "2533 < BS <= 2698"},
    { 90,  "2698 < BS <= 2873"},
    { 91,  "2873 < BS <= 3059"},
    { 92,  "3059 < BS <= 3258"},
    { 93,  "3258 < BS <= 3469"},
    { 94,  "3469 < BS <= 3694"},
    { 95,  "3694 < BS <= 3934"},
    { 96,  "3934 < BS <= 4189"},
    { 97,  "4189 < BS <= 4461"},
    { 98,  "4461 < BS <= 4751"},
    { 99, "4751 < BS <= 5059"},
    { 100, "5059 < BS <= 5387"},
    { 101, "5387 < BS <= 5737"},
    { 102, "5737 < BS <= 6109"},
    { 103, "6109 < BS <= 6506"},
    { 104, "6506 < BS <= 6928"},
    { 105, "6928 < BS <= 7378"},
    { 106, "7378 < BS <= 7857"},
    { 107, "7857 < BS <= 8367"},
    { 108, "8367 < BS <= 8910"},
    { 109, "8910 < BS <= 9488"},
    { 110, "9488 < BS <= 10104"},
    { 111, "10104 < BS <= 10760"},
    { 112, "10760 < BS <= 11458"},
    { 113, "11458 < BS <= 12202"},
    { 114, "12202 < BS <= 12994"},
    { 115, "12994 < BS <= 13838"},
    { 116, "13838 < BS <= 14736"},
    { 117, "14736 < BS <= 15692"},
    { 118, "15692 < BS <= 16711"},
    { 119, "16711 < BS <= 17795"},
    { 120, "17795 < BS <= 18951"},
    { 121, "18951 < BS <= 20181"},
    { 122, "20181 < BS <= 21491"},
    { 123, "21491 < BS <= 22885"},
    { 124, "22885 < BS <= 24371"},
    { 125, "24371 < BS <= 25953"},
    { 126, "25953 < BS <= 27638"},
    { 127, "27638 < BS <= 29431"},
    { 128, "29431 < BS <= 31342"},
    { 129, "31342 < BS <= 33376"},
    { 130, "33376 < BS <= 35543"},
    { 131, "35543 < BS <= 37850"},
    { 132, "37850 < BS <= 40307"},
    { 133, "40307 < BS <= 42923"},
    { 134, "42923 < BS <= 45709"},
    { 135, "45709 < BS <= 48676"},
    { 136, "48676 < BS <= 51836"},
    { 137, "51836 < BS <= 55200"},
    { 138, "55200 < BS <= 58784"},
    { 139, "58784 < BS <= 62599"},
    { 140, "62599 < BS <= 66663"},
    { 141, "66663 < BS <= 70990"},
    { 142, "70990 < BS <= 75598"},
    { 143, "75598 < BS <= 80505"},
    { 144, "80505 < BS <= 85730"},
    { 145, "85730 < BS <= 91295"},
    { 146, "91295 < BS <= 97221"},
    { 147, "97221 < BS <= 103532"},
    { 148, "103532 < BS <= 110252"},
    { 149, "110252 < BS <= 117409"},
    { 150, "117409 < BS <= 125030"},
    { 151, "125030 < BS <= 133146"},
    { 152, "133146 < BS <= 141789"},
    { 153, "141789 < BS <= 150992"},
    { 154, "150992 < BS <= 160793"},
    { 155, "160793 < BS <= 171231"},
    { 156, "171231 < BS <= 182345"},
    { 157, "182345 < BS <= 194182"},
    { 158, "194182 < BS <= 206786"},
    { 159, "206786 < BS <= 220209"},
    { 160, "220209 < BS <= 234503"},
    { 161, "234503 < BS <= 249725"},
    { 162, "249725 < BS <= 265935"},
    { 163, "265935 < BS <= 283197"},
    { 164, "283197 < BS <= 301579"},
    { 165, "301579 < BS <= 321155"},
    { 166, "321155 < BS <= 342002"},
    { 167, "342002 < BS <= 364202"},
    { 168, "364202 < BS <= 387842"},
    { 169, "387842 < BS <= 413018"},
    { 170, "413018 < BS <= 439827"},
    { 171, "439827 < BS <= 468377"},
    { 172, "468377 < BS <= 498780"},
    { 173, "498780 < BS <= 531156"},
    { 174, "531156 < BS <= 565634"},
    { 175, "565634 < BS <= 602350"},
    { 176, "602350 < BS <= 641449"},
    { 177, "641449 < BS <= 683087"},
    { 178, "683087 < BS <= 727427"},
    { 179, "727427 < BS <= 774645"},
    { 180, "774645 < BS <= 824928"},
    { 181, "824928 < BS <= 878475"},
    { 182, "878475 < BS <= 935498"},
    { 183, "935498 < BS <= 996222"},
    { 184, "996222 < BS <= 1060888"},
    { 185, "1060888 < BS <= 1129752"},
    { 186, "1129752 < BS <= 1203085"},
    { 187, "1203085 < BS <= 1281179"},
    { 188, "1281179 < BS <= 1364342"},
    { 189, "1364342 < BS <= 1452903"},
    { 190, "1452903 < BS <= 1547213"},
    { 191, "1547213 < BS <= 1647644"},
    { 192, "1647644 < BS <= 1754595"},
    { 193, "1754595 < BS <= 1868488"},
    { 194, "1868488 < BS <= 1989774"},
    { 195, "1989774 < BS <= 2118933"},
    { 196, "2118933 < BS <= 2256475"},
    { 197, "2256475 < BS <= 2402946"},
    { 198, "2402946 < BS <= 2558924"},
    { 199, "2558924 < BS <= 2725027"},
    { 200, "2725027 < BS <= 2901912"},
    { 201, "2901912 < BS <= 3090279"},
    { 202, "3090279 < BS <= 3290873"},
    { 203, "3290873 < BS <= 3504487"},
    { 204, "3504487 < BS <= 3731968"},
    { 205, "3731968 < BS <= 3974215"},
    { 206, "3974215 < BS <= 4232186"},
    { 207, "4232186 < BS <= 4506902"},
    { 208, "4506902 < BS <= 4799451"},
    { 209, "4799451 < BS <= 5110989"},
    { 210, "5110989 < BS <= 5442750"},
    { 211, "5442750 < BS <= 5796046"},
    { 212, "5796046 < BS <= 6172275"},
    { 213, "6172275 < BS <= 6572925"},
    { 214, "6572925 < BS <= 6999582"},
    { 215, "6999582 < BS <= 7453933"},
    { 216, "7453933 < BS <= 7937777"},
    { 217, "7937777 < BS <= 8453028"},
    { 218, "8453028 < BS <= 9001725"},
    { 219, "9001725 < BS <= 9586039"},
    { 220, "9586039 < BS <= 10208280"},
    { 221, "10208280 < BS <= 10870913"},
    { 222, "10870913 < BS <= 11576557"},
    { 223, "11576557 < BS <= 12328006"},
    { 224, "12328006 < BS <= 13128233"},
    { 225, "13128233 < BS <= 13980403"},
    { 226, "13980403 < BS <= 14887889"},
    { 227, "14887889 < BS <= 15854280"},
    { 228, "15854280 < BS <= 16883401"},
    { 229, "16883401 < BS <= 17979324"},
    { 230, "17979324 < BS <= 19146385"},
    { 231, "19146385 < BS <= 20389201"},
    { 232, "20389201 < BS <= 21712690"},
    { 233, "21712690 < BS <= 23122088"},
    { 234, "23122088 < BS <= 24622972"},
    { 235, "24622972 < BS <= 26221280"},
    { 236, "26221280 < BS <= 27923336"},
    { 237, "27923336 < BS <= 29735875"},
    { 238, "29735875 < BS <= 31666069"},
    { 239, "31666069 < BS <= 33721553"},
    { 240, "33721553 < BS <= 35910462"},
    { 241, "35910462 < BS <= 38241455"},
    { 242, "38241455 < BS <= 40723756"},
    { 243, "40723756 < BS <= 43367187"},
    { 244, "43367187 < BS <= 46182206"},
    { 245, "46182206 < BS <= 49179951"},
    { 246, "49179951 < BS <= 52372284"},
    { 247, "52372284 < BS <= 55771835"},
    { 248, "55771835 < BS <= 59392055"},
    { 249, "59392055 < BS <= 63247269"},
    { 250, "63247269 < BS <= 67352729"},
    { 251, "67352729 < BS <= 71724679"},
    { 252, "71724679 < BS <= 76380419"},
    { 253, "76380419 < BS <= 81338368"},
    { 254, "BS > 81338368"},
    { 255, "Reserved"},
    { 0, NULL }
};
static value_string_ext buffer_size_8bits_vals_ext = VALUE_STRING_EXT_INIT(buffer_size_8bits_vals);

static const value_string tpc_command_vals[] =
{
    { 0,   "-6dB"},
    { 1,   "-4dB"},
    { 2,   "-2dB"},
    { 3,   "0dB"},
    { 4,   "2dB"},
    { 5,   "4dB"},
    { 6,   "6dB"},
    { 7,   "8dB"},
    { 0,   NULL }
};



static const true_false_string power_backoff_affects_power_management_vals =
{
    "Power backoff is applied to power management",
    "Power backoff not applied to power management"
};

static const true_false_string phr_source_vals =
{
    "PH based on reference format",
    "PH based on real transmission",
};

static const true_false_string activation_deactivation_vals =
{
    "Activation",
    "Deactivation"
};

static const true_false_string c_vals =
{
    "Octets containing Resource Serving Cell ID field(s) and Resource BWP ID field(s) are present",
    "Octets containing Resource Serving Cell ID field(s) and Resource BWP ID field(s) are not present"
};

static const true_false_string sul_vals =
{
    "Applies to the SUL carrier configuration",
    "Applies to the NUL carrier configuration"
};

static const true_false_string sp_srs_act_deact_f_vals =
{
    "NZP CSI-RS resource index is used",
    "SSB index or SRS resource index is used"
};

static const true_false_string aper_csi_trigger_state_t_vals =
{
    "Mapped to the codepoint of the DCI CSI request field",
    "Not mapped to the codepoint of the DCI CSI request field"
};

static const value_string bit_rate_vals[] =
{
    { 0, "no bit rate recommendation"},
    { 1, "0 kbit/s"},
    { 2, "9 kbit/s"},
    { 3, "11 kbit/s"},
    { 4, "13 kbit/s"},
    { 5, "17 kbit/s"},
    { 6, "21 kbit/s"},
    { 7, "25 kbit/s"},
    { 8, "29 kbit/s"},
    { 9, "32 kbit/s"},
    { 10, "36 kbit/s"},
    { 11, "40 kbit/s"},
    { 12, "48 kbit/s"},
    { 13, "56 kbit/s"},
    { 14, "72 kbit/s"},
    { 15, "88 kbit/s"},
    { 16, "104 kbit/s"},
    { 17, "120 kbit/s"},
    { 18, "140 kbit/s"},
    { 19, "160 kbit/s"},
    { 20, "180 kbit/s"},
    { 21, "200 kbit/s"},
    { 22, "220 kbit/s"},
    { 23, "240 kbit/s"},
    { 24, "260 kbit/s"},
    { 25, "280 kbit/s"},
    { 26, "300 kbit/s"},
    { 27, "350 kbit/s"},
    { 28, "400 kbit/s"},
    { 29, "450 kbit/s"},
    { 30, "500 kbit/s"},
    { 31, "600 kbit/s"},
    { 32, "700 kbit/s"},
    { 33, "800 kbit/s"},
    { 34, "900 kbit/s"},
    { 35, "1000 kbit/s"},
    { 36, "1100 kbit/s"},
    { 37, "1200 kbit/s"},
    { 38, "1300 kbit/s"},
    { 39, "1400 kbit/s"},
    { 40, "1500 kbit/s"},
    { 41, "1750 kbit/s"},
    { 42, "2000 kbit/s"},
    { 43, "2250 kbit/s"},
    { 44, "2500 kbit/s"},
    { 45, "2750 kbit/s"},
    { 46, "3000 kbit/s"},
    { 47, "3500 kbit/s"},
    { 48, "4000 kbit/s"},
    { 49, "4500 kbit/s"},
    { 50, "5000 kbit/s"},
    { 51, "5500 kbit/s"},
    { 52, "6000 kbit/s"},
    { 53, "6500 kbit/s"},
    { 54, "7000 kbit/s"},
    { 55, "7500 kbit/s"},
    { 56, "8000 kbit/s"},
    { 0, NULL }
};
static value_string_ext bit_rate_vals_ext = VALUE_STRING_EXT_INIT(bit_rate_vals);

/* Forward declarations */
static int dissect_mac_nr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void*);

/* Write the given formatted text to:
   - the info column (if pinfo != NULL)
   - 1 or 2 other labels (optional)
*/
static void write_pdu_label_and_info(proto_item *ti1, proto_item *ti2,
                                     packet_info *pinfo, const char *format, ...)
{
    #define MAX_INFO_BUFFER 256
    static char info_buffer[MAX_INFO_BUFFER];
    va_list ap;

    if ((ti1 == NULL) && (ti2 == NULL) && (pinfo == NULL)) {
        return;
    }

    va_start(ap, format);
    g_vsnprintf(info_buffer, MAX_INFO_BUFFER, format, ap);
    va_end(ap);

    /* Add to indicated places */
    if (pinfo != NULL) {
        col_append_str(pinfo->cinfo, COL_INFO, info_buffer);
    }
    if (ti1 != NULL) {
        proto_item_append_text(ti1, "%s", info_buffer);
    }
    if (ti2 != NULL) {
        proto_item_append_text(ti2, "%s", info_buffer);
    }
}

/* Version of function above, where no g_vsnprintf() call needed */
static void write_pdu_label_and_info_literal(proto_item *ti1, proto_item *ti2,
                                             packet_info *pinfo, const char *info_buffer)
{
    /* Add to indicated places */
    if (pinfo != NULL) {
        col_append_str(pinfo->cinfo, COL_INFO, info_buffer);
    }
    if (ti1 != NULL) {
        proto_item_append_text(ti1, "%s", info_buffer);
    }
    if (ti2 != NULL) {
        proto_item_append_text(ti2, "%s", info_buffer);
    }
}

static void
call_with_catch_all(dissector_handle_t handle, tvbuff_t* tvb, packet_info *pinfo, proto_tree *tree)
{
    /* Call it (catch exceptions so that stats will be updated) */
    if (handle) {
        TRY {
            call_dissector_only(handle, tvb, pinfo, tree, NULL);
        }
        CATCH_ALL {
        }
        ENDTRY
    }
}

/* Dissect BCCH PDU */
static void dissect_bcch(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                         proto_item *pdu_ti,
                         int offset, mac_nr_info *p_mac_nr_info)
{
    proto_item *ti;

    write_pdu_label_and_info(pdu_ti, NULL, pinfo,
                             "BCCH PDU (%u bytes, on %s transport)  ",
                             tvb_reported_length_remaining(tvb, offset),
                             val_to_str_const(p_mac_nr_info->rntiType,
                                              bcch_transport_channel_vals,
                                              "Unknown"));

    /* Show which transport layer it came in on (inferred from RNTI type) */
    ti = proto_tree_add_uint(tree, hf_mac_nr_context_bcch_transport_channel,
                             tvb, offset, 0, p_mac_nr_info->rntiType);
    PROTO_ITEM_SET_GENERATED(ti);

    /****************************************/
    /* Whole frame is BCCH data             */

    /* Raw data */
    ti = proto_tree_add_item(tree, hf_mac_nr_bcch_pdu,
                             tvb, offset, -1, ENC_NA);

    if (global_mac_nr_attempt_rrc_decode) {
        /* Attempt to decode payload using NR RRC dissector */
        dissector_handle_t protocol_handle;
        tvbuff_t *rrc_tvb = tvb_new_subset_remaining(tvb, offset);

        if (p_mac_nr_info->rntiType == NO_RNTI) {
            protocol_handle = nr_rrc_bcch_bch_handle;
        } else {
            protocol_handle = nr_rrc_bcch_dl_sch_handle;
        }

        /* Hide raw view of bytes */
        PROTO_ITEM_SET_HIDDEN(ti);

        call_with_catch_all(protocol_handle, rrc_tvb, pinfo, tree);
    }
}

/* Dissect PCCH PDU */
static void dissect_pcch(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                         proto_item *pdu_ti, int offset, mac_nr_info *p_mac_nr_info _U_)
{
    proto_item *ti;

    write_pdu_label_and_info(pdu_ti, NULL, pinfo,
                             "PCCH PDU (%u bytes)  ",
                             tvb_reported_length_remaining(tvb, offset));

    /****************************************/
    /* Whole frame is PCCH data             */

    /* Always show as raw data */
    ti = proto_tree_add_item(tree, hf_mac_nr_pcch_pdu,
                             tvb, offset, -1, ENC_NA);

    if (global_mac_nr_attempt_rrc_decode) {

        /* Attempt to decode payload using NR RRC dissector */
        tvbuff_t *rrc_tvb = tvb_new_subset_remaining(tvb, offset);

        /* Hide raw view of bytes */
        PROTO_ITEM_SET_HIDDEN(ti);

        call_with_catch_all(nr_rrc_pcch_handle, rrc_tvb, pinfo, tree);

    }
}

static void dissect_rar(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
                        proto_item *pdu_ti _U_, guint32 offset,
                        mac_nr_info *p_mac_nr_info _U_)
{
    write_pdu_label_and_info(pdu_ti, NULL, pinfo,
                             "RAR (RA-RNTI=%u, SFN=%-4u, SF=%u) ",
                             p_mac_nr_info->rnti, p_mac_nr_info->sysframeNumber, p_mac_nr_info->subframeNumber);

    /* Create hidden 'virtual root' so can filter on mac-nr.rar */
    proto_item *ti = proto_tree_add_item(tree, hf_mac_nr_rar, tvb, offset, -1, ENC_NA);
    PROTO_ITEM_SET_HIDDEN(ti);

    gboolean E, T;

    do {
        /* Subheader */
        proto_item *subheader_ti = proto_tree_add_item(tree,
                                                       hf_mac_nr_rar_subheader,
                                                       tvb, offset, 0, ENC_ASCII|ENC_NA);
        proto_tree *rar_subheader_tree = proto_item_add_subtree(subheader_ti, ett_mac_nr_rar_subheader);

        /* Note extension & T bits */
        proto_tree_add_item_ret_boolean(rar_subheader_tree, hf_mac_nr_rar_e, tvb, offset, 1, ENC_BIG_ENDIAN, &E);
        proto_tree_add_item_ret_boolean(rar_subheader_tree, hf_mac_nr_rar_t, tvb, offset, 1, ENC_BIG_ENDIAN, &T);

        if (!T) {
            /* BI */

            /* 2 reserved bits */
            proto_tree_add_item(rar_subheader_tree, hf_mac_nr_rar_reserved, tvb, offset, 1, ENC_BIG_ENDIAN);

            /* BI (4 bits) */
            guint32 BI;
            proto_tree_add_item_ret_uint(rar_subheader_tree, hf_mac_nr_rar_bi, tvb, offset, 1, ENC_BIG_ENDIAN, &BI);
            offset++;

            write_pdu_label_and_info(pdu_ti, subheader_ti, pinfo,
                                     "BI=%u ", BI);
        }
        else {
            /* RAPID */
            guint32 rapid;
            proto_tree_add_item_ret_uint(rar_subheader_tree, hf_mac_nr_rar_rapid, tvb, offset, 1, ENC_BIG_ENDIAN, &rapid);
            offset++;

            if (TRUE) {
                /* SubPDU.  Not for SI request - TODO: define RAPID range for SI request in mac_nr_info */

                /* 1 reserved bit */
                proto_tree_add_item(rar_subheader_tree, hf_mac_nr_rar_reserved1, tvb, offset, 1, ENC_BIG_ENDIAN);

                /* TA (12 bits) */
                guint32 ta;
                proto_tree_add_item_ret_uint(rar_subheader_tree, hf_mac_nr_rar_ta, tvb, offset, 2, ENC_BIG_ENDIAN, &ta);
                offset++;

                /* Break down the 27-bits of the grant field, according to 38.213, section 8.2 */
                static const int *rar_grant_fields[] = {
                    &hf_mac_nr_rar_grant_hopping,
                    &hf_mac_nr_rar_grant_fra,
                    &hf_mac_nr_rar_grant_tsa,
                    &hf_mac_nr_rar_grant_mcs,
                    &hf_mac_nr_rar_grant_tcsp,
                    &hf_mac_nr_rar_grant_csi,
                    NULL
                };

                proto_tree_add_bitmask(rar_subheader_tree, tvb, offset, hf_mac_nr_rar_grant,
                                       ett_mac_nr_rar_grant, rar_grant_fields, ENC_BIG_ENDIAN);
                offset += 4;

                /* C-RNTI (2 bytes) */
                guint32 c_rnti;
                proto_tree_add_item_ret_uint(rar_subheader_tree, hf_mac_nr_rar_temp_crnti, tvb, offset, 2, ENC_BIG_ENDIAN, &c_rnti);
                offset += 2;

                write_pdu_label_and_info(pdu_ti, subheader_ti, pinfo,
                                         "(RAPID=%u TA=%u Temp C-RNTI=%u) ", rapid, ta, c_rnti);
            }
            else {
                write_pdu_label_and_info(pdu_ti, subheader_ti, pinfo,
                                         "(RAPID=%u) ", rapid);
            }
        }
        /* Set subheader (+subpdu..) length */
        proto_item_set_end(subheader_ti, tvb, offset);

    } while (E);

    /* Any remaining length is padding */
    if (tvb_reported_length_remaining(tvb, offset)) {
        proto_tree_add_item(tree, hf_mac_nr_padding, tvb, offset, -1, ENC_NA);
    }
}

static gboolean is_fixed_sized_lcid(guint8 lcid, guint8 direction)
{
    if (direction == DIRECTION_UPLINK) {
        switch (lcid) {
            case CCCH_LCID:
            case CCCH_48_BITS_LCID:
            case RECOMMENDED_BIT_RATE_QUERY_LCID:
            case CONFIGURED_GRANT_CONFIGURATION_LCID:
            case SINGLE_ENTRY_PHR_LCID:
            case C_RNTI_LCID:
            case SHORT_TRUNCATED_BSR_LCID:
            case SHORT_BSR_LCID:
            case PADDING_LCID:
                return TRUE;
            default:
                return FALSE;
        }
    }
    else {
        switch (lcid) {
            case RECOMMENDED_BIT_RATE_LCID:
            case SP_ZP_CSI_RS_RESOURCE_SET_ACT_DEACT_LCID:
            case PUCCH_SPATIAL_REL_ACT_DEACT_LCID:
            case SP_CSI_REPORT_ON_PUCCH_ACT_DEACT_LCID:
            case TCI_STATE_IND_FOR_UE_SPEC_PDCCH_LCID:
            case DUPLICATION_ACTIVATION_DEACTIVATION_LCID:
            case SCELL_ACTIVATION_DEACTIVATION_4_LCID:
            case SCELL_ACTIVATION_DEACTIVATION_1_LCID:
            case LONG_DRX_COMMAND_LCID:
            case DRX_COMMAND_LCID:
            case TIMING_ADVANCE_COMMAND_LCID:
            case UE_CONTENTION_RESOLUTION_IDENTITY_LCID:
            case PADDING_LCID:
                return TRUE;
            default:
                return FALSE;
        }
    }
}

static true_false_string subheader_f_vals = {
    "16 bits",
    "8 bits"
};


/* Returns new subtree that was added for this item */
static proto_item* dissect_me_phr_ph(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree,
                                    const int *ph_item, const int *pcmax_f_c_item,
                                    guint32 *PH, guint32 *offset)
{
    /* Subtree for this entry */
    proto_item *entry_ti = proto_tree_add_item(tree,
                                               hf_mac_nr_control_me_phr_entry,
                                               tvb, *offset, 0, ENC_ASCII|ENC_NA);
    proto_tree *entry_tree = proto_item_add_subtree(entry_ti, ett_mac_nr_me_phr_entry);

    /* P */
    proto_tree_add_item(entry_tree, hf_mac_nr_control_me_phr_p, tvb, *offset, 1, ENC_BIG_ENDIAN);
    /* V */
    gboolean V;
    proto_tree_add_item_ret_boolean(entry_tree, hf_mac_nr_control_me_phr_v, tvb, *offset, 1, ENC_BIG_ENDIAN, &V);
    /* PH. TODO: infer whether value relates to Type1 (PUSCH), Type2 (PUCCH) or Type3 (SRS).
       And decide whether:
       - there needs to be a separate field for each SCellIndex and type OR
       - a generated field added with the inferred type OR
       - just do proto_item_append_text() indicating what the type was
     */
    proto_tree_add_item_ret_uint(entry_tree, *ph_item, tvb, *offset, 1, ENC_BIG_ENDIAN, PH);
    (*offset)++;

    if (!V) {
        /* Reserved (2 bits) */
        proto_tree_add_item(entry_tree, hf_mac_nr_control_me_phr_reserved_2, tvb, *offset, 1, ENC_BIG_ENDIAN);
        /* pcmax_f_c (6 bits) */
        proto_tree_add_item(entry_tree, *pcmax_f_c_item, tvb, *offset, 1, ENC_BIG_ENDIAN);
        (*offset)++;
    }

    proto_item_set_end(entry_ti, tvb, *offset);
    return entry_ti;
}


static void set_rlc_seqnum_length(rlc_bearer_type_t rlc_bearer_type,
                                  guint8 direction _U_,
                                  guint8 *seqnum_length)
{
    switch (rlc_bearer_type) {
        case rlcUM6:
            *seqnum_length = 6;
            break;
        case rlcUM12:
            *seqnum_length = 12;
            break;

        case rlcAM12:
            *seqnum_length = 12;
            break;
        case rlcAM18:
            *seqnum_length = 18;
            break;

        default:
            break;
    }
}



/* Lookup channel details for lcid */
static void lookup_rlc_channel_from_lcid(guint16 ueid _U_,
                                         guint8 lcid,
                                         guint8 direction,
                                         rlc_bearer_type_t *rlc_bearer_type,
                                         guint8 *seqnum_length,
                                         gint *drb_id)
{
    /* Zero params (in case no match is found) */
    *rlc_bearer_type = rlcRaw;
    *seqnum_length    = 0;
    *drb_id           = 0;

    /* Look up in static (UAT) table */
    guint m;
    for (m=0; m < num_lcid_drb_mappings; m++) {
        if (lcid == lcid_drb_mappings[m].lcid) {

            *rlc_bearer_type = lcid_drb_mappings[m].bearer_type;

            /* Set seqnum_length and rlc_ext_li_field */
            set_rlc_seqnum_length(*rlc_bearer_type, direction, seqnum_length);

            /* Set drb_id */
            *drb_id = lcid_drb_mappings[m].drbid;
            break;
        }
    }
}


/* Helper function to call RLC dissector for SDUs (where channel params are known) */
static void call_rlc_dissector(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                               proto_item *pdu_ti,
                               int offset, guint16 data_length,
                               guint8 mode, guint8 direction, guint16 ueid,
                               guint8 bearerType, guint8 bearerId,
                               guint8 sequenceNumberLength,
                               guint8 priority _U_)
{
    tvbuff_t            *rb_tvb = tvb_new_subset_length(tvb, offset, data_length);
    struct rlc_nr_info *p_rlc_nr_info;

    /* Resuse or create RLC info */
    p_rlc_nr_info = (rlc_nr_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_rlc_nr, 0);
    if (p_rlc_nr_info == NULL) {
        p_rlc_nr_info = wmem_new0(wmem_file_scope(), struct rlc_nr_info);
    }

    /* Fill in details for channel */
    p_rlc_nr_info->rlcMode = mode;
    p_rlc_nr_info->direction = direction;
    /* p_rlc_nr_info->priority = priority; */
    p_rlc_nr_info->ueid = ueid;
    p_rlc_nr_info->bearerType = bearerType;
    p_rlc_nr_info->bearerId = bearerId;
    p_rlc_nr_info->pduLength = data_length;
    p_rlc_nr_info->sequenceNumberLength = sequenceNumberLength;

    /* Store info in packet */
    p_add_proto_data(wmem_file_scope(), pinfo, proto_rlc_nr, 0, p_rlc_nr_info);

    if (global_mac_nr_layer_to_show != ShowRLCLayer) {
        /* Don't want these columns replaced */
        col_set_writable(pinfo->cinfo, -1, FALSE);
    }
    else {
        /* Clear info column before first RLC PDU */
        if (s_number_of_rlc_pdus_shown == 0) {
            col_clear(pinfo->cinfo, COL_INFO);
        }
        else {
            /* Add a separator and protect column contents here */
            write_pdu_label_and_info_literal(pdu_ti, NULL, pinfo, "   ||   ");
            col_set_fence(pinfo->cinfo, COL_INFO);
        }
    }
    s_number_of_rlc_pdus_shown++;

    /* Call it (catch exceptions so that stats will be updated) */
    call_with_catch_all(rlc_nr_handle, rb_tvb, pinfo, tree);

    /* Let columns be written to again */
    col_set_writable(pinfo->cinfo, -1, TRUE);
}

/* see 3GPP 38.133 Table 10.1.17.1-1 */
static void
mac_nr_phr_fmt(gchar *s, guint32 v)
{
    gint32 val = (gint32)v;

    if (val == 0) {
        g_snprintf(s, ITEM_LABEL_LENGTH, "PH < -32 dB (0)");
    } else if (val == 63) {
        g_snprintf(s, ITEM_LABEL_LENGTH, "PH >= 38 dB (63)");
    } else if (val <= 54) {
        g_snprintf(s, ITEM_LABEL_LENGTH, "%d dB <= PH < %d dB (%d)", val - 33, val - 32, val);
    } else {
        g_snprintf(s, ITEM_LABEL_LENGTH, "%d dB <= PH < %d dB (%d)", 22 + 2 * (val - 55), 24 + 2 * (val - 55), val);
    }
}

/* see 3GPP 38.133 Table 10.1.18.1-1 */
static void
mac_nr_pcmax_f_c_fmt(gchar *s, guint32 v)
{
    gint32 val = (gint32)v;

    if (val == 0) {
        g_snprintf(s, ITEM_LABEL_LENGTH, "Pcmax,f,c < -29 dBm (0)");
    } else if (val == 63) {
        g_snprintf(s, ITEM_LABEL_LENGTH, "Pcmax,f,c >= 33 dBm (63)");
    } else {
        g_snprintf(s, ITEM_LABEL_LENGTH, "%d dBm <= Pcmax,f,c < %d dBm (%d)", val - 30, val - 29, val);
    }
}

/* UL-SCH and DL-SCH formats have much in common, so handle them in a common
   function */
static void dissect_ulsch_or_dlsch(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree,
                                   proto_item *pdu_ti, guint32 offset,
                                   mac_nr_info *p_mac_nr_info,
                                   proto_tree *context_tree _U_)
{
    gboolean ces_seen = FALSE;
    gboolean data_seen = FALSE;

    /************************************************************************/
    /* Dissect each sub-pdu.                                             */
    do {
        /* Subheader */
        proto_item *subheader_ti = proto_tree_add_item(tree,
                                                       hf_mac_nr_subheader,
                                                       tvb, offset, 0, ENC_ASCII|ENC_NA);
        proto_tree *subheader_tree = proto_item_add_subtree(subheader_ti, ett_mac_nr_subheader);


        gboolean F, fixed_len;
        guint32 SDU_length=0;

        /* 1st bit is always reserved */
        /* 2nd bit depends upon LCID */
        guint8 lcid = tvb_get_guint8(tvb, offset) & 0x3f;
        fixed_len = is_fixed_sized_lcid(lcid, p_mac_nr_info->direction);
        if (fixed_len) {
            proto_tree_add_bits_item(subheader_tree, hf_mac_nr_subheader_reserved, tvb, offset<<3, 2, ENC_BIG_ENDIAN);
        }
        else {
            proto_tree_add_bits_item(subheader_tree, hf_mac_nr_subheader_reserved, tvb, offset<<3, 1, ENC_BIG_ENDIAN);
            /* Data, so check F bit and length */
            proto_tree_add_item_ret_boolean(subheader_tree, hf_mac_nr_subheader_f, tvb, offset, 1, ENC_BIG_ENDIAN, &F);
        }

        /* LCID */
        proto_tree_add_uint(subheader_tree,
                            (p_mac_nr_info->direction == DIRECTION_UPLINK) ?
                                  hf_mac_nr_ulsch_lcid : hf_mac_nr_dlsch_lcid,
                            tvb, offset, 1, lcid);
        offset++;

        if (!fixed_len) {
            if (F) {
                /* Long length */
                proto_tree_add_item_ret_uint(subheader_tree, hf_mac_nr_subheader_length_2_bytes, tvb, offset, 2, ENC_BIG_ENDIAN, &SDU_length);
                offset += 2;
            }
            else {
                /* Short length */
                proto_tree_add_item_ret_uint(subheader_tree, hf_mac_nr_subheader_length_1_byte, tvb, offset, 1, ENC_BIG_ENDIAN, &SDU_length);
                offset++;
            }
        }

        if (lcid <= 32 || (p_mac_nr_info->direction == DIRECTION_UPLINK && lcid == CCCH_48_BITS_LCID)) {
            proto_item *sch_pdu_ti;

            /* Note whether this sub-pdu gets dissected by RLC/RRC */
            gboolean dissected_by_upper_layer = FALSE;

            /* Add SDU, for now just as hex data */
            if (p_mac_nr_info->direction == DIRECTION_UPLINK) {
                if (lcid == CCCH_LCID) {
                    SDU_length = 8;
                } else if (lcid == CCCH_48_BITS_LCID) {
                    SDU_length = 6;
                }
                sch_pdu_ti = proto_tree_add_item(subheader_tree, hf_mac_nr_ulsch_sdu,
                                                 tvb, offset, SDU_length, ENC_NA);
            }
            else {
                sch_pdu_ti = proto_tree_add_item(subheader_tree, hf_mac_nr_dlsch_sdu,
                                                 tvb, offset, SDU_length, ENC_NA);
            }

            /* Call RLC if configured to do so for this SDU */
            if ((lcid >= 3) && (lcid <= 32)) {
                /* Look for mapping for this LCID to drb channel set by UAT table */
                rlc_bearer_type_t rlc_bearer_type;
                guint8 seqnum_length;
                gint drb_id;

                // TODO: priority not set.
                guint8 priority = 0;
                lookup_rlc_channel_from_lcid(p_mac_nr_info->ueid,
                                             lcid,
                                             p_mac_nr_info->direction,
                                             &rlc_bearer_type,
                                             &seqnum_length,
                                             &drb_id);

                /* Dissect according to channel type */
                switch (rlc_bearer_type) {
                    case rlcUM6:
                    case rlcUM12:
                        call_rlc_dissector(tvb, pinfo, tree, pdu_ti, offset, SDU_length,
                                           RLC_UM_MODE, p_mac_nr_info->direction, p_mac_nr_info->ueid,
                                           BEARER_TYPE_DRB, drb_id, seqnum_length,
                                           priority);
                        dissected_by_upper_layer = TRUE;
                        break;
                    case rlcAM12:
                    case rlcAM18:
                        call_rlc_dissector(tvb, pinfo, tree, pdu_ti, offset, SDU_length,
                                           RLC_AM_MODE, p_mac_nr_info->direction, p_mac_nr_info->ueid,
                                           BEARER_TYPE_DRB, drb_id, seqnum_length,
                                           priority);
                        dissected_by_upper_layer = TRUE;
                        break;
                    case rlcTM:
                        call_rlc_dissector(tvb, pinfo, tree, pdu_ti, offset, SDU_length,
                                           RLC_TM_MODE, p_mac_nr_info->direction, p_mac_nr_info->ueid,
                                           BEARER_TYPE_DRB, drb_id, 0,
                                           priority);
                        dissected_by_upper_layer = TRUE;
                        break;
                    case rlcRaw:
                        /* Nothing to do! */
                        break;
                }
            } else if (lcid == 1 || lcid == 2) {
                /* SRB, TODO: call RLC dissector */
            } else if (global_mac_nr_attempt_rrc_decode) {
                dissector_handle_t protocol_handle;
                tvbuff_t *rrc_tvb = tvb_new_subset_remaining(tvb, offset);

                if (p_mac_nr_info->direction == DIRECTION_UPLINK) {
                    protocol_handle = (lcid == CCCH_LCID) ? nr_rrc_ul_ccch1_handle : nr_rrc_ul_ccch_handle;
                } else {
                    protocol_handle = nr_rrc_dl_ccch_handle;
                }
                /* Hide raw view of bytes */
                PROTO_ITEM_SET_HIDDEN(sch_pdu_ti);
                call_with_catch_all(protocol_handle, rrc_tvb, pinfo, tree);
                dissected_by_upper_layer = TRUE;
            }

            /* Only write summary to Info column if didn't send to upper_layer dissector */
            write_pdu_label_and_info(pdu_ti, subheader_ti, dissected_by_upper_layer ? NULL : pinfo,
                                     "(LCID:%u %u bytes) ", lcid, SDU_length);

            offset += SDU_length;


            if (p_mac_nr_info->direction == DIRECTION_UPLINK) {
                if (ces_seen) {
                    expert_add_info_format(pinfo, subheader_ti, &ei_mac_nr_ul_sch_control_subheader_before_data_subheader,
                                           "UL-SCH: should not have Data SDUs after Control Elements");
                }
            }
            data_seen = TRUE;
        }
        else {
            /* Control Elements */

            /* Add some space to info column between entries */
            if (data_seen || ces_seen) {
                col_append_str(pinfo->cinfo, COL_INFO, "  ");
            }

            if (lcid != PADDING_LCID) {
                ces_seen = TRUE;
            }

            if (p_mac_nr_info->direction == DIRECTION_UPLINK) {
                guint32 phr_ph, phr_pcmax_f_c, c_rnti, lcg_id, bs, br_lcid, bit_rate;
                gboolean dir;

                switch (lcid) {
                    case RECOMMENDED_BIT_RATE_QUERY_LCID:
                        proto_tree_add_item_ret_uint(subheader_tree, hf_mac_nr_control_recommended_bit_rate_query_lcid,
                                            tvb, offset, 1, ENC_BIG_ENDIAN, &br_lcid);
                        proto_tree_add_item_ret_boolean(subheader_tree, hf_mac_nr_control_recommended_bit_rate_query_dir,
                                                        tvb, offset, 1, ENC_BIG_ENDIAN, &dir);
                        proto_tree_add_item_ret_uint(subheader_tree, hf_mac_nr_control_recommended_bit_rate_query_bit_rate,
                                                     tvb, offset, 2, ENC_BIG_ENDIAN, &bit_rate);
                        proto_tree_add_item(subheader_tree, hf_mac_nr_control_recommended_bit_rate_query_reserved,
                                            tvb, offset+1, 1, ENC_BIG_ENDIAN);
                        write_pdu_label_and_info(pdu_ti, subheader_ti, pinfo,
                                                 "(Recommended BR Query LCID=%u Dir=%s BR=%s) ", br_lcid, dir ? "UL" : "DL",
                                                 val_to_str_ext_const(bit_rate, &bit_rate_vals_ext, "Unknown"));
                        offset += 2;
                        break;
                    case CONFIGURED_GRANT_CONFIGURATION_LCID:
                        /* Fixed size of zero bits */
                        write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo,
                                                         "(Configured Grant Config) ");
                        break;
                    case MULTIPLE_ENTRY_PHR_1_LCID:
                    case MULTIPLE_ENTRY_PHR_4_LCID:
                    {
                        static const int * me_phr_byte1_flags[] = {
                            &hf_mac_nr_control_me_phr_c7_flag,
                            &hf_mac_nr_control_me_phr_c6_flag,
                            &hf_mac_nr_control_me_phr_c5_flag,
                            &hf_mac_nr_control_me_phr_c4_flag,
                            &hf_mac_nr_control_me_phr_c3_flag,
                            &hf_mac_nr_control_me_phr_c2_flag,
                            &hf_mac_nr_control_me_phr_c1_flag,
                            &hf_mac_nr_control_me_phr_reserved,
                            NULL
                        };
                        static const int * me_phr_byte2_flags[] = {
                            &hf_mac_nr_control_me_phr_c15_flag,
                            &hf_mac_nr_control_me_phr_c14_flag,
                            &hf_mac_nr_control_me_phr_c13_flag,
                            &hf_mac_nr_control_me_phr_c12_flag,
                            &hf_mac_nr_control_me_phr_c11_flag,
                            &hf_mac_nr_control_me_phr_c10_flag,
                            &hf_mac_nr_control_me_phr_c9_flag,
                            &hf_mac_nr_control_me_phr_c8_flag,
                            NULL
                        };
                        static const int * me_phr_byte3_flags[] = {
                            &hf_mac_nr_control_me_phr_c23_flag,
                            &hf_mac_nr_control_me_phr_c22_flag,
                            &hf_mac_nr_control_me_phr_c21_flag,
                            &hf_mac_nr_control_me_phr_c20_flag,
                            &hf_mac_nr_control_me_phr_c19_flag,
                            &hf_mac_nr_control_me_phr_c18_flag,
                            &hf_mac_nr_control_me_phr_c17_flag,
                            &hf_mac_nr_control_me_phr_c16_flag,
                            NULL
                        };
                        static const int * me_phr_byte4_flags[] = {
                            &hf_mac_nr_control_me_phr_c31_flag,
                            &hf_mac_nr_control_me_phr_c30_flag,
                            &hf_mac_nr_control_me_phr_c29_flag,
                            &hf_mac_nr_control_me_phr_c28_flag,
                            &hf_mac_nr_control_me_phr_c27_flag,
                            &hf_mac_nr_control_me_phr_c26_flag,
                            &hf_mac_nr_control_me_phr_c25_flag,
                            &hf_mac_nr_control_me_phr_c24_flag,
                            NULL
                        };
                        guint32 start_offset = offset;
                        guint8 scell_bitmap1;
                        guint32 scell_bitmap2_3_4;
                        proto_tree_add_bitmask_list(subheader_tree, tvb, offset, 1, me_phr_byte1_flags, ENC_NA);
                        scell_bitmap1 = tvb_get_guint8(tvb, offset);
                        offset++;
                        if (lcid == MULTIPLE_ENTRY_PHR_4_LCID) {
                            proto_tree_add_bitmask_list(subheader_tree, tvb, offset, 1, me_phr_byte2_flags, ENC_NA);
                            proto_tree_add_bitmask_list(subheader_tree, tvb, offset+1, 1, me_phr_byte3_flags, ENC_NA);
                            proto_tree_add_bitmask_list(subheader_tree, tvb, offset+2, 1, me_phr_byte4_flags, ENC_NA);
                            scell_bitmap2_3_4 = tvb_get_letoh24(tvb, offset); /* read them in little endian on purpose */
                            offset += 3;
                        }

                        static const int *ph_fields1[] = {
                            &hf_mac_nr_control_me_phr_ph_c1,
                            &hf_mac_nr_control_me_phr_ph_c2,
                            &hf_mac_nr_control_me_phr_ph_c3,
                            &hf_mac_nr_control_me_phr_ph_c4,
                            &hf_mac_nr_control_me_phr_ph_c5,
                            &hf_mac_nr_control_me_phr_ph_c6,
                            &hf_mac_nr_control_me_phr_ph_c7,
                        };
                        static const int *ph_fields2_3_4[] = {
                            &hf_mac_nr_control_me_phr_ph_c8,
                            &hf_mac_nr_control_me_phr_ph_c9,
                            &hf_mac_nr_control_me_phr_ph_c10,
                            &hf_mac_nr_control_me_phr_ph_c11,
                            &hf_mac_nr_control_me_phr_ph_c12,
                            &hf_mac_nr_control_me_phr_ph_c13,
                            &hf_mac_nr_control_me_phr_ph_c14,
                            &hf_mac_nr_control_me_phr_ph_c15,
                            &hf_mac_nr_control_me_phr_ph_c16,
                            &hf_mac_nr_control_me_phr_ph_c17,
                            &hf_mac_nr_control_me_phr_ph_c18,
                            &hf_mac_nr_control_me_phr_ph_c19,
                            &hf_mac_nr_control_me_phr_ph_c20,
                            &hf_mac_nr_control_me_phr_ph_c21,
                            &hf_mac_nr_control_me_phr_ph_c22,
                            &hf_mac_nr_control_me_phr_ph_c23,
                            &hf_mac_nr_control_me_phr_ph_c24,
                            &hf_mac_nr_control_me_phr_ph_c25,
                            &hf_mac_nr_control_me_phr_ph_c26,
                            &hf_mac_nr_control_me_phr_ph_c27,
                            &hf_mac_nr_control_me_phr_ph_c28,
                            &hf_mac_nr_control_me_phr_ph_c29,
                            &hf_mac_nr_control_me_phr_ph_c30,
                            &hf_mac_nr_control_me_phr_ph_c31,
                        };

                        /* PCell entries */
                        guint32 PH;
                        proto_item *entry_ti;
                        if (p_mac_nr_info->phr_type2_othercell) {
                            /* The PH and PCMAX,f,c fields can be either for a LTE or NR cell */
                            entry_ti = dissect_me_phr_ph(tvb, pinfo, subheader_ti, &hf_mac_nr_control_me_phr_ph_type2_spcell,
                                                         &hf_mac_nr_control_me_phr_pcmax_f_c_type2_spcell, &PH, &offset);
                            proto_item_append_text(entry_ti, " (Type2, SpCell PH=%u)", PH);
                        }
                        entry_ti = dissect_me_phr_ph(tvb, pinfo, subheader_ti, &hf_mac_nr_control_me_phr_ph_type1_pcell,
                                                     &hf_mac_nr_control_me_phr_pcmax_f_c_type1_pcell, &PH, &offset);
                        proto_item_append_text(entry_ti, " (Type1, PCell PH=%u)", PH);


                        /* SCell entries */
                        /* The PH and PCMAX,f,c fields can be either for a LTE or NR cell */
                        for (int n=1; n <= 7; n++) {
                            if (scell_bitmap1 & (1 << n)) {
                                entry_ti = dissect_me_phr_ph(tvb, pinfo, subheader_ti, ph_fields1[n-1],
                                                             &hf_mac_nr_control_me_phr_pcmax_f_c_typeX, &PH, &offset);
                                proto_item_append_text(entry_ti, " (SCellIndex %d PH=%u)", n, PH);
                            }
                        }
                        if (lcid == MULTIPLE_ENTRY_PHR_4_LCID) {
                            for (int n=0; n <= 23; n++) {
                                if (scell_bitmap2_3_4 & (1 << n)) {
                                    entry_ti = dissect_me_phr_ph(tvb, pinfo, subheader_ti, ph_fields2_3_4[n],
                                                                 &hf_mac_nr_control_me_phr_pcmax_f_c_typeX, &PH, &offset);
                                    proto_item_append_text(entry_ti, " (SCellIndex %d PH=%u)", n+8, PH);
                                }
                            }
                        }

                        write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo,
                                                         "(Multi-entry PHR) ");

                        /* Make sure dissected length matches signalled length */
                        if (offset != start_offset + SDU_length) {
                            proto_tree_add_expert_format(subheader_tree, pinfo, &ei_mac_nr_sdu_length_different_from_dissected,
                                                         tvb, start_offset, offset-start_offset,
                                                         "A Multiple-Entry PHR subheader has a length field of %u bytes, but "
                                                         "dissected %u bytes", SDU_length, offset-start_offset);
                            /* Assume length was correct, so at least can dissect further subheaders */
                            offset = start_offset + SDU_length;
                        }
                        break;
                    }
                    case SINGLE_ENTRY_PHR_LCID:
                        /* R R PH (6 bits) */
                        proto_tree_add_item(subheader_tree, hf_mac_nr_control_se_phr_reserved,
                                            tvb, offset, 1, ENC_NA);
                        proto_tree_add_item_ret_uint(subheader_tree, hf_mac_nr_control_se_phr_ph,
                                                     tvb, offset, 1, ENC_BIG_ENDIAN, &phr_ph);
                        offset++;

                        /* R R PCMAX_f_c (6 bits) */
                        proto_tree_add_item(subheader_tree, hf_mac_nr_control_se_phr_reserved,
                                            tvb, offset, 1, ENC_NA);
                        proto_tree_add_item_ret_uint(subheader_tree, hf_mac_nr_control_se_phr_pcmax_f_c,
                                                     tvb, offset, 1, ENC_NA, &phr_pcmax_f_c);
                        offset++;
                        write_pdu_label_and_info(pdu_ti, subheader_ti, pinfo,
                                                 "(PHR PH=%u PCMAX_f_c=%u) ", phr_ph, phr_pcmax_f_c);
                        break;
                    case C_RNTI_LCID:
                        proto_tree_add_item_ret_uint(subheader_tree, hf_mac_nr_control_crnti,
                                                     tvb, offset, 2, ENC_BIG_ENDIAN, &c_rnti);
                        write_pdu_label_and_info(pdu_ti, subheader_ti, pinfo,
                                                 "(C-RNTI=%u) ", c_rnti);
                        offset += 2;
                        break;
                    case SHORT_TRUNCATED_BSR_LCID:
                    case SHORT_BSR_LCID:
                        {
                            static const int *hf_mac_nr_control_bsr_short_bs_lcg[] = {
                                &hf_mac_nr_control_bsr_short_bs_lcg0,
                                &hf_mac_nr_control_bsr_short_bs_lcg1,
                                &hf_mac_nr_control_bsr_short_bs_lcg2,
                                &hf_mac_nr_control_bsr_short_bs_lcg3,
                                &hf_mac_nr_control_bsr_short_bs_lcg4,
                                &hf_mac_nr_control_bsr_short_bs_lcg5,
                                &hf_mac_nr_control_bsr_short_bs_lcg6,
                                &hf_mac_nr_control_bsr_short_bs_lcg7
                            };

                            proto_tree_add_item_ret_uint(subheader_tree, hf_mac_nr_control_bsr_short_lcg,
                                                         tvb, offset, 1, ENC_BIG_ENDIAN, &lcg_id);
                            proto_tree_add_item_ret_uint(subheader_tree, *hf_mac_nr_control_bsr_short_bs_lcg[lcg_id],
                                                         tvb, offset, 1, ENC_BIG_ENDIAN, &bs);
                            write_pdu_label_and_info(pdu_ti, subheader_ti, pinfo,
                                                     "(Short %sBSR LCG ID=%u BS=%u) ",
                                                     lcid == SHORT_BSR_LCID ? "" : "Truncated ", lcg_id, bs);
                            offset++;
                        }
                        break;
                    case LONG_TRUNCATED_BSR_LCID:
                        {
                            static const int * long_bsr_flags[] = {
                                &hf_mac_nr_control_bsr_long_lcg7,
                                &hf_mac_nr_control_bsr_long_lcg6,
                                &hf_mac_nr_control_bsr_long_lcg5,
                                &hf_mac_nr_control_bsr_long_lcg4,
                                &hf_mac_nr_control_bsr_long_lcg3,
                                &hf_mac_nr_control_bsr_long_lcg2,
                                &hf_mac_nr_control_bsr_long_lcg1,
                                &hf_mac_nr_control_bsr_long_lcg0,
                                NULL
                            };

                            proto_tree_add_bitmask_list(subheader_tree, tvb, offset, 1, long_bsr_flags, ENC_NA);
                            guint CE_start = offset;
                            offset++;

                            while ((offset-CE_start) < SDU_length) proto_tree_add_item(subheader_tree, hf_mac_nr_control_bsr_trunc_long_bs, tvb, offset++, 1, ENC_NA);

                            /* TODO: show in string here how many BSs were seen */
                            write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo,
                                                             "(Long Truncated BSR) ");

                            if (SDU_length > 7) {
                                proto_tree_add_expert_format(subheader_tree, pinfo, &ei_mac_nr_sdu_length_different_from_dissected,
                                                             tvb, CE_start, SDU_length,
                                                             "A Long Truncated BSR subheader should have a length field up to 7 bytes, but "
                                                             "is set to %u bytes", SDU_length);
                            }
                        }
                        break;
                    case LONG_BSR_LCID:
                        {
                            static const int * long_bsr_flags[] = {
                                &hf_mac_nr_control_bsr_long_lcg7,
                                &hf_mac_nr_control_bsr_long_lcg6,
                                &hf_mac_nr_control_bsr_long_lcg5,
                                &hf_mac_nr_control_bsr_long_lcg4,
                                &hf_mac_nr_control_bsr_long_lcg3,
                                &hf_mac_nr_control_bsr_long_lcg2,
                                &hf_mac_nr_control_bsr_long_lcg1,
                                &hf_mac_nr_control_bsr_long_lcg0,
                                NULL
                            };

                            guint8 flags = tvb_get_guint8(tvb, offset);
                            proto_tree_add_bitmask_list(subheader_tree, tvb, offset, 1, long_bsr_flags, ENC_NA);
                            guint CE_start = offset;
                            offset++;

                            /* Show BSR values. */
                            if (flags & 0x01) proto_tree_add_item(subheader_tree, hf_mac_nr_control_bsr_long_bs_lcg0, tvb, offset++, 1, ENC_NA);
                            if (flags & 0x02) proto_tree_add_item(subheader_tree, hf_mac_nr_control_bsr_long_bs_lcg1, tvb, offset++, 1, ENC_NA);
                            if (flags & 0x04) proto_tree_add_item(subheader_tree, hf_mac_nr_control_bsr_long_bs_lcg2, tvb, offset++, 1, ENC_NA);
                            if (flags & 0x08) proto_tree_add_item(subheader_tree, hf_mac_nr_control_bsr_long_bs_lcg3, tvb, offset++, 1, ENC_NA);
                            if (flags & 0x10) proto_tree_add_item(subheader_tree, hf_mac_nr_control_bsr_long_bs_lcg4, tvb, offset++, 1, ENC_NA);
                            if (flags & 0x20) proto_tree_add_item(subheader_tree, hf_mac_nr_control_bsr_long_bs_lcg5, tvb, offset++, 1, ENC_NA);
                            if (flags & 0x40) proto_tree_add_item(subheader_tree, hf_mac_nr_control_bsr_long_bs_lcg6, tvb, offset++, 1, ENC_NA);
                            if (flags & 0x80) proto_tree_add_item(subheader_tree, hf_mac_nr_control_bsr_long_bs_lcg7, tvb, offset++, 1, ENC_NA);

                            /* TODO: show in string here how many BSs were seen */
                            write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo,
                                                             "(Long BSR) ");


                            /* Make sure dissected length matches signalled length */
                            if ((offset - CE_start) != SDU_length) {
                                proto_tree_add_expert_format(subheader_tree, pinfo, &ei_mac_nr_sdu_length_different_from_dissected,
                                                             tvb, CE_start, offset-CE_start,
                                                             "A Long BSR subheader has a length field of %u bytes, but "
                                                             "dissected %u bytes", SDU_length, offset-CE_start);
                                /* Assume length was correct, so at least can dissect further subheaders */
                                offset = CE_start + SDU_length;
                            }
                        }
                        break;
                    case PADDING_LCID:
                        {
                            /* The rest of the PDU is padding */
                            int pad_len = tvb_reported_length_remaining(tvb, offset);
                            if (pad_len > 0)
                                proto_tree_add_item(subheader_tree, hf_mac_nr_padding, tvb, offset, -1, ENC_NA);
                            write_pdu_label_and_info(pdu_ti, subheader_ti, pinfo, "(Padding %u bytes) ", pad_len);
                            /* Move to the end of the frame */
                            offset = tvb_reported_length(tvb);
                        }
                        break;
                }
            }
            else {
                /* Downlink control elements */
                guint32 ta_tag_id, ta_ta, br_lcid, bit_rate;
                gboolean dir;

                if (lcid != PADDING_LCID) {
                    if (data_seen) {
                        expert_add_info_format(pinfo, subheader_ti, &ei_mac_nr_dl_sch_control_subheader_after_data_subheader,
                                               "DL-SCH: should not have Control Elements after Data SDUs");
                    }
                }

                switch (lcid) {
                    case RECOMMENDED_BIT_RATE_LCID:
                        proto_tree_add_item_ret_uint(subheader_tree, hf_mac_nr_control_recommended_bit_rate_lcid,
                                            tvb, offset, 1, ENC_BIG_ENDIAN, &br_lcid);
                        proto_tree_add_item_ret_boolean(subheader_tree, hf_mac_nr_control_recommended_bit_rate_dir,
                                                        tvb, offset, 1, ENC_BIG_ENDIAN, &dir);
                        proto_tree_add_item_ret_uint(subheader_tree, hf_mac_nr_control_recommended_bit_rate_bit_rate,
                                                     tvb, offset, 2, ENC_BIG_ENDIAN, &bit_rate);
                        proto_tree_add_item(subheader_tree, hf_mac_nr_control_recommended_bit_rate_reserved,
                                            tvb, offset+1, 1, ENC_BIG_ENDIAN);
                        offset += 2;
                        write_pdu_label_and_info(pdu_ti, subheader_ti, pinfo,
                                                 "(Recommended BR LCID=%u Dir=%s BR=%s) ", br_lcid, dir ? "UL" : "DL",
                                                 val_to_str_ext_const(bit_rate, &bit_rate_vals_ext, "Unknown"));
                        break;
                    case SP_ZP_CSI_RS_RESOURCE_SET_ACT_DEACT_LCID:
                        proto_tree_add_item(subheader_tree, hf_mac_control_sp_zp_csi_rs_resource_set_act_deact_ad,
                                            tvb, offset, 1, ENC_NA);
                        proto_tree_add_item(subheader_tree, hf_mac_control_sp_zp_csi_rs_resource_set_act_deact_serving_cell_id,
                                            tvb, offset, 1, ENC_NA);
                        proto_tree_add_item(subheader_tree, hf_mac_control_sp_zp_csi_rs_resource_set_act_deact_bwp_id,
                                            tvb, offset, 1, ENC_NA);
                        offset++;
                        proto_tree_add_item(subheader_tree, hf_mac_control_sp_zp_csi_rs_resource_set_act_deact_reserved_2,
                                            tvb, offset, 1, ENC_NA);
                        proto_tree_add_item(subheader_tree, hf_mac_control_sp_zp_csi_rs_resource_set_act_deact_sp_zp_rs_resource_set_id,
                                            tvb, offset, 1, ENC_NA);
                        offset++;
                        write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo,
                                                         "(SP ZP CSI-RS Res Set Act/Deact) ");
                        break;
                    case PUCCH_SPATIAL_REL_ACT_DEACT_LCID:
                        {
                            static const int * pucch_spatial_rel_act_deact_flags[] = {
                                &hf_mac_nr_control_pucch_spatial_rel_act_deact_s7,
                                &hf_mac_nr_control_pucch_spatial_rel_act_deact_s6,
                                &hf_mac_nr_control_pucch_spatial_rel_act_deact_s5,
                                &hf_mac_nr_control_pucch_spatial_rel_act_deact_s4,
                                &hf_mac_nr_control_pucch_spatial_rel_act_deact_s3,
                                &hf_mac_nr_control_pucch_spatial_rel_act_deact_s2,
                                &hf_mac_nr_control_pucch_spatial_rel_act_deact_s1,
                                &hf_mac_nr_control_pucch_spatial_rel_act_deact_s0,
                                NULL
                            };
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_pucch_spatial_rel_act_deact_reserved,
                                                tvb, offset, 1, ENC_NA);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_pucch_spatial_rel_act_deact_serving_cell_id,
                                                tvb, offset, 1, ENC_NA);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_pucch_spatial_rel_act_deact_bwp_id,
                                                tvb, offset, 1, ENC_NA);
                            offset++;
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_pucch_spatial_rel_act_deact_reserved,
                                                tvb, offset, 1, ENC_NA);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_pucch_spatial_rel_act_deact_pucch_resource_id,
                                                tvb, offset, 1, ENC_NA);
                            offset++;
                            proto_tree_add_bitmask_list(subheader_tree, tvb, offset, 1, pucch_spatial_rel_act_deact_flags, ENC_NA);
                            offset++;
                            write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo,
                                                             "(PUCCH Spatial Rel Act/Deact) ");
                        }
                        break;
                    case SP_SRS_ACT_DEACT_LCID:
                        {
                            gboolean ad, c;
                            guint32 start_offset = offset;
                            proto_tree_add_item_ret_boolean(subheader_tree, hf_mac_nr_control_sp_srs_act_deact_ad,
                                                            tvb, offset, 1, ENC_NA, &ad);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_srs_act_deact_srs_resource_set_cell_id,
                                                tvb, offset, 1, ENC_NA);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_srs_act_deact_srs_resource_set_bwp_id,
                                                tvb, offset, 1, ENC_NA);
                            offset++;
                            proto_tree_add_bits_item(subheader_tree, hf_mac_nr_control_sp_srs_act_deact_reserved,
                                                     tvb, offset<<3, 2, ENC_NA);
                            proto_tree_add_item_ret_boolean(subheader_tree, hf_mac_nr_control_sp_srs_act_deact_c,
                                                            tvb, offset, 1, ENC_NA, &c);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_srs_act_deact_sul,
                                                tvb, offset, 1, ENC_NA);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_srs_act_deact_sp_srs_resource_set_id,
                                                tvb, offset, 1, ENC_NA);
                            offset++;
                            if (ad) {
                                guint length = c ? (SDU_length-2) / 2 + 2: SDU_length;
                                while (offset - start_offset < length) {
                                    proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_srs_act_deact_f,
                                                        tvb, offset, 1, ENC_NA);
                                    proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_srs_act_deact_resource_id,
                                                        tvb, offset, 1, ENC_NA);
                                    offset++;
                                }

                            }
                            if (c) {
                                while (offset - start_offset < SDU_length) {
                                    proto_tree_add_bits_item(subheader_tree, hf_mac_nr_control_sp_srs_act_deact_reserved,
                                                             tvb, offset<<3, 1, ENC_NA);
                                    proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_srs_act_deact_resource_serving_cell_id,
                                                        tvb, offset, 1, ENC_NA);
                                    proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_srs_act_deact_resource_bwp_id,
                                                        tvb, offset, 1, ENC_NA);
                                    offset++;
                                }
                            }
                            write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo,
                                                             "(SP SRS Act/Deact) ");
                        }
                        break;
                    case SP_CSI_REPORT_ON_PUCCH_ACT_DEACT_LCID:
                        {
                            static const int * sp_csi_report_on_pucch_act_deact_flags[] = {
                                &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s7,
                                &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s6,
                                &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s5,
                                &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s4,
                                &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s3,
                                &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s2,
                                &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s1,
                                &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s0,
                                NULL
                            };
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_reserved,
                                                tvb, offset, 1, ENC_NA);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_serving_cell_id,
                                                tvb, offset, 1, ENC_NA);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_bwp_id,
                                                tvb, offset, 1, ENC_NA);
                            offset++;
                            proto_tree_add_bitmask_list(subheader_tree, tvb, offset, 1, sp_csi_report_on_pucch_act_deact_flags, ENC_NA);
                            offset++;
                            write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo,
                                                             "(SP CSI Report on PUCCH Act/Deact) ");
                        }
                        break;
                    case TCI_STATE_IND_FOR_UE_SPEC_PDCCH_LCID:
                        proto_tree_add_item(subheader_tree, hf_mac_nr_control_tci_state_ind_for_ue_spec_pdcch_serving_cell_id,
                                            tvb, offset, 1, ENC_NA);
                        proto_tree_add_item(subheader_tree, hf_mac_nr_control_tci_state_ind_for_ue_spec_pdcch_coreset_id,
                                            tvb, offset, 2, ENC_NA);
                        offset++;
                        proto_tree_add_item(subheader_tree, hf_mac_nr_control_tci_state_ind_for_ue_spec_pdcch_tci_state_id,
                                            tvb, offset, 1, ENC_NA);
                        offset++;
                        write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo,
                                                             "(TCI State Ind PDCCH) ");
                        break;
                    case TCI_STATES_ACT_DEACT_FOR_UE_SPEC_PDSCH_LCID:
                        {
                            guint32 start_offset = offset;
                            static const int * tci_states_act_deact_for_ue_spec_pdsc_flags[] = {
                                &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t7,
                                &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t6,
                                &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t5,
                                &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t4,
                                &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t3,
                                &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t2,
                                &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t1,
                                &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t0,
                                NULL
                            };
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_reserved,
                                                tvb, offset, 1, ENC_NA);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_serving_cell_id,
                                                tvb, offset, 1, ENC_NA);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_bwp_id,
                                                tvb, offset, 1, ENC_NA);
                            offset++;
                            while (offset - start_offset < SDU_length) {
                                proto_tree_add_bitmask_list(subheader_tree, tvb, offset, 1, tci_states_act_deact_for_ue_spec_pdsc_flags, ENC_NA);
                                offset++;
                            }
                            write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo,
                                                             "(TCI States Act Deact PDSCH) ");
                        }
                        break;
                    case APER_CSI_TRIGGER_STATE_SUBSELECT_LCID:
                        {
                            guint32 start_offset = offset;
                            static const int * aper_csi_trigger_state_subselect_flags[] = {
                                &hf_mac_nr_control_aper_csi_trigger_state_subselect_t7,
                                &hf_mac_nr_control_aper_csi_trigger_state_subselect_t6,
                                &hf_mac_nr_control_aper_csi_trigger_state_subselect_t5,
                                &hf_mac_nr_control_aper_csi_trigger_state_subselect_t4,
                                &hf_mac_nr_control_aper_csi_trigger_state_subselect_t3,
                                &hf_mac_nr_control_aper_csi_trigger_state_subselect_t2,
                                &hf_mac_nr_control_aper_csi_trigger_state_subselect_t1,
                                &hf_mac_nr_control_aper_csi_trigger_state_subselect_t0,
                                NULL
                            };
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_aper_csi_trigger_state_subselect_reserved,
                                                tvb, offset, 1, ENC_NA);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_aper_csi_trigger_state_subselect_serving_cell_id,
                                                tvb, offset, 1, ENC_NA);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_aper_csi_trigger_state_subselect_bwp_id,
                                                tvb, offset, 1, ENC_NA);
                            offset++;
                            while (offset - start_offset < SDU_length) {
                                proto_tree_add_bitmask_list(subheader_tree, tvb, offset, 1, aper_csi_trigger_state_subselect_flags, ENC_NA);
                                offset++;
                            }
                            write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo,
                                                             "(Aperiodic CSI Trigger State Subselection) ");
                        }
                        break;
                    case SP_CSI_RS_CSI_IM_RES_SET_ACT_DEACT_LCID:
                        {
                            gboolean ad;
                            guint32 start_offset = offset;
                            static const int * sp_csi_rs_csi_im_res_set_act_deact_flags[] = {
                                &hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_reserved3,
                                &hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_tci_state_id,
                                NULL
                            };
                            proto_tree_add_item_ret_boolean(subheader_tree, hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_ad,
                                                            tvb, offset, 1, ENC_NA, &ad);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_serving_cell_id,
                                                tvb, offset, 1, ENC_NA);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_bwp_id,
                                                tvb, offset, 1, ENC_NA);
                            offset++;
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_reserved,
                                            tvb, offset, 1, ENC_NA);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_im,
                                                tvb, offset, 1, ENC_NA);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_sp_csi_rs_res_set_id,
                                                tvb, offset, 1, ENC_NA);
                            offset++;
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_reserved2,
                                            tvb, offset, 1, ENC_NA);
                            proto_tree_add_item(subheader_tree, hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_sp_csi_im_res_set_id,
                                                tvb, offset, 1, ENC_NA);
                            offset++;
                            if (ad) {
                                while (offset - start_offset < SDU_length) {
                                    proto_tree_add_bitmask_list(subheader_tree, tvb, offset, 1, sp_csi_rs_csi_im_res_set_act_deact_flags, ENC_NA);
                                    offset++;
                                }
                            }
                            write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo,
                                                             "(SP CSI-RS/CSI-IM Res Set Act/Deact) ");
                        }
                        break;
                    case DUPLICATION_ACTIVATION_DEACTIVATION_LCID:
                        {
                            static const int * dupl_act_deact_flags[] = {
                                &hf_mac_nr_control_dupl_act_deact_drb7,
                                &hf_mac_nr_control_dupl_act_deact_drb6,
                                &hf_mac_nr_control_dupl_act_deact_drb5,
                                &hf_mac_nr_control_dupl_act_deact_drb4,
                                &hf_mac_nr_control_dupl_act_deact_drb3,
                                &hf_mac_nr_control_dupl_act_deact_drb2,
                                &hf_mac_nr_control_dupl_act_deact_drb1,
                                &hf_mac_nr_control_dupl_act_deact_reserved,
                                NULL
                            };
                            proto_tree_add_bitmask_list(subheader_tree, tvb, offset, 1, dupl_act_deact_flags, ENC_NA);
                            offset++;
                            write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo,
                                                             "(Dupl Act/Deact) ");
                        }
                        break;
                    case SCELL_ACTIVATION_DEACTIVATION_4_LCID:
                        {
                            static const int * scell_act_deact_1_flags[] = {
                                &hf_mac_nr_control_scell_act_deact_cell7,
                                &hf_mac_nr_control_scell_act_deact_cell6,
                                &hf_mac_nr_control_scell_act_deact_cell5,
                                &hf_mac_nr_control_scell_act_deact_cell4,
                                &hf_mac_nr_control_scell_act_deact_cell3,
                                &hf_mac_nr_control_scell_act_deact_cell2,
                                &hf_mac_nr_control_scell_act_deact_cell1,
                                &hf_mac_nr_control_scell_act_deact_reserved,
                                NULL
                            };
                            static const int * scell_act_deact_2_flags[] = {
                                &hf_mac_nr_control_scell_act_deact_cell15,
                                &hf_mac_nr_control_scell_act_deact_cell14,
                                &hf_mac_nr_control_scell_act_deact_cell13,
                                &hf_mac_nr_control_scell_act_deact_cell12,
                                &hf_mac_nr_control_scell_act_deact_cell11,
                                &hf_mac_nr_control_scell_act_deact_cell10,
                                &hf_mac_nr_control_scell_act_deact_cell9,
                                &hf_mac_nr_control_scell_act_deact_cell8,
                                NULL
                            };
                            static const int * scell_act_deact_3_flags[] = {
                                &hf_mac_nr_control_scell_act_deact_cell23,
                                &hf_mac_nr_control_scell_act_deact_cell22,
                                &hf_mac_nr_control_scell_act_deact_cell21,
                                &hf_mac_nr_control_scell_act_deact_cell20,
                                &hf_mac_nr_control_scell_act_deact_cell19,
                                &hf_mac_nr_control_scell_act_deact_cell18,
                                &hf_mac_nr_control_scell_act_deact_cell17,
                                &hf_mac_nr_control_scell_act_deact_cell16,
                                NULL
                            };
                            static const int * scell_act_deact_4_flags[] = {
                                &hf_mac_nr_control_scell_act_deact_cell31,
                                &hf_mac_nr_control_scell_act_deact_cell30,
                                &hf_mac_nr_control_scell_act_deact_cell29,
                                &hf_mac_nr_control_scell_act_deact_cell28,
                                &hf_mac_nr_control_scell_act_deact_cell27,
                                &hf_mac_nr_control_scell_act_deact_cell26,
                                &hf_mac_nr_control_scell_act_deact_cell25,
                                &hf_mac_nr_control_scell_act_deact_cell24,
                                NULL
                            };
                            proto_tree_add_bitmask_list(subheader_tree, tvb, offset, 1, scell_act_deact_1_flags, ENC_NA);
                            offset++;
                            proto_tree_add_bitmask_list(subheader_tree, tvb, offset, 1, scell_act_deact_2_flags, ENC_NA);
                            offset++;
                            proto_tree_add_bitmask_list(subheader_tree, tvb, offset, 1, scell_act_deact_3_flags, ENC_NA);
                            offset++;
                            proto_tree_add_bitmask_list(subheader_tree, tvb, offset, 1, scell_act_deact_4_flags, ENC_NA);
                            offset++;

                            write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo,
                                                             "(SCell Act/Deact 4) ");
                        }
                        break;
                    case SCELL_ACTIVATION_DEACTIVATION_1_LCID:
                        {
                            static const int * scell_act_deact_1_flags[] = {
                                &hf_mac_nr_control_scell_act_deact_cell7,
                                &hf_mac_nr_control_scell_act_deact_cell6,
                                &hf_mac_nr_control_scell_act_deact_cell5,
                                &hf_mac_nr_control_scell_act_deact_cell4,
                                &hf_mac_nr_control_scell_act_deact_cell3,
                                &hf_mac_nr_control_scell_act_deact_cell2,
                                &hf_mac_nr_control_scell_act_deact_cell1,
                                &hf_mac_nr_control_scell_act_deact_reserved,
                                NULL
                            };
                            proto_tree_add_bitmask_list(subheader_tree, tvb, offset, 1, scell_act_deact_1_flags, ENC_NA);
                            offset++;

                            write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo, "(SCell Act/Deact 1) ");
                        }
                        break;
                    case LONG_DRX_COMMAND_LCID:
                        /* Fixed size of zero bits */
                        write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo, "(Long DRX) ");
                        break;
                    case DRX_COMMAND_LCID:
                        write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo, "(DRX) ");
                        break;
                    case TIMING_ADVANCE_COMMAND_LCID:
                        /* TAG ID (2 bits) */
                        proto_tree_add_item_ret_uint(subheader_tree, hf_mac_nr_control_timing_advance_tagid,
                                                     tvb, offset, 1, ENC_BIG_ENDIAN, &ta_tag_id);

                        /* Timing Advance Command (6 bits) */
                        proto_tree_add_item_ret_uint(subheader_tree, hf_mac_nr_control_timing_advance_command,
                                                     tvb, offset, 1, ENC_BIG_ENDIAN, &ta_ta);
                        offset++;

                        write_pdu_label_and_info(pdu_ti, subheader_ti, pinfo,
                                                 "(TAG=%u TA=%u) ", ta_tag_id, ta_ta);
                        break;
                    case UE_CONTENTION_RESOLUTION_IDENTITY_LCID:
                        proto_tree_add_item(subheader_tree, hf_mac_nr_control_ue_contention_resolution_identity,
                                            tvb, offset, 6, ENC_NA);
                        offset += 6;
                        write_pdu_label_and_info_literal(pdu_ti, subheader_ti, pinfo, "(Contention Resolution) ");
                        break;
                    case PADDING_LCID:
                        {
                            /* The rest of the PDU is padding */
                            int pad_len = tvb_reported_length_remaining(tvb, offset);
                            if (pad_len > 0)
                                proto_tree_add_item(subheader_tree, hf_mac_nr_padding, tvb, offset, -1, ENC_NA);
                            write_pdu_label_and_info(pdu_ti, subheader_ti, pinfo, "(Padding %u bytes) ", pad_len);
                            /* Move to the end of the frame */
                            offset = tvb_reported_length(tvb);
                        }
                        break;
                }
            }
        }

        /* Set subheader extent here */
        proto_item_set_end(subheader_ti, tvb, offset);

    } while (tvb_reported_length_remaining(tvb, offset));

}


/*****************************/
/* Main dissection function. */
static int dissect_mac_nr(tvbuff_t *tvb, packet_info *pinfo,
                          proto_tree *tree, void* data _U_)
{
    proto_tree          *mac_nr_tree;
    proto_item          *pdu_ti;
    proto_tree          *context_tree;
    proto_item          *context_ti, *ti;
    gint                 offset = 0;
    struct mac_nr_info *p_mac_nr_info;

    /* Set protocol name */
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "MAC-NR");

    /* Create protocol tree */
    pdu_ti = proto_tree_add_item(tree, proto_mac_nr, tvb, offset, tvb_reported_length(tvb), ENC_NA);
    proto_item_append_text(pdu_ti, " ");
    mac_nr_tree = proto_item_add_subtree(pdu_ti, ett_mac_nr);

    /* Look for packet info! */
    p_mac_nr_info = (mac_nr_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_mac_nr, 0);

    /* Can't dissect anything without it... */
    if (p_mac_nr_info == NULL) {
        proto_tree_add_expert(mac_nr_tree, pinfo, &ei_mac_nr_no_per_frame_data, tvb, offset, -1);
        return 0;
    }

    /* Clear info column */
    col_clear(pinfo->cinfo, COL_INFO);

    /* Restart this count */
    s_number_of_rlc_pdus_shown = 0;


    /*****************************************/
    /* Show context information              */

    /* Create context root */
    context_ti = proto_tree_add_string_format(mac_nr_tree, hf_mac_nr_context,
                                              tvb, offset, 0, "", "Context");
    context_tree = proto_item_add_subtree(context_ti, ett_mac_nr_context);
    PROTO_ITEM_SET_GENERATED(context_ti);

    /* Radio type */
    ti = proto_tree_add_uint(context_tree, hf_mac_nr_context_radio_type,
                             tvb, 0, 0, p_mac_nr_info->radioType);
    PROTO_ITEM_SET_GENERATED(ti);

    /* Direction */
    ti = proto_tree_add_uint(context_tree, hf_mac_nr_context_direction,
                             tvb, 0, 0, p_mac_nr_info->direction);
    PROTO_ITEM_SET_GENERATED(ti);

    /* RNTI type and value */
    if (p_mac_nr_info->rntiType != NO_RNTI) {
        ti = proto_tree_add_uint(context_tree, hf_mac_nr_context_rnti,
                                 tvb, 0, 0, p_mac_nr_info->rnti);
        PROTO_ITEM_SET_GENERATED(ti);
        proto_item_append_text(context_ti, " (RNTI=%u)", p_mac_nr_info->rnti);
    }

    ti = proto_tree_add_uint(context_tree, hf_mac_nr_context_rnti_type,
                             tvb, 0, 0, p_mac_nr_info->rntiType);
    PROTO_ITEM_SET_GENERATED(ti);

    /* UEId */
    if (p_mac_nr_info->ueid != 0) {
        ti = proto_tree_add_uint(context_tree, hf_mac_nr_context_ueid,
                                 tvb, 0, 0, p_mac_nr_info->ueid);
        PROTO_ITEM_SET_GENERATED(ti);
    }

    if (p_mac_nr_info->rntiType == C_RNTI || p_mac_nr_info->rntiType == CS_RNTI) {
        /* Harqid */
        ti = proto_tree_add_uint(context_tree, hf_mac_nr_context_harqid,
                                tvb, 0, 0, p_mac_nr_info->harqid);
        PROTO_ITEM_SET_GENERATED(ti);

        if (p_mac_nr_info->direction == DIRECTION_UPLINK) {
            /* Type 2 other */
            ti = proto_tree_add_boolean(context_tree, hf_mac_nr_context_phr_type2_othercell,
                                        tvb, 0, 0, p_mac_nr_info->phr_type2_othercell);
            PROTO_ITEM_SET_GENERATED(ti);
        }
    }


    /* Dissect the MAC PDU itself. Format depends upon RNTI type. */
    switch (p_mac_nr_info->rntiType) {

        case P_RNTI:
            /* PCCH PDU */
            dissect_pcch(tvb, pinfo, mac_nr_tree, pdu_ti, offset, p_mac_nr_info);
            break;

        case RA_RNTI:
            /* RAR PDU */
            dissect_rar(tvb, pinfo, mac_nr_tree, pdu_ti, offset, p_mac_nr_info);
            break;

        case C_RNTI:
        case CS_RNTI:
            /* Can be UL-SCH or DL-SCH */
            dissect_ulsch_or_dlsch(tvb, pinfo, mac_nr_tree, pdu_ti, offset,
                                   p_mac_nr_info,
                                   context_tree);
            break;

        case SI_RNTI:
            /* BCCH over DL-SCH */
            dissect_bcch(tvb, pinfo, mac_nr_tree, pdu_ti, offset, p_mac_nr_info);
            break;

        case NO_RNTI:
            /* Must be BCCH over BCH... */
            dissect_bcch(tvb, pinfo, mac_nr_tree, pdu_ti, offset, p_mac_nr_info);
            break;


        default:
            break;
    }

    return -1;
}

/* Heuristic dissector looks for supported framing protocol (see header file for details) */
static gboolean dissect_mac_nr_heur(tvbuff_t *tvb, packet_info *pinfo,
                                    proto_tree *tree, void *data _U_)
{
    gint        offset = 0;
    mac_nr_info *p_mac_nr_info;
    tvbuff_t    *mac_tvb;
    guint8      tag;

    /* Needs to be at least as long as:
       - the signature string
       - fixed header bytes
       - tag for data
       - at least one byte of MAC PDU payload */
    if (tvb_captured_length_remaining(tvb, offset) < (gint)(strlen(MAC_NR_START_STRING)+3+2)) {
        return FALSE;
    }

    /* OK, compare with signature string */
    if (tvb_strneql(tvb, offset, MAC_NR_START_STRING, strlen(MAC_NR_START_STRING)) != 0) {
        return FALSE;
    }
    offset += (gint)strlen(MAC_NR_START_STRING);

    /* If redissecting, use previous info struct (if available) */
    p_mac_nr_info = (mac_nr_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_mac_nr, 0);
    if (p_mac_nr_info == NULL) {
        /* Allocate new info struct for this frame */
        p_mac_nr_info = wmem_new0(wmem_file_scope(), mac_nr_info);

        /* Read fixed fields */
        p_mac_nr_info->radioType = tvb_get_guint8(tvb, offset++);
        p_mac_nr_info->direction = tvb_get_guint8(tvb, offset++);
        p_mac_nr_info->rntiType = tvb_get_guint8(tvb, offset++);

        /* Read optional fields */
        do {
            /* Process next tag */
            tag = tvb_get_guint8(tvb, offset++);
            switch (tag) {
                case MAC_NR_RNTI_TAG:
                    p_mac_nr_info->rnti = tvb_get_ntohs(tvb, offset);
                    offset += 2;
                    break;
                case MAC_NR_UEID_TAG:
                    p_mac_nr_info->ueid = tvb_get_ntohs(tvb, offset);
                    offset += 2;
                    break;
                case MAC_NR_HARQID:
                    p_mac_nr_info->harqid = tvb_get_guint8(tvb, offset);
                    offset++;
                    break;
                case MAC_NR_FRAME_SUBFRAME_TAG:
                    p_mac_nr_info->sysframeNumber = tvb_get_bits16(tvb, offset<<3, 12, ENC_BIG_ENDIAN);
                    p_mac_nr_info->subframeNumber = tvb_get_bits8(tvb, ((offset+1)<<3)+4, 4);
                    offset += 2;
                    break;
                case MAC_NR_PHR_TYPE2_OTHERCELL_TAG:
                    p_mac_nr_info->phr_type2_othercell = tvb_get_guint8(tvb, offset);
                    offset++;
                    break;
                case MAC_NR_PAYLOAD_TAG:
                    /* Have reached data, so set payload length and get out of loop */
                    /* TODO: this is not correct if there is padding which isn't in frame */
                    p_mac_nr_info->length = tvb_reported_length_remaining(tvb, offset);
                    continue;
                default:
                    /* It must be a recognised tag */
                    {
                        proto_item *ti;
                        proto_tree *subtree;

                        col_set_str(pinfo->cinfo, COL_PROTOCOL, "MAC-NR");
                        col_clear(pinfo->cinfo, COL_INFO);
                        ti = proto_tree_add_item(tree, proto_mac_nr, tvb, offset, tvb_reported_length(tvb), ENC_NA);
                        subtree = proto_item_add_subtree(ti, ett_mac_nr);
                        proto_tree_add_expert(subtree, pinfo, &ei_mac_nr_unknown_udp_framing_tag,
                                              tvb, offset-1, 1);
                    }
                    wmem_free(wmem_file_scope(), p_mac_nr_info);
                    return TRUE;
            }
        } while (tag != MAC_NR_PAYLOAD_TAG);

        p_add_proto_data(wmem_file_scope(), pinfo, proto_mac_nr, 0, p_mac_nr_info);
    }
    else {
        offset = tvb_reported_length(tvb) - p_mac_nr_info->length;
    }

    /**************************************/
    /* OK, now dissect as MAC NR          */

    /* Create tvb that starts at actual MAC PDU */
    mac_tvb = tvb_new_subset_remaining(tvb, offset);
    dissect_mac_nr(mac_tvb, pinfo, tree, NULL);

    return TRUE;
}


/* Callback used as part of configuring a channel mapping using UAT */
static void* lcid_drb_mapping_copy_cb(void* dest, const void* orig, size_t len _U_)
{
    const lcid_drb_mapping_t *o = (const lcid_drb_mapping_t *)orig;
    lcid_drb_mapping_t       *d = (lcid_drb_mapping_t *)dest;

    /* Copy all items over */
    d->lcid  = o->lcid;
    d->drbid = o->drbid;
    d->bearer_type = o->bearer_type;

    return d;
}

/* Function to be called from outside this module (e.g. in a plugin) to get per-packet data */
mac_nr_info *get_mac_nr_proto_data(packet_info *pinfo)
{
    return (mac_nr_info *)p_get_proto_data(wmem_file_scope(), pinfo, proto_mac_nr, 0);
}

/* Function to be called from outside this module (e.g. in a plugin) to set per-packet data */
void set_mac_nr_proto_data(packet_info *pinfo, mac_nr_info *p_mac_nr_info)
{
    p_add_proto_data(wmem_file_scope(), pinfo, proto_mac_nr, 0, p_mac_nr_info);
}

void proto_register_mac_nr(void)
{
    static hf_register_info hf[] =
    {
        /**********************************/
        /* Items for decoding context     */
        { &hf_mac_nr_context,
            { "Context",
              "mac-nr.context", FT_STRING, BASE_NONE, NULL, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_context_radio_type,
            { "Radio Type",
              "mac-nr.radio-type", FT_UINT8, BASE_DEC, VALS(radio_type_vals), 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_context_direction,
            { "Direction",
              "mac-nr.direction", FT_UINT8, BASE_DEC, VALS(direction_vals), 0x0,
              "Direction of message", HFILL
            }
        },
        { &hf_mac_nr_context_rnti,
            { "RNTI",
              "mac-nr.rnti", FT_UINT16, BASE_HEX_DEC, NULL, 0x0,
              "RNTI associated with message", HFILL
            }
        },
        { &hf_mac_nr_context_rnti_type,
            { "RNTI Type",
              "mac-nr.rnti-type", FT_UINT8, BASE_DEC, VALS(rnti_type_vals), 0x0,
              "Type of RNTI associated with message", HFILL
            }
        },
        { &hf_mac_nr_context_ueid,
            { "UEId",
              "mac-nr.ueid", FT_UINT16, BASE_DEC, NULL, 0x0,
              "User Equipment Identifier associated with message", HFILL
            }
        },
        { &hf_mac_nr_context_harqid,
            { "HarqId",
              "mac-nr.harqid", FT_UINT8, BASE_DEC, NULL, 0x0,
              "HARQ Identifier", HFILL
            }
        },
        { &hf_mac_nr_context_bcch_transport_channel,
            { "Transport channel",
              "mac-nr.bcch-transport-channel", FT_UINT8, BASE_DEC, VALS(bcch_transport_channel_vals), 0x0,
              "Transport channel BCCH data was carried on", HFILL
            }
        },
        { &hf_mac_nr_context_phr_type2_othercell,
            { "PHR Type2 other cell PHR",
              "mac-nr.type2-other-cell", FT_BOOLEAN, BASE_NONE, NULL, 0x0,
              NULL, HFILL
            }
        },

        { &hf_mac_nr_subheader,
            { "Subheader",
              "mac-nr.subheader", FT_STRING, BASE_NONE, NULL, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_subheader_reserved,
            { "Reserved",
              "mac-nr.subheader.reserved", FT_UINT8, BASE_HEX, NULL, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_subheader_f,
            { "Format",
              "mac-nr.subheader.f", FT_BOOLEAN, 8, TFS(&subheader_f_vals), 0x40,
              "Format of subheader length field", HFILL
            }
        },
        { &hf_mac_nr_subheader_length_1_byte,
            { "SDU Length",
              "mac-nr.subheader.sdu-length", FT_UINT8, BASE_DEC, NULL, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_subheader_length_2_bytes,
            { "SDU Length",
              "mac-nr.subheader.sdu-length", FT_UINT16, BASE_DEC, NULL, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_ulsch_lcid,
            { "LCID",
              "mac-nr.ulsch.lcid", FT_UINT8, BASE_HEX|BASE_EXT_STRING, &ulsch_lcid_vals_ext, 0x3f,
              "UL-SCH Logical Channel Identifier", HFILL
            }
        },
        { &hf_mac_nr_dlsch_lcid,
            { "LCID",
              "mac-nr.dlsch.lcid", FT_UINT8, BASE_HEX|BASE_EXT_STRING, &dlsch_lcid_vals_ext, 0x3f,
              "DL-SCH Logical Channel Identifier", HFILL
            }
        },
        { &hf_mac_nr_ulsch_sdu,
            { "UL-SCH SDU",
              "mac-nr.ulsch.sdu", FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_dlsch_sdu,
            { "DL-SCH SDU",
              "mac-nr.dlsch.sdu", FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_bcch_pdu,
            { "BCCH PDU",
              "mac-nr.bcch.pdu", FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_pcch_pdu,
            { "PCCH PDU",
              "mac-nr.pcch.pdu", FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL
            }
        },


        /*********************************/
        /* RAR fields                    */
        { &hf_mac_nr_rar,
            { "RAR",
              "mac-nr.rar", FT_NONE, BASE_NONE, NULL, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_rar_e,
            { "Extension",
              "mac-nr.rar.e", FT_BOOLEAN, 8, TFS(&rar_ext_vals), 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_rar_t,
            { "Type",
              "mac-nr.rar.t", FT_BOOLEAN, 8, TFS(&rar_type_vals), 0x40,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_rar_reserved,
            { "Reserved",
              "mac-nr.rar.reserved", FT_UINT8, BASE_DEC, NULL, 0x30,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_rar_reserved1,
            { "Reserved",
              "mac-nr.rar.reserved", FT_UINT8, BASE_DEC, NULL, 0x80,
              NULL, HFILL
            }
        },

        { &hf_mac_nr_rar_subheader,
            { "Subheader",
              "mac-nr.rar.subheader", FT_STRING, BASE_NONE, NULL, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_rar_bi,
            { "Backoff Indicator",
              "mac-nr.rar.bi", FT_UINT8, BASE_DEC, VALS(rar_bi_vals), 0x0f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_rar_rapid,
            { "RAPID",
              "mac-nr.rar.rapid", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_rar_ta,
            { "Timing Advance",
              "mac-nr.rar.ta", FT_UINT16, BASE_DEC, NULL, 0x7ff8,
              NULL, HFILL
            }
        },

        { &hf_mac_nr_rar_grant,
            { "Grant",
              "mac-nr.rar.grant", FT_UINT32, BASE_HEX, NULL, 0x07ffffff,
              "UL Grant details", HFILL
            }
        },
        { &hf_mac_nr_rar_grant_hopping,
            { "Frequency hopping flag",
              "mac-nr.rar.grant.hopping", FT_BOOLEAN, 32, TFS(&tfs_set_notset), 0x04000000,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_rar_grant_fra,
            { "Msg3 PUSCH frequency resource allocation",
              "mac-nr.rar.grant.fra", FT_UINT32, BASE_DEC, NULL, 0x03fff000,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_rar_grant_tsa,
            { "Msg3 PUSCH time resource allocation",
              "mac-nr.rar.grant.tsa", FT_UINT32, BASE_DEC, NULL, 0x00000f00,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_rar_grant_mcs,
            { "MCS",
              "mac-nr.rar.grant.mcs", FT_UINT32, BASE_DEC, NULL, 0x000000f0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_rar_grant_tcsp,
            { "TPC command for Msg3 PUSCH",
              "mac-nr.rar.grant.tcsp", FT_UINT32, BASE_DEC, VALS(tpc_command_vals), 0x0000000e,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_rar_grant_csi,
            { "CSI request",
              "mac-nr.rar.grant.csi", FT_BOOLEAN, 32, TFS(&tfs_set_notset), 0x00000001,
              NULL, HFILL
            }
        },

        { &hf_mac_nr_rar_temp_crnti,
            { "Temporary C-RNTI",
              "mac-nr.rar.temp_crnti", FT_UINT16, BASE_HEX_DEC, NULL, 0x0,
              NULL, HFILL
            }
        },

        { &hf_mac_nr_padding,
            { "Padding",
              "mac-nr.padding", FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL
            }
        },

        { &hf_mac_nr_control_crnti,
            { "C-RNTI",
              "mac-nr.control.crnti", FT_UINT16, BASE_HEX_DEC, NULL, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_ue_contention_resolution_identity,
            { "UE Contention Resolution Identity",
              "mac-nr.control.ue-contention-resolution.identity", FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_timing_advance_tagid,
            { "TAG ID",
              "mac-nr.control.timing-advance.tag-id", FT_UINT8, BASE_DEC, NULL, 0xc0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_timing_advance_command,
            { "Timing Advance Command",
              "mac-nr.control.timing-advance.command", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_se_phr_reserved,
            { "Reserved",
              "mac-nr.control.se-phr.reserved", FT_UINT8, BASE_HEX, NULL, 0xc0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_se_phr_ph,
            { "Power Headroom",
              "mac-nr.control.se-phr.ph", FT_UINT8, BASE_CUSTOM, CF_FUNC(mac_nr_phr_fmt), 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_se_phr_pcmax_f_c,
            { "Pcmax,c,f",
              "mac-nr.control.se-phr.pcmax_f_c", FT_UINT8, BASE_CUSTOM, CF_FUNC(mac_nr_pcmax_f_c_fmt), 0x3f,
              NULL, HFILL
            }
        },

        { &hf_mac_nr_control_recommended_bit_rate_query_lcid,
            { "LCID",
              "mac-nr.control.recommended-bit-rate-query.lcid", FT_UINT8, BASE_DEC, NULL, 0xfc,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_recommended_bit_rate_query_dir,
            { "Direction",
              "mac-nr.control.recommended-bit-rate-query.dir", FT_BOOLEAN, 8, TFS(&tfs_uplink_downlink), 0x02,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_recommended_bit_rate_query_bit_rate,
            { "Bit Rate",
              "mac-nr.control.recommended-bit-rate-query.bit-rate", FT_UINT16, BASE_DEC|BASE_EXT_STRING, &bit_rate_vals_ext, 0x01f8,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_recommended_bit_rate_query_reserved,
            { "Reserved",
              "mac-nr.control.recommended-bit-rate-query.reserved", FT_UINT8, BASE_DEC, NULL, 0x07,
              NULL, HFILL
            }
        },

        { &hf_mac_nr_control_me_phr_c7_flag,
            { "C7",
              "mac-nr.control.me-phr.c7", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x80,
              "SCellIndex 7 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c6_flag,
            { "C6",
              "mac-nr.control.me-phr.c6", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x40,
              "SCellIndex 6 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c5_flag,
            { "C5",
              "mac-nr.control.me-phr.c5", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x20,
              "SCellIndex 5 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c4_flag,
            { "C4",
              "mac-nr.control.me-phr.c4", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x10,
              "SCellIndex 4 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c3_flag,
            { "C3",
              "mac-nr.control.me-phr.c3", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x08,
              "SCellIndex 3 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c2_flag,
            { "C2",
              "mac-nr.control.me-phr.c2", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x04,
              "SCellIndex 2 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c1_flag,
            { "C1",
              "mac-nr.control.me-phr.c1", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x02,
              "SCellIndex 1 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c15_flag,
            { "C15",
              "mac-nr.control.me-phr.c15", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x80,
              "SCellIndex 15 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c14_flag,
            { "C14",
              "mac-nr.control.me-phr.c14", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x40,
              "SCellIndex 14 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c13_flag,
            { "C13",
              "mac-nr.control.me-phr.c13", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x20,
              "SCellIndex 13 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c12_flag,
            { "C12",
              "mac-nr.control.me-phr.c12", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x10,
              "SCellIndex 12 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c11_flag,
            { "C11",
              "mac-nr.control.me-phr.c11", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x08,
              "SCellIndex 11 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c10_flag,
            { "C10",
              "mac-nr.control.me-phr.c10", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x04,
              "SCellIndex 10 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c9_flag,
            { "C9",
              "mac-nr.control.me-phr.c9", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x02,
              "SCellIndex 9 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c8_flag,
            { "C8",
              "mac-nr.control.me-phr.c8", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x01,
              "SCellIndex 8 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c23_flag,
            { "C23",
              "mac-nr.control.me-phr.c23", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x80,
              "SCellIndex 23 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c22_flag,
            { "C22",
              "mac-nr.control.me-phr.c22", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x40,
              "SCellIndex 22 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c21_flag,
            { "C21",
              "mac-nr.control.me-phr.c21", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x20,
              "SCellIndex 21 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c20_flag,
            { "C20",
              "mac-nr.control.me-phr.c20", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x10,
              "SCellIndex 20 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c19_flag,
            { "C19",
              "mac-nr.control.me-phr.c19", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x08,
              "SCellIndex 19 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c18_flag,
            { "C18",
              "mac-nr.control.me-phr.c18", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x04,
              "SCellIndex 18 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c17_flag,
            { "C17",
              "mac-nr.control.me-phr.c17", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x02,
              "SCellIndex 17 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c16_flag,
            { "C16",
              "mac-nr.control.me-phr.c16", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x01,
              "SCellIndex 16 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c31_flag,
            { "C31",
              "mac-nr.control.me-phr.c31", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x80,
              "SCellIndex 31 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c30_flag,
            { "C30",
              "mac-nr.control.me-phr.c30", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x40,
              "SCellIndex 30 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c29_flag,
            { "C29",
              "mac-nr.control.me-phr.c29", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x20,
              "SCellIndex 29 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c28_flag,
            { "C28",
              "mac-nr.control.me-phr.c28", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x10,
              "SCellIndex 28 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c27_flag,
            { "C27",
              "mac-nr.control.me-phr.c27", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x08,
              "SCellIndex 27 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c26_flag,
            { "C26",
              "mac-nr.control.me-phr.c26", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x04,
              "SCellIndex 26 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c25_flag,
            { "C25",
              "mac-nr.control.me-phr.c25", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x02,
              "SCellIndex 25 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_c24_flag,
            { "C24",
              "mac-nr.control.me-phr.c24", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x01,
              "SCellIndex 24 PHR report flag", HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_entry,
            { "Entry",
              "mac-nr.control.me.phr.entry", FT_STRING, BASE_NONE, NULL, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_reserved,
            { "Reserved",
              "mac-nr.control.me-phr.reserved", FT_BOOLEAN, 8, NULL, 0x01,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_p,
            { "P",
              "mac-nr.control.me-phr.p", FT_BOOLEAN, 8, TFS(&power_backoff_affects_power_management_vals), 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_v,
            { "V",
              "mac-nr.control.me-phr.v", FT_BOOLEAN, 8, TFS(&phr_source_vals), 0x40,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_type2_spcell,
            { "Power Headroom, (Type2, SpCell)",
              "mac-nr.control.me-phr.ph", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_type1_pcell,
            { "Power Headroom (Type1, PCell)",
              "mac-nr.control.me-phr.ph.type1-pcell", FT_UINT8, BASE_CUSTOM, CF_FUNC(mac_nr_phr_fmt), 0x3f,
              NULL, HFILL
            }
        },

        { &hf_mac_nr_control_me_phr_ph_c31,
            { "PH for SCellIndex 31",
              "mac-nr.control.me-phr.ph.c31", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c30,
            { "PH for SCellIndex 30",
              "mac-nr.control.me-phr.ph.c30", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c29,
            { "PH for SCellIndex 29",
              "mac-nr.control.me-phr.ph.c29", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c28,
            { "PH for SCellIndex 28",
              "mac-nr.control.me-phr.ph.c28", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c27,
            { "PH for SCellIndex 27",
              "mac-nr.control.me-phr.ph.c27", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c26,
            { "PH for SCellIndex 26",
              "mac-nr.control.me-phr.ph.c26", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c25,
            { "PH for SCellIndex 25",
              "mac-nr.control.me-phr.ph.c25", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c24,
            { "PH for SCellIndex 24",
              "mac-nr.control.me-phr.ph.c24", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c23,
            { "PH for SCellIndex 23",
              "mac-nr.control.me-phr.ph.c23", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c22,
            { "PH for SCellIndex 22",
              "mac-nr.control.me-phr.ph.c22", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c21,
            { "PH for SCellIndex 21",
              "mac-nr.control.me-phr.ph.c21", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c20,
            { "PH for SCellIndex 20",
              "mac-nr.control.me-phr.ph.c20", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c19,
            { "PH for SCellIndex 19",
              "mac-nr.control.me-phr.ph.c19", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c18,
            { "PH for SCellIndex 18",
              "mac-nr.control.me-phr.ph.c18", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c17,
            { "PH for SCellIndex 17",
              "mac-nr.control.me-phr.ph.c17", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c16,
            { "PH for SCellIndex 16",
              "mac-nr.control.me-phr.ph.c16", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c15,
            { "PH for SCellIndex 15",
              "mac-nr.control.me-phr.ph.c15", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c14,
            { "PH for SCellIndex 14",
              "mac-nr.control.me-phr.ph.c14", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c13,
            { "PH for SCellIndex 13",
              "mac-nr.control.me-phr.ph.c13", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c12,
            { "PH for SCellIndex 12",
              "mac-nr.control.me-phr.ph.c12", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c11,
            { "PH for SCellIndex 11",
              "mac-nr.control.me-phr.ph.c11", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c10,
            { "PH for SCellIndex 10",
              "mac-nr.control.me-phr.ph.c10", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c9,
            { "PH for SCellIndex 9",
              "mac-nr.control.me-phr.ph.c9", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c8,
            { "PH for SCellIndex 8",
              "mac-nr.control.me-phr.ph.c8", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c7,
            { "PH for SCellIndex 7",
              "mac-nr.control.me-phr.ph.c7", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c6,
            { "PH for SCellIndex 6",
              "mac-nr.control.me-phr.ph.c6", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c5,
            { "PH for SCellIndex 5",
              "mac-nr.control.me-phr.ph.c5", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c4,
            { "PH for SCellIndex 4",
              "mac-nr.control.me-phr.ph.c4", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c3,
            { "PH for SCellIndex 3",
              "mac-nr.control.me-phr.ph.c3", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c2,
            { "PH for SCellIndex 2",
              "mac-nr.control.me-phr.ph.c2", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_ph_c1,
            { "PH for SCellIndex 1",
              "mac-nr.control.me-phr.ph.c1", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_reserved_2,
            { "Reserved",
              "mac-nr.control.me-phr.reserved", FT_BOOLEAN, 8, NULL, 0xc0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_pcmax_f_c_type2_spcell,
            { "Pcmax,f,c",
              "mac-nr.control.me-phr.type2-spcell", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_pcmax_f_c_type1_pcell,
            { "Pcmax,f,c",
              "mac-nr.control.me-phr.type1-pcell", FT_UINT8, BASE_CUSTOM, CF_FUNC(mac_nr_pcmax_f_c_fmt), 0x3f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_me_phr_pcmax_f_c_typeX,
            { "Pcmax,f,c",
              "mac-nr.control.me-phr.typeX", FT_UINT8, BASE_DEC, NULL, 0x3f,
              NULL, HFILL
            }
        },

        { &hf_mac_nr_control_recommended_bit_rate_lcid,
            { "LCID",
              "mac-nr.control.recommended-bit-rate.lcid", FT_UINT8, BASE_DEC, NULL, 0xfc,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_recommended_bit_rate_dir,
            { "Direction",
              "mac-nr.control.recommended-bit-rate.dir", FT_BOOLEAN, 8, TFS(&tfs_uplink_downlink), 0x02,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_recommended_bit_rate_bit_rate,
            { "Bit Rate",
              "mac-nr.control.recommended-bit-rate.bit-rate", FT_UINT16, BASE_DEC|BASE_EXT_STRING, &bit_rate_vals_ext, 0x01f8,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_recommended_bit_rate_reserved,
            { "Reserved",
              "mac-nr.control.recommended-bit-rate.reserved", FT_UINT8, BASE_DEC, NULL, 0x07,
              NULL, HFILL
            }
        },

        { &hf_mac_control_sp_zp_csi_rs_resource_set_act_deact_ad,
            { "Reserved",
              "mac-nr.control.sp-zp-csi-rs-resource-set-act-deact.ad", FT_BOOLEAN, 8, TFS(&activation_deactivation_vals), 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_control_sp_zp_csi_rs_resource_set_act_deact_serving_cell_id,
            { "Serving Cell ID",
              "mac-nr.control.sp-zp-csi-rs-resource-set-act-deact.serving-cell-id", FT_UINT8, BASE_DEC, NULL, 0x7c,
              NULL, HFILL
            }
        },
        { &hf_mac_control_sp_zp_csi_rs_resource_set_act_deact_bwp_id,
            { "BWP ID",
              "mac-nr.control.sp-zp-csi-rs-resource-set-act-deact.bwp-id", FT_UINT8, BASE_DEC, NULL, 0x03,
              NULL, HFILL
            }
        },
        { &hf_mac_control_sp_zp_csi_rs_resource_set_act_deact_reserved_2,
            { "Reserved",
              "mac-nr.control.sp-zp-csi-rs-resource-set-act-deact.reserved", FT_UINT8, BASE_HEX, NULL, 0xf0,
              NULL, HFILL
            }
        },
        { &hf_mac_control_sp_zp_csi_rs_resource_set_act_deact_sp_zp_rs_resource_set_id,
            { "SP ZP CSI-RS resource set ID",
              "mac-nr.control.sp-zp-csi-rs-resource-set-act-deact.sp-zp-rs-resource-set-id", FT_UINT8, BASE_DEC, NULL, 0x0f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_pucch_spatial_rel_act_deact_reserved,
            { "Reserved",
              "mac-nr.control.pucch-spatial-rel-act-deact.reserved", FT_UINT8, BASE_HEX, NULL, 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_pucch_spatial_rel_act_deact_serving_cell_id,
            { "Serving Cell ID",
              "mac-nr.control.pucch-spatial-rel-act-deact.serving-cell-id", FT_UINT8, BASE_DEC, NULL, 0x7c,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_pucch_spatial_rel_act_deact_bwp_id,
            { "BWP ID",
              "mac-nr.control.pucch-spatial-rel-act-deact.bwp-id", FT_UINT8, BASE_DEC, NULL, 0x03,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_pucch_spatial_rel_act_deact_pucch_resource_id,
            { "PUCCH Resource ID",
              "mac-nr.control.pucch-spatial-rel-act-deact.pucch-resource-id", FT_UINT8, BASE_DEC, NULL, 0x7f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_pucch_spatial_rel_act_deact_s7,
            { "PUCCH Spatial Relation Info 7",
              "mac-nr.control.pucch-spatial-rel-act-deact.s7", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_pucch_spatial_rel_act_deact_s6,
            { "PUCCH Spatial Relation Info 6",
              "mac-nr.control.pucch-spatial-rel-act-deact.s6", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x40,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_pucch_spatial_rel_act_deact_s5,
            { "PUCCH Spatial Relation Info 5",
              "mac-nr.control.pucch-spatial-rel-act-deact.s5", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x20,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_pucch_spatial_rel_act_deact_s4,
            { "PUCCH Spatial Relation Info 4",
              "mac-nr.control.pucch-spatial-rel-act-deact.s4", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x10,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_pucch_spatial_rel_act_deact_s3,
            { "PUCCH Spatial Relation Info 3",
              "mac-nr.control.pucch-spatial-rel-act-deact.s3", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x08,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_pucch_spatial_rel_act_deact_s2,
            { "PUCCH Spatial Relation Info 2",
              "mac-nr.control.pucch-spatial-rel-act-deact.s2", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x04,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_pucch_spatial_rel_act_deact_s1,
            { "PUCCH Spatial Relation Info 1",
              "mac-nr.control.pucch-spatial-rel-act-deact.s1", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x02,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_pucch_spatial_rel_act_deact_s0,
            { "PUCCH Spatial Relation Info 0",
              "mac-nr.control.pucch-spatial-rel-act-deact.s0", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x01,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_srs_act_deact_ad,
            { "A/D",
              "mac-nr.control.sp-srs-act-deact.ad", FT_BOOLEAN, 8, TFS(&activation_deactivation_vals), 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_srs_act_deact_srs_resource_set_cell_id,
            { "SRS Resource Set's Cell ID",
              "mac-nr.control.sp-srs-act-deact.srs-resource-set-cell-id", FT_UINT8, BASE_DEC, NULL, 0x7c,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_srs_act_deact_srs_resource_set_bwp_id,
            { "SRS Resource Set's BWP ID",
              "mac-nr.control.sp-srs-act-deact.srs-resource-set-bwp-id", FT_UINT8, BASE_DEC, NULL, 0x03,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_srs_act_deact_reserved,
            { "Reserved",
              "mac-nr.control.sp-srs-act-deact.reserved", FT_UINT8, BASE_HEX, NULL, 0x00,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_srs_act_deact_c,
            { "C",
              "mac-nr.control.sp-srs-act-deact.c", FT_BOOLEAN, 8, TFS(&c_vals), 0x20,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_srs_act_deact_sul,
            { "SUL",
              "mac-nr.control.sp-srs-act-deact.sul", FT_BOOLEAN, 8, TFS(&sul_vals), 0x10,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_srs_act_deact_sp_srs_resource_set_id,
            { "SP SRS Resource Set ID",
              "mac-nr.control.sp-srs-act-deact.sp-srs-resource-set-id", FT_UINT8, BASE_DEC, NULL, 0x0f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_srs_act_deact_f,
            { "F",
              "mac-nr.control.sp-srs-act-deact.f", FT_BOOLEAN, 8, TFS(&sp_srs_act_deact_f_vals), 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_srs_act_deact_resource_id,
            { "Resource ID",
              "mac-nr.control.sp-srs-act-deact.sp-srs-resource-set-id", FT_UINT8, BASE_DEC, NULL, 0x7f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_srs_act_deact_resource_serving_cell_id,
            { "Resource Serving Cell ID",
              "mac-nr.control.sp-srs-act-deact.resource-serving-cell-id", FT_UINT8, BASE_DEC, NULL, 0x7c,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_srs_act_deact_resource_bwp_id,
            { "Resource BWP ID",
              "mac-nr.control.sp-srs-act-deact.resource-bwp-id", FT_UINT8, BASE_DEC, NULL, 0x03,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_reserved,
            { "Reserved",
              "mac-nr.control.sp-csi-report-on-pucch-act-deact.reserved", FT_UINT8, BASE_HEX, NULL, 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_serving_cell_id,
            { "Serving Cell ID",
              "mac-nr.control.sp-csi-report-on-pucch-act-deact.serving-cell-id", FT_UINT8, BASE_DEC, NULL, 0x7c,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_bwp_id,
            { "BWP ID",
              "mac-nr.control.sp-csi-report-on-pucch-act-deact.bwp-id", FT_UINT8, BASE_DEC, NULL, 0x03,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s7,
            { "Semi-Persistent CSI report configuration 7",
              "mac-nr.control.sp-csi-report-on-pucch-act-deact.s7", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s6,
            { "Semi-Persistent CSI report configuration 6",
              "mac-nr.control.sp-csi-report-on-pucch-act-deact.s6", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x40,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s5,
            { "Semi-Persistent CSI report configuration 5",
              "mac-nr.control.sp-csi-report-on-pucch-act-deact.s5", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x20,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s4,
            { "Semi-Persistent CSI report configuration 4",
              "mac-nr.control.sp-csi-report-on-pucch-act-deact.s4", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x10,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s3,
            { "Semi-Persistent CSI report configuration 3",
              "mac-nr.control.sp-csi-report-on-pucch-act-deact.s3", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x08,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s2,
            { "Semi-Persistent CSI report configuration 2",
              "mac-nr.control.sp-csi-report-on-pucch-act-deact.s2", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x04,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s1,
            { "Semi-Persistent CSI report configuration 1",
              "mac-nr.control.sp-csi-report-on-pucch-act-deact.s1", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x02,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_report_on_pucch_act_deact_s0,
            { "Semi-Persistent CSI report configuration 0",
              "mac-nr.control.sp-csi-report-on-pucch-act-deact.s0", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x01,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_tci_state_ind_for_ue_spec_pdcch_serving_cell_id,
            { "Serving Cell ID",
              "mac-nr.control.tci-state-ind-for-ue-spec-pdcch.serving-cell-id", FT_UINT8, BASE_DEC, NULL, 0xf8,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_tci_state_ind_for_ue_spec_pdcch_coreset_id,
            { "CORESET ID",
              "mac-nr.control.tci-state-ind-for-ue-spec-pdcch.coreset-id", FT_UINT16, BASE_DEC, NULL, 0x0780,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_tci_state_ind_for_ue_spec_pdcch_tci_state_id,
            { "TCI State ID",
              "mac-nr.control.tci-state-ind-for-ue-spec-pdcch.tci-state-id", FT_UINT8, BASE_DEC, NULL, 0x7f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_reserved,
            { "Reserved",
              "mac-nr.control.tci-states-act-deact-for-ue-spec-pdsch.reserved", FT_UINT8, BASE_HEX, NULL, 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_serving_cell_id,
            { "Serving Cell ID",
              "mac-nr.control.tci-states-act-deact-for-ue-spec-pdsch.serving-cell-id", FT_UINT8, BASE_DEC, NULL, 0x7c,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_bwp_id,
            { "BWP ID",
              "mac-nr.control.tci-states-act-deact-for-ue-spec-pdsch.bwp-id", FT_UINT8, BASE_DEC, NULL, 0x03,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t7,
            { "TCI state N+7",
              "mac-nr.control.tci-states-act-deact-for-ue-spec-pdsch.t7", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t6,
            { "TCI state N+6",
              "mac-nr.control.tci-states-act-deact-for-ue-spec-pdsch.t6", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x40,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t5,
            { "TCI state N+5",
              "mac-nr.control.tci-states-act-deact-for-ue-spec-pdsch.t5", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x20,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t4,
            { "TCI state N+4",
              "mac-nr.control.tci-states-act-deact-for-ue-spec-pdsch.t4", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x10,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t3,
            { "TCI state N+3",
              "mac-nr.control.tci-states-act-deact-for-ue-spec-pdsch.t3", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x08,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t2,
            { "TCI state N+2",
              "mac-nr.control.tci-states-act-deact-for-ue-spec-pdsch.t2", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x04,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t1,
            { "TCI state N+1",
              "mac-nr.control.tci-states-act-deact-for-ue-spec-pdsch.t1", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x02,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_tci_states_act_deact_for_ue_spec_pdsch_t0,
            { "TCI state N",
              "mac-nr.control.tci-states-act-deact-for-ue-spec-pdsch.t0", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x01,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_aper_csi_trigger_state_subselect_reserved,
            { "Reserved",
              "mac-nr.control.aper-csi-trigger-state-subselect.reserved", FT_UINT8, BASE_HEX, NULL, 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_aper_csi_trigger_state_subselect_serving_cell_id,
            { "Serving Cell ID",
              "mac-nr.control.aper-csi-trigger-state-subselect.serving-cell-id", FT_UINT8, BASE_DEC, NULL, 0x7c,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_aper_csi_trigger_state_subselect_bwp_id,
            { "BWP ID",
              "mac-nr.control.aper-csi-trigger-state-subselect.bwp-id", FT_UINT8, BASE_DEC, NULL, 0x03,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_aper_csi_trigger_state_subselect_t7,
            { "Aperiodic trigger state N+7",
              "mac-nr.control.aper-csi-trigger-state-subselect.t7", FT_BOOLEAN, 8, TFS(&aper_csi_trigger_state_t_vals), 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_aper_csi_trigger_state_subselect_t6,
            { "Aperiodic trigger state N+6",
              "mac-nr.control.aper-csi-trigger-state-subselect.t6", FT_BOOLEAN, 8, TFS(&aper_csi_trigger_state_t_vals), 0x40,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_aper_csi_trigger_state_subselect_t5,
            { "Aperiodic trigger state N+5",
              "mac-nr.control.aper-csi-trigger-state-subselect.t5", FT_BOOLEAN, 8, TFS(&aper_csi_trigger_state_t_vals), 0x20,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_aper_csi_trigger_state_subselect_t4,
            { "Aperiodic trigger state N+4",
              "mac-nr.control.aper-csi-trigger-state-subselect.t4", FT_BOOLEAN, 8, TFS(&aper_csi_trigger_state_t_vals), 0x10,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_aper_csi_trigger_state_subselect_t3,
            { "Aperiodic trigger state N+3",
              "mac-nr.control.aper-csi-trigger-state-subselect.t3", FT_BOOLEAN, 8, TFS(&aper_csi_trigger_state_t_vals), 0x08,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_aper_csi_trigger_state_subselect_t2,
            { "Aperiodic trigger state N+2",
              "mac-nr.control.aper-csi-trigger-state-subselect.t2", FT_BOOLEAN, 8, TFS(&aper_csi_trigger_state_t_vals), 0x04,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_aper_csi_trigger_state_subselect_t1,
            { "Aperiodic trigger state N+1",
              "mac-nr.control.aper-csi-trigger-state-subselect.t1", FT_BOOLEAN, 8, TFS(&aper_csi_trigger_state_t_vals), 0x02,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_aper_csi_trigger_state_subselect_t0,
            { "Aperiodic trigger state N",
              "mac-nr.control.aper-csi-trigger-state-subselect.t0", FT_BOOLEAN, 8, TFS(&aper_csi_trigger_state_t_vals), 0x01,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_ad,
            { "A/D",
              "mac-nr.control.sp-csi-rs-cs-im-res-set-act-deact.ad", FT_BOOLEAN, 8, TFS(&activation_deactivation_vals), 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_serving_cell_id,
            { "Serving Cell ID",
              "mac-nr.control.sp-csi-rs-cs-im-res-set-act-deact.serving-cell-id", FT_UINT8, BASE_DEC, NULL, 0x7c,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_bwp_id,
            { "BWP ID",
              "mac-nr.control.sp-csi-rs-cs-im-res-set-act-deact.bwp-id", FT_UINT8, BASE_DEC, NULL, 0x03,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_reserved,
            { "Reserved",
              "mac-nr.control.sp-csi-rs-cs-im-res-set-act-deact.reserved", FT_UINT8, BASE_HEX, NULL, 0xe0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_im,
            { "IM",
              "mac-nr.control.sp-csi-rs-cs-im-res-set-act-deact.im", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x10,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_sp_csi_rs_res_set_id,
            { "SP CSI-RS resource set ID",
              "mac-nr.control.sp-csi-rs-cs-im-res-set-act-deact.sp-csi-rs-res-set-id", FT_UINT8, BASE_DEC, NULL, 0x0f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_reserved2,
            { "Reserved",
              "mac-nr.control.sp-csi-rs-cs-im-res-set-act-deact.reserved", FT_UINT8, BASE_HEX, NULL, 0xf0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_sp_csi_im_res_set_id,
            { "SP CSI-IM resource set ID",
              "mac-nr.control.sp-csi-rs-cs-im-res-set-act-deact.sp-csi-im-res-set-id", FT_UINT8, BASE_DEC, NULL, 0x0f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_reserved3,
            { "Reserved",
              "mac-nr.control.sp-csi-rs-cs-im-res-set-act-deact.reserved", FT_UINT8, BASE_HEX, NULL, 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_sp_csi_rs_csi_im_res_set_act_deact_tci_state_id,
            { "TCI State ID",
              "mac-nr.control.sp-csi-rs-cs-im-res-set-act-deact.tci-state-id", FT_UINT8, BASE_DEC, NULL, 0x7f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_dupl_act_deact_drb7,
            { "DRB 7",
              "mac-nr.control.dupl-act-deact.drb7", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_dupl_act_deact_drb6,
            { "DRB 6",
              "mac-nr.control.dupl-act-deact.drb6", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x40,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_dupl_act_deact_drb5,
            { "DRB 5",
              "mac-nr.control.dupl-act-deact.drb5", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x20,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_dupl_act_deact_drb4,
            { "DRB 4",
              "mac-nr.control.dupl-act-deact.drb4", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x10,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_dupl_act_deact_drb3,
            { "DRB 3",
              "mac-nr.control.dupl-act-deact.drb3", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x08,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_dupl_act_deact_drb2,
            { "DRB 2",
              "mac-nr.control.dupl-act-deact.drb2", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x04,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_dupl_act_deact_drb1,
            { "DRB 1",
              "mac-nr.control.dupl-act-deact.drb1", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x02,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_dupl_act_deact_reserved,
            { "Reserved",
              "mac-nr.control.dupl-act-deact.reserved", FT_UINT8, BASE_HEX, NULL, 0x01,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell7,
            { "Cell 7",
              "mac-nr.control.scell-act-deact.cell7", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell6,
            { "Cell 6",
              "mac-nr.control.scell-act-deact.cell6", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x40,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell5,
            { "Cell 5",
              "mac-nr.control.scell-act-deact.cell5", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x20,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell4,
            { "Cell 4",
              "mac-nr.control.scell-act-deact.cell4", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x10,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell3,
            { "Cell 3",
              "mac-nr.control.scell-act-deact.cell3", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x08,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell2,
            { "Cell 2",
              "mac-nr.control.scell-act-deact.cell2", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x04,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell1,
            { "Cell 1",
              "mac-nr.control.scell-act-deact.cell1", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x02,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_reserved,
            { "Reserved",
              "mac-nr.control.scell-act-deact.reserved", FT_UINT8, BASE_HEX, NULL, 0x01,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell15,
            { "Cell 15",
              "mac-nr.control.scell-act-deact.cell15", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell14,
            { "Cell 14",
              "mac-nr.control.scell-act-deact.cell14", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x40,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell13,
            { "Cell 13",
              "mac-nr.control.scell-act-deact.cell13", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x20,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell12,
            { "Cell 12",
              "mac-nr.control.scell-act-deact.cell12", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x10,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell11,
            { "Cell 11",
              "mac-nr.control.scell-act-deact.cell11", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x08,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell10,
            { "Cell 10",
              "mac-nr.control.scell-act-deact.cell10", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x04,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell9,
            { "Cell 9",
              "mac-nr.control.scell-act-deact.cell9", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x02,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell8,
            { "Cell 8",
              "mac-nr.control.scell-act-deact.cell8", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x01,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell23,
            { "Cell 23",
              "mac-nr.control.scell-act-deact.cell23", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell22,
            { "Cell 22",
              "mac-nr.control.scell-act-deact.cell22", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x40,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell21,
            { "Cell 21",
              "mac-nr.control.scell-act-deact.cell21", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x20,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell20,
            { "Cell 20",
              "mac-nr.control.scell-act-deact.cell20", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x10,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell19,
            { "Cell 19",
              "mac-nr.control.scell-act-deact.cell19", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x08,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell18,
            { "Cell 18",
              "mac-nr.control.scell-act-deact.cell18", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x04,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell17,
            { "Cell 17",
              "mac-nr.control.scell-act-deact.cell17", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x02,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell16,
            { "Cell 16",
              "mac-nr.control.scell-act-deact.cell16", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x01,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell31,
            { "Cell 31",
              "mac-nr.control.scell-act-deact.cell31", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x80,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell30,
            { "Cell 30",
              "mac-nr.control.scell-act-deact.cell30", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x40,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell29,
            { "Cell 29",
              "mac-nr.control.scell-act-deact.cell29", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x20,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell28,
            { "Cell 28",
              "mac-nr.control.scell-act-deact.cell28", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x10,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell27,
            { "Cell 27",
              "mac-nr.control.scell-act-deact.cell27", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x08,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell26,
            { "Cell 26",
              "mac-nr.control.scell-act-deact.cell26", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x04,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell25,
            { "Cell 25",
              "mac-nr.control.scell-act-deact.cell25", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x02,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_scell_act_deact_cell24,
            { "Cell 24",
              "mac-nr.control.scell-act-deact.cell24", FT_BOOLEAN, 8, TFS(&tfs_activated_deactivated), 0x01,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_bsr_short_lcg,
            { "LCG",
              "mac-nr.control.bsr.short.lcg", FT_UINT8, BASE_DEC, NULL, 0xe0,
              "Logical Channel Group", HFILL
            }
        },
        { &hf_mac_nr_control_bsr_short_bs_lcg0,
            { "Buffer Size for LCG0",
              "mac-nr.control.bsr.bs-lcg0", FT_UINT8, BASE_DEC|BASE_EXT_STRING, &buffer_size_5bits_vals_ext, 0x1f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_bsr_short_bs_lcg1,
            { "Buffer Size for LCG1",
              "mac-nr.control.bsr.bs-lcg1", FT_UINT8, BASE_DEC|BASE_EXT_STRING, &buffer_size_5bits_vals_ext, 0x1f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_bsr_short_bs_lcg2,
            { "Buffer Size for LCG2",
              "mac-nr.control.bsr.bs-lcg2", FT_UINT8, BASE_DEC|BASE_EXT_STRING, &buffer_size_5bits_vals_ext, 0x1f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_bsr_short_bs_lcg3,
            { "Buffer Size for LCG3",
              "mac-nr.control.bsr.bs-lcg3", FT_UINT8, BASE_DEC|BASE_EXT_STRING, &buffer_size_5bits_vals_ext, 0x1f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_bsr_short_bs_lcg4,
            { "Buffer Size for LCG4",
              "mac-nr.control.bsr.bs-lcg4", FT_UINT8, BASE_DEC|BASE_EXT_STRING, &buffer_size_5bits_vals_ext, 0x1f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_bsr_short_bs_lcg5,
            { "Buffer Size for LCG5",
              "mac-nr.control.bsr.bs-lcg5", FT_UINT8, BASE_DEC|BASE_EXT_STRING, &buffer_size_5bits_vals_ext, 0x1f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_bsr_short_bs_lcg6,
            { "Buffer Size for LCG6",
              "mac-nr.control.bsr.bs-lcg6", FT_UINT8, BASE_DEC|BASE_EXT_STRING, &buffer_size_5bits_vals_ext, 0x1f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_bsr_short_bs_lcg7,
            { "Buffer Size for LCG7",
              "mac-nr.control.bsr.bs-lcg7", FT_UINT8, BASE_DEC|BASE_EXT_STRING, &buffer_size_5bits_vals_ext, 0x1f,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_bsr_long_lcg7,
            { "LCG7",
              "mac-nr.control.bsr.long.lcg7", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x80,
              "Logical Channel Group 7", HFILL
            }
        },
        { &hf_mac_nr_control_bsr_long_lcg6,
            { "LCG6",
              "mac-nr.control.bsr.long.lcg6", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x40,
              "Logical Channel Group 6", HFILL
            }
        },
        { &hf_mac_nr_control_bsr_long_lcg5,
            { "LCG5",
              "mac-nr.control.bsr.long.lcg5", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x20,
              "Logical Channel Group 5", HFILL
            }
        },
        { &hf_mac_nr_control_bsr_long_lcg4,
            { "LCG4",
              "mac-nr.control.bsr.long.lcg4", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x10,
              "Logical Channel Group 4", HFILL
            }
        },
        { &hf_mac_nr_control_bsr_long_lcg3,
            { "LCG3",
              "mac-nr.control.bsr.long.lcg3", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x08,
              "Logical Channel Group 3", HFILL
            }
        },
        { &hf_mac_nr_control_bsr_long_lcg2,
            { "LCG2",
              "mac-nr.control.bsr.long.lcg2", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x04,
              "Logical Channel Group 2", HFILL
            }
        },
        { &hf_mac_nr_control_bsr_long_lcg1,
            { "LCG1",
              "mac-nr.control.bsr.long.lcg1", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x02,
              "Logical Channel Group 1", HFILL
            }
        },
        { &hf_mac_nr_control_bsr_long_lcg0,
            { "LCG0",
              "mac-nr.control.bsr.long.lcg0", FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x01,
              "Logical Channel Group 0", HFILL
            }
        },
        { &hf_mac_nr_control_bsr_trunc_long_bs,
            { "Buffer Size",
              "mac-nr.control.bsr.trunc-bs", FT_UINT8, BASE_DEC|BASE_EXT_STRING, &buffer_size_8bits_vals_ext, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_bsr_long_bs_lcg7,
            { "Buffer Size for LCG7",
              "mac-nr.control.bsr.bs-lcg7", FT_UINT8, BASE_DEC|BASE_EXT_STRING, &buffer_size_8bits_vals_ext, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_bsr_long_bs_lcg6,
            { "Buffer Size for LCG6",
              "mac-nr.control.bsr.bs-lcg6", FT_UINT8, BASE_DEC|BASE_EXT_STRING, &buffer_size_8bits_vals_ext, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_bsr_long_bs_lcg5,
            { "Buffer Size for LCG5",
              "mac-nr.control.bsr.bs-lcg5", FT_UINT8, BASE_DEC|BASE_EXT_STRING, &buffer_size_8bits_vals_ext, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_bsr_long_bs_lcg4,
            { "Buffer Size for LCG4",
              "mac-nr.control.bsr.bs-lcg4", FT_UINT8, BASE_DEC|BASE_EXT_STRING, &buffer_size_8bits_vals_ext, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_bsr_long_bs_lcg3,
            { "Buffer Size for LCG3",
              "mac-nr.control.bsr.bs-lcg3", FT_UINT8, BASE_DEC|BASE_EXT_STRING, &buffer_size_8bits_vals_ext, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_bsr_long_bs_lcg2,
            { "Buffer Size for LCG2",
              "mac-nr.control.bsr.bs-lcg2", FT_UINT8, BASE_DEC|BASE_EXT_STRING, &buffer_size_8bits_vals_ext, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_bsr_long_bs_lcg1,
            { "Buffer Size for LCG1",
              "mac-nr.control.bsr.bs-lcg1", FT_UINT8, BASE_DEC|BASE_EXT_STRING, &buffer_size_8bits_vals_ext, 0x0,
              NULL, HFILL
            }
        },
        { &hf_mac_nr_control_bsr_long_bs_lcg0,
            { "Buffer Size for LCG0",
              "mac-nr.control.bsr.bs-lcg0", FT_UINT8, BASE_DEC|BASE_EXT_STRING, &buffer_size_8bits_vals_ext, 0x0,
              NULL, HFILL
            }
        },

    };

    static gint *ett[] =
    {
        &ett_mac_nr,
        &ett_mac_nr_context,
        &ett_mac_nr_subheader,
        &ett_mac_nr_rar_subheader,
        &ett_mac_nr_rar_grant,
        &ett_mac_nr_me_phr_entry
    };

    static ei_register_info ei[] = {
        { &ei_mac_nr_no_per_frame_data,                              { "mac-nr.no_per_frame_data", PI_UNDECODED, PI_WARN, "Can't dissect NR MAC frame because no per-frame info was attached!", EXPFILL }},
        { &ei_mac_nr_sdu_length_different_from_dissected,            { "mac-nr.sdu-length-different-from-dissected", PI_UNDECODED, PI_WARN, "Something is wrong with sdu length or dissection is wrong", EXPFILL }},
        { &ei_mac_nr_unknown_udp_framing_tag,                        { "mac-nr.unknown-udp-framing-tag", PI_UNDECODED, PI_WARN, "Unknown UDP framing tag, aborting dissection", EXPFILL }},
        { &ei_mac_nr_dl_sch_control_subheader_after_data_subheader,  { "mac-nr.ulsch.ce-after-data",  PI_SEQUENCE, PI_WARN, "For DL-SCH PDUs, CEs should come before data", EXPFILL }},
        { &ei_mac_nr_ul_sch_control_subheader_before_data_subheader, { "mac-nr.dlsch.ce-before-data", PI_SEQUENCE, PI_WARN, "For UL-SCH PDUs, CEs should come after data", EXPFILL }}
    };

    module_t *mac_nr_module;
    expert_module_t* expert_mac_nr;

    static uat_field_t lcid_drb_mapping_flds[] = {
        UAT_FLD_VS(lcid_drb_mappings, lcid, "LCID (3-32)", drb_lcid_vals, "The MAC LCID"),
        UAT_FLD_DEC(lcid_drb_mappings, drbid,"DRBID id (1-32)", "Identifier of logical data channel"),
        UAT_FLD_VS(lcid_drb_mappings, bearer_type, "RLC Channel Type", rlc_bearer_type_vals, "The MAC LCID"),
        UAT_END_FIELDS
    };

    /* Register protocol. */
    proto_mac_nr = proto_register_protocol("MAC-NR", "MAC-NR", "mac-nr");
    proto_register_field_array(proto_mac_nr, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    expert_mac_nr = expert_register_protocol(proto_mac_nr);
    expert_register_field_array(expert_mac_nr, ei, array_length(ei));

    /* Allow other dissectors to find this one by name. */
    register_dissector("mac-nr", dissect_mac_nr, proto_mac_nr);

    /* Preferences */
    mac_nr_module = prefs_register_protocol(proto_mac_nr, NULL);

    prefs_register_bool_preference(mac_nr_module, "attempt_rrc_decode",
        "Attempt to decode BCCH, PCCH and CCCH data using NR RRC dissector",
        "Attempt to decode BCCH, PCCH and CCCH data using NR RRC dissector",
        &global_mac_nr_attempt_rrc_decode);

    lcid_drb_mappings_uat = uat_new("Static LCID -> drb Table",
                                    sizeof(lcid_drb_mapping_t),
                                    "drb_bearerconfig",
                                    TRUE,
                                    &lcid_drb_mappings,
                                    &num_lcid_drb_mappings,
                                    UAT_AFFECTS_DISSECTION, /* affects dissection of packets, but not set of named fields */
                                    "",  /* TODO: is this ref to help manual? */
                                    lcid_drb_mapping_copy_cb,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    lcid_drb_mapping_flds);

    prefs_register_uat_preference(mac_nr_module,
                                  "drb_table",
                                  "LCID -> DRB Mappings Table",
                                  "A table that maps from configurable lcids -> RLC bearer configs",
                                  lcid_drb_mappings_uat);
}

void proto_reg_handoff_mac_nr(void)
{
    /* Add as a heuristic UDP dissector */
    heur_dissector_add("udp", dissect_mac_nr_heur, "MAC-NR over UDP", "mac_nr_udp", proto_mac_nr, HEURISTIC_DISABLE);

    rlc_nr_handle = find_dissector_add_dependency("rlc-nr", proto_mac_nr);
    nr_rrc_bcch_bch_handle = find_dissector_add_dependency("nr-rrc.bcch.bch", proto_mac_nr);
    nr_rrc_bcch_dl_sch_handle = find_dissector_add_dependency("nr-rrc.bcch.dl.sch", proto_mac_nr);
    nr_rrc_pcch_handle = find_dissector_add_dependency("nr-rrc.pcch", proto_mac_nr);
    nr_rrc_dl_ccch_handle = find_dissector_add_dependency("nr-rrc.dl.ccch", proto_mac_nr);
    nr_rrc_ul_ccch_handle = find_dissector_add_dependency("nr-rrc.ul.ccch", proto_mac_nr);
    nr_rrc_ul_ccch1_handle = find_dissector_add_dependency("nr-rrc.ul.ccch1", proto_mac_nr);
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
