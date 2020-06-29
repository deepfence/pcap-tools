/* packet-pfcp.c
 *
 * Routines for Packet Forwarding Control Protocol (PFCP) dissection
 *
 * Copyright 2017-2018, Anders Broman <anders.broman@ericsson.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Ref 3GPP TS 29.244 V15.3.0 (2018-09-23)
 */
#include "config.h"

#include <epan/packet.h>
#include <epan/conversation.h>
#include <epan/etypes.h>
#include <epan/expert.h>
#include <epan/sminmpec.h>
#include <epan/addr_resolv.h> /* Needed for BASE_ENTERPRISES */
#include "packet-e164.h"
#include "packet-e212.h"
#include "packet-ntp.h"

void proto_register_pfcp(void);
void proto_reg_handoff_pfcp(void);

static dissector_handle_t pfcp_handle;
static dissector_handle_t pfcp_3gpp_ies_handle;

#define UDP_PORT_PFCP  8805
static guint g_pfcp_port = UDP_PORT_PFCP;

static int proto_pfcp = -1;

static int hf_pfcp_msg_type = -1;
static int hf_pfcp_msg_length = -1;
static int hf_pfcp_hdr_flags = -1;
static int hf_pfcp_version = -1;
static int hf_pfcp_mp_flag = -1;
static int hf_pfcp_s_flag = -1;
static int hf_pfcp_seid = -1;
static int hf_pfcp_seqno = -1;
static int hf_pfcp_mp = -1;

static int hf_pfcp2_ie = -1;
static int hf_pfcp2_ie_len = -1;
static int hf_pfcp2_enterprise_ie = -1;
static int hf_pfcp_enterprise_id = -1;
static int hf_pfcp_enterprise_data = -1;

static int hf_pfcp_response_in = -1;
static int hf_pfcp_response_to = -1;
static int hf_pfcp_response_time = -1;

static int hf_pfcp_session = -1;

static int hf_pfcp_spare_b2 = -1;
static int hf_pfcp_spare_b3 = -1;
static int hf_pfcp_spare_b4 = -1;
static int hf_pfcp_spare_b5 = -1;
static int hf_pfcp_spare_b6 = -1;
static int hf_pfcp_spare_b7 = -1;
static int hf_pfcp_spare_b7_b6 = -1;
static int hf_pfcp_spare_b7_b5 = -1;
static int hf_pfcp_spare_b7_b4 = -1;
static int hf_pfcp_spare_b7_b3 = -1;
static int hf_pfcp_spare_b7_b2 = -1;
static int hf_pfcp_spare_b7_b1 = -1;
static int hf_pfcp_spare_h0 = -1;
static int hf_pfcp_spare_h1 = -1;
static int hf_pfcp_spare_oct = -1;
static int hf_pfcp_spare = -1;

static int hf_pfcp2_cause = -1;
static int hf_pfcp_node_id_type = -1;
static int hf_pfcp_node_id_ipv4 = -1;
static int hf_pfcp_node_id_ipv6 = -1;
static int hf_pfcp_node_id_fqdn = -1;
static int hf_pfcp_recovery_time_stamp = -1;
static int hf_pfcp_f_seid_flags = -1;
static int hf_pfcp_b0_v6 = -1;
static int hf_pfcp_b1_v4 = -1;
static int hf_pfcp_f_seid_ipv4 = -1;
static int hf_pfcp_f_seid_ipv6 = -1;
static int hf_pfcp_pdr_id = -1;
static int hf_pfcp_precedence = -1;
static int hf_pfcp_source_interface = -1;
static int hf_pfcp_f_teid_flags = -1;
static int hf_pfcp_fteid_flg_spare = -1;
static int hf_pfcp_fteid_flg_b3_ch_id = -1;
static int hf_pfcp_fteid_flg_b2_ch = -1;
static int hf_pfcp_fteid_flg_b1_v6 = -1;
static int hf_pfcp_fteid_flg_b0_v4 = -1;
static int hf_pfcp_f_teid_ch_id = -1;
static int hf_pfcp_f_teid_teid = -1;
static int hf_pfcp_f_teid_ipv4 = -1;
static int hf_pfcp_f_teid_ipv6 = -1;
static int hf_pfcp_network_instance = -1;
static int hf_pfcp_pdn_type = -1;
static int hf_pfcp_failed_rule_id_type = -1;
static int hf_pfcp_time_qouta_mechanism_bti_type = -1;
static int hf_pfcp_time_qouta_mechanism_bti = -1;
static int hf_pfcp_multiplier_value_digits = -1;
static int hf_pfcp_multiplier_exponent = -1;
static int hf_pfcp_aggregated_urr_id_ie_urr_id = -1;

static int hf_pfcp_ue_ip_address_flags = -1;
static int hf_pfcp_ue_ip_address_flag_b0_v6 = -1;
static int hf_pfcp_ue_ip_address_flag_b1_v4 = -1;
static int hf_pfcp_ue_ip_address_flag_b2_sd = -1;
static int hf_pfcp_ue_ip_address_flag_b3_v6d = -1;
static int hf_pfcp_ue_ip_addr_ipv4 = -1;
static int hf_pfcp_ue_ip_add_ipv6 = -1;
static int hf_pfcp_ue_ip_add_ipv6_prefix = -1;
static int hf_pfcp_application_id = -1;
static int hf_pfcp_application_id_str = -1;

static int hf_pfcp_sdf_filter_flags = -1;
static int hf_pfcp_sdf_filter_flags_b0_fd = -1;
static int hf_pfcp_sdf_filter_flags_b1_ttc = -1;
static int hf_pfcp_sdf_filter_flags_b2_spi = -1;
static int hf_pfcp_sdf_filter_flags_b3_fl = -1;
static int hf_pfcp_sdf_filter_flags_b4_bid = -1;

static int hf_pfcp_flow_desc_len = -1;
static int hf_pfcp_flow_desc = -1;
static int hf_pfcp_traffic_class = -1;
static int hf_pfcp_traffic_mask = -1;
static int hf_pfcp_spi = -1;
static int hf_pfcp_flow_label_spare_bit = -1;
static int hf_pfcp_flow_label = -1;
static int hf_pfcp_sdf_filter_id = -1;

static int hf_pfcp_out_hdr_desc = -1;
static int hf_pfcp_far_id_flg = -1;
static int hf_pfcp_far_id = -1;
static int hf_pfcp_urr_id_flg = -1;
static int hf_pfcp_urr_id = -1;
static int hf_pfcp_qer_id_flg = -1;
static int hf_pfcp_qer_id = -1;
static int hf_pfcp_predef_rules_name = -1;

static int hf_pfcp_apply_action_flags = -1;
static int hf_pfcp_apply_action_flags_b4_dupl = -1;
static int hf_pfcp_apply_action_flags_b3_nocp = -1;
static int hf_pfcp_apply_action_flags_b2_buff = -1;
static int hf_pfcp_apply_action_flags_b1_forw = -1;
static int hf_pfcp_apply_action_flags_b0_drop = -1;

static int hf_pfcp_bar_id = -1;
static int hf_pfcp_fq_csid_node_id_type = -1;
static int hf_pfcp_num_csid = -1;
static int hf_pfcp_fq_csid_node_id_ipv4 = -1;
static int hf_pfcp_fq_csid_node_id_ipv6 = -1;
static int hf_pfcp_fq_csid_node_id_mcc_mnc = -1;
static int hf_pfcp_fq_csid_node_id_int = -1;
static int hf_pfcp_fq_csid = -1;
static int hf_pfcp_measurement_period = -1;
static int hf_pfcp_duration_measurement = -1;
static int hf_pfcp_time_of_first_packet = -1;
static int hf_pfcp_time_of_last_packet = -1;
static int hf_pfcp_dst_interface = -1;
static int hf_pfcp_redirect_address_type = -1;
static int hf_pfcp_redirect_server_addr_len = -1;
static int hf_pfcp_redirect_server_address = -1;
static int hf_pfcp_linked_urr_id = -1;
static int hf_pfcp_outer_hdr_desc = -1;
static int hf_pfcp_outer_hdr_creation_teid = -1;
static int hf_pfcp_outer_hdr_creation_ipv4 = -1;
static int hf_pfcp_outer_hdr_creation_ipv6 = -1;
static int hf_pfcp_outer_hdr_creation_port = -1;
static int hf_pfcp_time_threshold = -1;
static int hf_pfcp_forwarding_policy_id_len = -1;
static int hf_pfcp_forwarding_policy_id = -1;

static int hf_pfcp_measurement_method_flags = -1;
static int hf_pfcp_measurement_method_flags_b0_durat = -1;
static int hf_pfcp_measurement_method_flags_b1_volume = -1;
static int hf_pfcp_measurement_method_flags_b2_event = -1;

static int hf_pfcp_subsequent_time_threshold = -1;
static int hf_pfcp_inactivity_detection_time = -1;
static int hf_pfcp_monitoring_time = -1;

static int hf_pfcp_reporting_triggers_o5_b7_liusa = -1;
static int hf_pfcp_reporting_triggers_o5_b6_droth = -1;
static int hf_pfcp_reporting_triggers_o5_b5_stopt = -1;
static int hf_pfcp_reporting_triggers_o5_b4_start = -1;
static int hf_pfcp_reporting_triggers_o5_b3_quhti = -1;
static int hf_pfcp_reporting_triggers_o5_b2_timth = -1;
static int hf_pfcp_reporting_triggers_o5_b1_volth = -1;
static int hf_pfcp_reporting_triggers_o5_b0_perio = -1;
static int hf_pfcp_reporting_triggers_o6_b5_evequ = -1;
static int hf_pfcp_reporting_triggers_o6_b4_eveth = -1;
static int hf_pfcp_reporting_triggers_o6_b3_macar = -1;
static int hf_pfcp_reporting_triggers_o6_b2_envcl = -1;
static int hf_pfcp_reporting_triggers_o6_b1_timqu = -1;
static int hf_pfcp_reporting_triggers_o6_b0_volqu = -1;

static int hf_pfcp_volume_threshold = -1;
static int hf_pfcp_volume_threshold_b2_dlvol = -1;
static int hf_pfcp_volume_threshold_b1_ulvol = -1;
static int hf_pfcp_volume_threshold_b0_tovol = -1;
static int hf_pfcp_volume_threshold_tovol = -1;
static int hf_pfcp_volume_threshold_ulvol = -1;
static int hf_pfcp_volume_threshold_dlvol = -1;

static int hf_pfcp_volume_quota = -1;
static int hf_pfcp_volume_quota_b2_dlvol = -1;
static int hf_pfcp_volume_quota_b1_ulvol = -1;
static int hf_pfcp_volume_quota_b0_tovol = -1;
static int hf_pfcp_volume_quota_tovol = -1;
static int hf_pfcp_volume_quota_ulvol = -1;
static int hf_pfcp_volume_quota_dlvol = -1;

static int hf_pfcp_subseq_volume_threshold = -1;
static int hf_pfcp_subseq_volume_threshold_b2_dlvol = -1;
static int hf_pfcp_subseq_volume_threshold_b1_ulvol = -1;
static int hf_pfcp_subseq_volume_threshold_b0_tovol = -1;
static int hf_pfcp_subseq_volume_threshold_tovol = -1;
static int hf_pfcp_subseq_volume_threshold_ulvol = -1;
static int hf_pfcp_subseq_volume_threshold_dlvol = -1;

static int hf_pfcp_time_quota = -1;
static int hf_pfcp_start_time = -1;
static int hf_pfcp_end_time = -1;
static int hf_pfcp_quota_holding_time = -1;
static int hf_pfcp_dropped_dl_traffic_threshold = -1;
static int hf_pfcp_dropped_dl_traffic_threshold_b1_dlby = -1;
static int hf_pfcp_dropped_dl_traffic_threshold_b0_dlpa = -1;
static int hf_pfcp_downlink_packets = -1;
static int hf_pfcp_bytes_downlink_data = -1;
static int hf_pfcp_qer_correlation_id = -1;
static int hf_pfcp_gate_status = -1;
static int hf_pfcp_gate_status_b0b1_dlgate = -1;
static int hf_pfcp_gate_status_b3b2_ulgate = -1;
static int hf_pfcp_ul_mbr = -1;
static int hf_pfcp_dl_mbr = -1;
static int hf_pfcp_ul_gbr = -1;
static int hf_pfcp_dl_gbr = -1;

static int hf_pfcp_report_type = -1;
static int hf_pfcp_report_type_b3_upir = -1;
static int hf_pfcp_report_type_b2_erir = -1;
static int hf_pfcp_report_type_b1_usar = -1;
static int hf_pfcp_report_type_b0_dldr = -1;

static int hf_pfcp_offending_ie = -1;

static int hf_pfcp_up_function_features_o6_b5_frrt = -1;
static int hf_pfcp_up_function_features_o6_b4_trace = -1;
static int hf_pfcp_up_function_features_o6_b3_quoac = -1;
static int hf_pfcp_up_function_features_o6_b2_udbc = -1;
static int hf_pfcp_up_function_features_o6_b1_pdiu = -1;
static int hf_pfcp_up_function_features_o6_b0_empu = -1;
static int hf_pfcp_up_function_features_o5_b7_treu = -1;
static int hf_pfcp_up_function_features_o5_b6_heeu = -1;
static int hf_pfcp_up_function_features_o5_b5_pfdm = -1;
static int hf_pfcp_up_function_features_o5_b4_ftup = -1;
static int hf_pfcp_up_function_features_o5_b3_trst = -1;
static int hf_pfcp_up_function_features_o5_b2_dlbd = -1;
static int hf_pfcp_up_function_features_o5_b1_ddnd = -1;
static int hf_pfcp_up_function_features_o5_b0_bucp = -1;

static int hf_pfcp_sequence_number = -1;
static int hf_pfcp_metric = -1;
static int hf_pfcp_timer_unit = -1;
static int hf_pfcp_timer_value = -1;

static int hf_pfcp_usage_report_trigger_o5_b7_immer = -1;
static int hf_pfcp_usage_report_trigger_o5_b6_droth = -1;
static int hf_pfcp_usage_report_trigger_o5_b5_stopt = -1;
static int hf_pfcp_usage_report_trigger_o5_b4_start = -1;
static int hf_pfcp_usage_report_trigger_o5_b3_quhti = -1;
static int hf_pfcp_usage_report_trigger_o5_b2_timth = -1;
static int hf_pfcp_usage_report_trigger_o5_b1_volth = -1;
static int hf_pfcp_usage_report_trigger_o5_b0_perio = -1;
static int hf_pfcp_usage_report_trigger_o6_b7_eveth = -1;
static int hf_pfcp_usage_report_trigger_o6_b6_macar = -1;
static int hf_pfcp_usage_report_trigger_o6_b5_envcl = -1;
static int hf_pfcp_usage_report_trigger_o6_b4_monit = -1;
static int hf_pfcp_usage_report_trigger_o6_b3_termr = -1;
static int hf_pfcp_usage_report_trigger_o6_b2_liusa = -1;
static int hf_pfcp_usage_report_trigger_o6_b1_timqu = -1;
static int hf_pfcp_usage_report_trigger_o6_b0_volqu = -1;
static int hf_pfcp_usage_report_trigger_o7_b0_evequ = -1;

static int hf_pfcp_volume_measurement = -1;
static int hf_pfcp_volume_measurement_b2_dlvol = -1;
static int hf_pfcp_volume_measurement_b1_ulvol = -1;
static int hf_pfcp_volume_measurement_b0_tovol = -1;
static int hf_pfcp_vol_meas_tovol = -1;
static int hf_pfcp_vol_meas_ulvol = -1;
static int hf_pfcp_vol_meas_dlvol = -1;

static int hf_pfcp_cp_function_features = -1;
static int hf_pfcp_cp_function_features_b0_load = -1;
static int hf_pfcp_cp_function_features_b1_ovrl = -1;

static int hf_pfcp_usage_information = -1;
static int hf_pfcp_usage_information_b3_ube = -1;
static int hf_pfcp_usage_information_b2_uae = -1;
static int hf_pfcp_usage_information_b1_aft = -1;
static int hf_pfcp_usage_information_b0_bef = -1;

static int hf_pfcp_application_instance_id = -1;
static int hf_pfcp_application_instance_id_str = -1;
static int hf_pfcp_flow_dir = -1;
static int hf_pfcp_packet_rate = -1;
static int hf_pfcp_packet_rate_b0_ulpr = -1;
static int hf_pfcp_packet_rate_b1_dlpr = -1;
static int hf_pfcp_ul_time_unit = -1;
static int hf_pfcp_max_ul_pr = -1;
static int hf_pfcp_dl_time_unit = -1;
static int hf_pfcp_max_dl_pr = -1;

static int hf_pfcp_dl_flow_level_marking = -1;
static int hf_pfcp_dl_flow_level_marking_b0_ttc = -1;
static int hf_pfcp_dl_flow_level_marking_b1_sci = -1;

static int hf_pfcp_sci = -1;
static int hf_pfcp_dl_data_notification_delay = -1;
static int hf_pfcp_packet_count = -1;
static int hf_pfcp_dl_data_service_inf_flags = -1;
static int hf_pfcp_dl_data_service_inf_b0_ppi = -1;
static int hf_pfcp_dl_data_service_inf_b1_qfii = -1;
static int hf_pfcp_ppi = -1;

static int hf_pfcp_pfcpsmreq_flags = -1;
static int hf_pfcp_pfcpsmreq_flags_b0_drobu = -1;
static int hf_pfcp_pfcpsmreq_flags_b1_sndem = -1;
static int hf_pfcp_pfcpsmreq_flags_b2_qaurr = -1;

static int hf_pfcp_pfcpsrrsp_flags = -1;
static int hf_pfcp_pfcpsrrsp_flags_b0_drobu = -1;

static int hf_pfcp_pfd_contents_flags = -1;
static int hf_pfcp_pfd_contents_flags_b3_cp = -1;
static int hf_pfcp_pfd_contents_flags_b2_dn = -1;
static int hf_pfcp_pfd_contents_flags_b1_url = -1;
static int hf_pfcp_pfd_contents_flags_b0_fd = -1;

static int hf_pfcp_url_len = -1;
static int hf_pfcp_url = -1;
static int hf_pfcp_dn_len = -1;
static int hf_pfcp_dn = -1;
static int hf_pfcp_cp_len = -1;
static int hf_pfcp_cp = -1;
static int hf_pfcp_header_type = -1;
static int hf_pfcp_hf_len = -1;
static int hf_pfcp_hf_name = -1;
static int hf_pfcp_hf_name_str = -1;
static int hf_pfcp_hf_val_len = -1;
static int hf_pfcp_hf_val = -1;
static int hf_pfcp_hf_val_str = -1;

static int hf_pfcp_measurement_info = -1;
static int hf_pfcp_measurement_info_b0_mbqe = -1;
static int hf_pfcp_measurement_info_b1_inam = -1;
static int hf_pfcp_measurement_info_b2_radi = -1;
static int hf_pfcp_measurement_info_b3_istm = -1;

static int hf_pfcp_node_report_type = -1;
static int hf_pfcp_node_report_type_b0_upfr = -1;

static int hf_pfcp_remote_gtp_u_peer_flags = -1;
static int hf_pfcp_remote_gtp_u_peer_flags_b0_v6 = -1;
static int hf_pfcp_remote_gtp_u_peer_flags_b1_v4 = -1;
static int hf_pfcp_remote_gtp_u_peer_ipv4 = -1;
static int hf_pfcp_remote_gtp_u_peer_ipv6 = -1;
static int hf_pfcp_ur_seqn = -1;

static int hf_pfcp_oci_flags = -1;
static int hf_pfcp_oci_flags_b0_aoci = -1;

static int hf_pfcp_pfcp_assoc_rel_req_flags = -1;
static int hf_pfcp_pfcp_assoc_rel_req_b0_sarr = -1;

static int hf_pfcp_upiri_flags = -1;
static int hf_pfcp_upiri_flags_b0_v4 = -1;
static int hf_pfcp_upiri_flags_b1_v6 = -1;
static int hf_pfcp_upiri_flg_b6_assosi = -1;
static int hf_pfcp_upiri_flg_b5_assoni = -1;
static int hf_pfcp_upiri_flg_b2b4_teidri = -1;
static int hf_pfcp_upiri_teidri = -1;
static int hf_pfcp_upiri_teid_range = -1;
static int hf_pfcp_upiri_ipv4 = -1;
static int hf_pfcp_upiri_ipv6 = -1;

static int hf_pfcp_user_plane_inactivity_timer = -1;

static int hf_pfcp_subsequent_volume_quota = -1;
static int hf_pfcp_subsequent_volume_quota_b2_dlvol = -1;
static int hf_pfcp_subsequent_volume_quota_b1_ulvol = -1;
static int hf_pfcp_subsequent_volume_quota_b0_tovol = -1;
static int hf_pfcp_subsequent_volume_quota_tovol = -1;
static int hf_pfcp_subsequent_volume_quota_ulvol = -1;
static int hf_pfcp_subsequent_volume_quota_dlvol = -1;

static int hf_pfcp_subsequent_time_quota = -1;

static int hf_pfcp_rqi_flag = -1;
static int hf_pfcp_qfi = -1;
static int hf_pfcp_query_urr_reference = -1;
static int hf_pfcp_additional_usage_reports_information = -1;
static int hf_pfcp_additional_usage_reports_information_b14_b0_number_value = -1;
static int hf_pfcp_additional_usage_reports_information_b15_auri = -1;
static int hf_pfcp_traffic_endpoint_id = -1;

static int hf_pfcp_mac_address_flags = -1;
static int hf_pfcp_mac_address_flags_b3_udes = -1;
static int hf_pfcp_mac_address_flags_b2_usou = -1;
static int hf_pfcp_mac_address_flags_b1_dest = -1;
static int hf_pfcp_mac_address_flags_b0_sour = -1;
static int hf_pfcp_mac_address_upper_dest_mac_address = -1;
static int hf_pfcp_mac_address_upper_source_mac_address = -1;
static int hf_pfcp_mac_address_dest_mac_address = -1;
static int hf_pfcp_mac_address_source_mac_address = -1;

static int hf_pfcp_c_tag_flags = -1;
static int hf_pfcp_c_tag_flags_b2_vid = -1;
static int hf_pfcp_c_tag_flags_b1_dei = -1;
static int hf_pfcp_c_tag_flags_b0_pcp = -1;
static int hf_pfcp_c_tag_cvid = -1;
static int hf_pfcp_c_tag_dei_flag = -1;
static int hf_pfcp_c_tag_pcp_value = -1;
static int hf_pfcp_c_tag_cvid_value = -1;

static int hf_pfcp_s_tag_flags = -1;
static int hf_pfcp_s_tag_flags_b2_vid = -1;
static int hf_pfcp_s_tag_flags_b1_dei = -1;
static int hf_pfcp_s_tag_flags_b0_pcp = -1;
static int hf_pfcp_s_tag_svid = -1;
static int hf_pfcp_s_tag_dei_flag = -1;
static int hf_pfcp_s_tag_pcp_value = -1;
static int hf_pfcp_s_tag_svid_value = -1;

static int hf_pfcp_ethertype = -1;

static int hf_pfcp_proxying_flags = -1;
static int hf_pfcp_proxying_flags_b1_ins = -1;
static int hf_pfcp_proxying_flags_b0_arp = -1;

static int hf_pfcp_ethertype_filter_id = -1;

static int hf_pfcp_ethertype_filter_properties_flags = -1;
static int hf_pfcp_ethertype_filter_properties_flags_b0_bide = -1;

static int hf_pfcp_suggested_buffering_packets_count_packet_count = -1;

static int hf_pfcp_user_id_flags = -1;
static int hf_pfcp_user_id_flags_b3_naif = -1;
static int hf_pfcp_user_id_flags_b2_msisdnf = -1;
static int hf_pfcp_user_id_flags_b1_imeif = -1;
static int hf_pfcp_user_id_flags_b0_imsif = -1;
static int hf_pfcp_user_id_length_of_imsi = -1;
static int hf_pfcp_user_id_length_of_imei = -1;
static int hf_pfcp_user_id_imei = -1;
static int hf_pfcp_user_id_length_of_msisdn = -1;
static int hf_pfcp_user_id_length_of_nai = -1;
static int hf_pfcp_user_id_nai = -1;

static int hf_pfcp_ethernet_pdu_session_information_flags = -1;
static int hf_pfcp_ethernet_pdu_session_information_flags_b0_ethi = -1;

static int hf_pfcp_mac_addresses_detected_number_of_mac_addresses = -1;
static int hf_pfcp_mac_addresses_detected_mac_address = -1;

static int hf_pfcp_mac_addresses_removed_number_of_mac_addresses = -1;
static int hf_pfcp_mac_addresses_removed_mac_address = -1;

static int hf_pfcp_ethernet_inactivity_timer = -1;

static int hf_pfcp_subsequent_event_quota = -1;

static int hf_pfcp_subsequent_event_threshold = -1;

static int hf_pfcp_trace_information_trace_id = -1;
static int hf_pfcp_trace_information_length_trigger_events = -1;
static int hf_pfcp_trace_information_trigger_events = -1;
static int hf_pfcp_trace_information_session_trace_depth = -1;
static int hf_pfcp_trace_information_length_list_interfaces = -1;
static int hf_pfcp_trace_information_list_interfaces = -1;
static int hf_pfcp_trace_information_length_ipaddress = -1;
static int hf_pfcp_trace_information_ipaddress = -1;

static int hf_pfcp_frame_route = -1;
static int hf_pfcp_frame_routing = -1;
static int hf_pfcp_frame_ipv6_route = -1;

static int hf_pfcp_event_quota = -1;

static int hf_pfcp_event_threshold = -1;

static int hf_pfcp_event_time_stamp = -1;

static int hf_pfcp_averaging_window = -1;

static int hf_pfcp_paging_policy_indicator = -1;

static int ett_pfcp = -1;
static int ett_pfcp_flags = -1;
static int ett_pfcp_ie = -1;
static int ett_pfcp_grouped_ie = -1;
static int ett_pfcp_f_seid_flags = -1;
static int ett_f_teid_flags = -1;
static int ett_pfcp_ue_ip_address_flags = -1;
static int ett_pfcp_sdf_filter_flags = -1;
static int ett_pfcp_apply_action_flags = -1;
static int ett_pfcp_measurement_method_flags = -1;
static int ett_pfcp_reporting_triggers = -1;
static int ett_pfcp_volume_threshold = -1;
static int ett_pfcp_volume_quota = -1;
static int ett_pfcp_subseq_volume_threshold = -1;
static int ett_pfcp_dropped_dl_traffic_threshold = -1;
static int ett_pfcp_gate_status = -1;
static int ett_pfcp_report_type = -1;
static int ett_pfcp_up_function_features = -1;
static int ett_pfcp_report_trigger = -1;
static int ett_pfcp_volume_measurement = -1;
static int ett_pfcp_cp_function_features = -1;
static int ett_pfcp_usage_information = -1;
static int ett_pfcp_packet_rate = -1;
static int ett_pfcp_pfcp_dl_flow_level_marking = -1;
static int ett_pfcp_dl_data_service_inf = -1;
static int ett_pfcp_pfcpsmreq = -1;
static int ett_pfcp_pfcpsrrsp = -1;
static int ett_pfcp_measurement_info = -1;
static int ett_pfcp_node_report_type = -1;
static int ett_pfcp_remote_gtp_u_peer = -1;
static int ett_pfcp_oci_flags = -1;
static int ett_pfcp_assoc_rel_req_flags = -1;
static int ett_pfcp_upiri_flags = -1;
static int ett_pfcp_flow_desc = -1;
static int ett_pfcp_tos = -1;
static int ett_pfcp_spi = -1;
static int ett_pfcp_flow_label = -1;
static int ett_pfcp_sdf_filter_id = -1;
static int ett_pfcp_subsequent_volume_quota = -1;
static int ett_pfcp_additional_usage_reports_information = -1;
static int ett_pfcp_mac_address = -1;
static int ett_pfcp_c_tag = -1;
static int ett_pfcp_c_tag_dei = -1;
static int ett_pfcp_s_tag = -1;
static int ett_pfcp_s_tag_dei = -1;
static int ett_pfcp_proxying = -1;
static int ett_pfcp_ethernet_filter_properties = -1;
static int ett_pfcp_user_id = -1;
static int ett_pfcp_ethernet_pdu_session_information = -1;


static expert_field ei_pfcp_ie_reserved = EI_INIT;
static expert_field ei_pfcp_ie_data_not_decoded = EI_INIT;
static expert_field ei_pfcp_ie_not_decoded_null = EI_INIT;
static expert_field ei_pfcp_ie_not_decoded_to_large = EI_INIT;
static expert_field ei_pfcp_enterprise_ie_3gpp = EI_INIT;
static expert_field ei_pfcp_ie_encoding_error = EI_INIT;


static gboolean g_pfcp_session = FALSE;
static guint32 pfcp_session_count;
typedef struct pfcp_session_args {
    wmem_list_t *seid_list;
    wmem_list_t *ip_list;
    guint64 last_seid;
    address last_ip;
    guint8 last_cause;
} pfcp_session_args_t;

typedef struct _pfcp_hdr {
    guint8 message; /* Message type */
    guint16 length; /* Length of header */
    gint64 seid;    /* Tunnel End-point ID */
} pfcp_hdr_t;

/* Relation between frame -> session */
GHashTable* pfcp_session_table;
/* Relation between <seid,ip> -> frame */
wmem_tree_t* pfcp_frame_tree;

typedef struct pfcp_info {
    guint64 seid;
    guint32 frame;
} pfcp_info_t;


static dissector_table_t pfcp_enterprise_ies_dissector_table;

static void dissect_pfcp_ies_common(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, gint offset, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_create_pdr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_pdi(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_create_far(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_forwarding_parameters(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_duplicating_parameters(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_create_urr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_create_qer(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_created_pdr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_update_pdr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_update_far(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_upd_forwarding_param(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_update_bar(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_update_urr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_update_qer(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_remove_pdr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_remove_far(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_remove_urr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_remove_qer(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_load_control_information(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_overload_control_information(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_application_ids_pfds(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_pfd_context(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_application_detection_inf(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_pfcp_query_urr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_usage_report_smr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_usage_report_sdr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_usage_report_srr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_downlink_data_report(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_create_bar(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_update_bar_smr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_remove_bar(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_error_indication_report(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_user_plane_path_failure_report(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_update_duplicating_parameters(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_aggregated_urrs(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_create_traffic_endpoint(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_created_traffic_endpoint(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_update_traffic_endpoint(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_remove_traffic_endpoint(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_ethernet_packet_filter(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_ethernet_traffic_information(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);
static void dissect_pfcp_additional_monitoring_time(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args _U_);

static const true_false_string pfcp_id_predef_dynamic_tfs = {
    "Predefined by UP",
    "Dynamic by CP",
};

#define PFCP_MSG_RESERVED_0                                 0

#define PFCP_MSG_HEARTBEAT_REQUEST                          1
#define PFCP_MSG_HEARTBEAT_RESPONSE                         2
#define PFCP_MSG_PFD_MANAGEMENT_REQUEST                     3
#define PFCP_MSG_PFD_MANAGEMENT_RESPONSE                    4
#define PFCP_MSG_ASSOCIATION_SETUP_REQUEST                  5
#define PFCP_MSG_ASSOCIATION_SETUP_RESPONSE                 6
#define PFCP_MSG_ASSOCIATION_UPDATE_REQUEST                 7
#define PFCP_MSG_ASSOCIATION_UPDATE_RESPONSE                8
#define PFCP_MSG_ASSOCIATION_RELEASE_REQUEST                9
#define PFCP_MSG_ASSOCIATION_RELEASE_RESPONSE               10
#define PFCP_MSG_VERSION_NOT_SUPPORTED_RESPONSE             11
#define PFCP_MSG_NODE_REPORT_REQEUST                        12
#define PFCP_MSG_NODE_REPORT_RERESPONSE                     13
#define PFCP_MSG_SESSION_SET_DELETION_REQUEST               14
#define PFCP_MSG_SESSION_SET_DELETION_RESPONSE              15
#define PFCP_MSG_SESSION_ESTABLISHMENT_REQUEST              50
#define PFCP_MSG_SESSION_ESTABLISHMENT_RESPONSE             51
#define PFCP_MSG_SESSION_MODIFICATION_REQUEST               52
#define PFCP_MSG_SESSION_MODIFICATION_RESPONSE              53
#define PFCP_MSG_SESSION_DELETION_REQUEST                   54
#define PFCP_MSG_SESSION_DELETION_RESPONSE                  55
#define PFCP_MSG_SESSION_REPORT_REQUEST                     56
#define PFCP_MSG_SESSION_REPORT_RESPONSE                    57

static const value_string pfcp_message_type[] = {
    {PFCP_MSG_RESERVED_0,             "Reserved"},
    /* PFCP Node related messages */

    { PFCP_MSG_HEARTBEAT_REQUEST, "PFCP Heartbeat Request"},
    { PFCP_MSG_HEARTBEAT_RESPONSE, "PFCP Heartbeat Response"},
    { PFCP_MSG_PFD_MANAGEMENT_REQUEST, "PFCP PFD Management Request"},
    { PFCP_MSG_PFD_MANAGEMENT_RESPONSE, "PFCP PFD Management Response"},
    { PFCP_MSG_ASSOCIATION_SETUP_REQUEST, "PFCP Association Setup Request"},
    { PFCP_MSG_ASSOCIATION_SETUP_RESPONSE, "PFCP Association Setup Response"},
    { PFCP_MSG_ASSOCIATION_UPDATE_REQUEST, "PFCP Association Update Request"},
    { PFCP_MSG_ASSOCIATION_UPDATE_RESPONSE, "PFCP Association Update Response"},
    { PFCP_MSG_ASSOCIATION_RELEASE_REQUEST, "PFCP Association Release Request"},
    { PFCP_MSG_ASSOCIATION_RELEASE_RESPONSE, "PFCP Association Release Response"},
    { PFCP_MSG_VERSION_NOT_SUPPORTED_RESPONSE, "PFCP Version Not Supported Response"},
    { PFCP_MSG_NODE_REPORT_REQEUST, "PFCP Node Report Request"},
    { PFCP_MSG_NODE_REPORT_RERESPONSE, "PFCP Node Report Response"},
    { PFCP_MSG_SESSION_SET_DELETION_REQUEST, "PFCP Session Set Deletion Request"},
    { PFCP_MSG_SESSION_SET_DELETION_RESPONSE, "PFCP Session Set Deletion Response"},
    //16 to 49    For future use
    //PFCP Session related messages
    { PFCP_MSG_SESSION_ESTABLISHMENT_REQUEST, "PFCP Session Establishment Request"},
    { PFCP_MSG_SESSION_ESTABLISHMENT_RESPONSE, "PFCP Session Establishment Response"},
    { PFCP_MSG_SESSION_MODIFICATION_REQUEST, "PFCP Session Modification Request"},
    { PFCP_MSG_SESSION_MODIFICATION_RESPONSE, "PFCP Session Modification Response"},
    { PFCP_MSG_SESSION_DELETION_REQUEST, "PFCP Session Deletion Request"},
    { PFCP_MSG_SESSION_DELETION_RESPONSE, "PFCP Session Deletion Response"},
    { PFCP_MSG_SESSION_REPORT_REQUEST, "PFCP Session Report Request"},
    { PFCP_MSG_SESSION_REPORT_RESPONSE, "PFCP Session Report Response"},
    //58 to 99    For future use
    //Other messages
    //100 to 255     For future use
    {0, NULL}
};
static value_string_ext pfcp_message_type_ext = VALUE_STRING_EXT_INIT(pfcp_message_type);

/* 8.1.2    Information Element Types */
#define PFCP_IE_ID_CREATE_PDR                   1
#define PFCP_IE_ID_PDI                          2
#define PFCP_IE_CREATE_FAR                      3
#define PFCP_IE_FORWARDING_PARAMETERS           4
#define PFCP_IE_DUPLICATING_PARAMETERS          5
#define PFCP_IE_CREATE_URR                      6
#define PFCP_IE_CREATE_QER                      7
#define PFCP_IE_CREATED_PDR                     8
#define PFCP_IE_UPDATE_PDR                      9
#define PFCP_IE_UPDATE_FAR                     10
#define PFCP_IE_UPD_FORWARDING_PARAM   11
#define PFCP_IE_UPDATE_BAR                     12
#define PFCP_IE_UPDATE_URR                     13
#define PFCP_IE_UPDATE_QER                     14
#define PFCP_IE_REMOVE_PDR                     15
#define PFCP_IE_REMOVE_FAR                     16
#define PFCP_IE_REMOVE_URR                     17
#define PFCP_IE_REMOVE_QER                     18

#define PFCP_IE_LOAD_CONTROL_INFORMATION          51
#define PFCP_IE_OVERLOAD_CONTROL_INFORMATION      54
#define PFCP_IE_APPLICATION_IDS_PFDS              58
#define PFCP_IE_PFD_CONTEXT                       59
#define PFCP_IE_APPLICATION_DETECTION_INF         68
#define PFCP_IE_QUERY_URR                         77
#define PFCP_IE_USAGE_REPORT_SMR                  78
#define PFCP_IE_USAGE_REPORT_SDR                  79
#define PFCP_IE_USAGE_REPORT_SRR                  80
#define PFCP_IE_DOWNLINK_DATA_REPORT              83
#define PFCP_IE_CREATE_BAR                        85
#define PFCP_IE_UPDATE_BAR_SMR                    86
#define PFCP_IE_REMOVE_BAR                        87
#define PFCP_IE_ERROR_INDICATION_REPORT           99
#define PFCP_IE_USER_PLANE_PATH_FAILURE_REPORT   102
#define PFCP_IE_UPDATE_DUPLICATING_PARAMETERS    105
#define PFCP_IE_AGGREGATED_URRS                  118
#define PFCP_IE_CREATE_TRAFFIC_ENDPOINT          127
#define PFCP_IE_CREATED_TRAFFIC_ENDPOINT         128
#define PFCP_IE_UPDATE_TRAFFIC_ENDPOINT          129
#define PFCP_IE_REMOVE_TRAFFIC_ENDPOINT          130
#define PFCP_IE_ETHERNET_PACKET_FILTER           132
#define PFCP_IE_ETHERNET_TRAFFIC_INFORMATION     143
#define PFCP_IE_ADDITIONAL_MONITORING_TIME       147
#define PFCP_IE_EVENT_INFORMATION                148
#define PFCP_IE_EVENT_REPORTING                  149

static const value_string pfcp_ie_type[] = {

    { 0, "Reserved"},
    { 1, "Create PDR"},                                             /* Extendable / Table 7.5.2.2-1 */
    { 2, "PDI"},                                                    /* Extendable / Table 7.5.2.2-2 */
    { 3, "Create FAR"},                                             /* Extendable / Table 7.5.2.3-1 */
    { 4, "Forwarding Parameters"},                                  /* Extendable / Table 7.5.2.3-2 */
    { 5, "Duplicating Parameters"},                                 /* Extendable / Table 7.5.2.3-3 */
    { 6, "Create URR"},                                             /* Extendable / Table 7.5.2.4-1 */
    { 7, "Create QER"},                                             /* Extendable / Table 7.5.2.5-1 */
    { 8, "Created PDR"},                                            /* Extendable / Table 7.5.3.2-1 */
    { 9, "Update PDR" },                                            /* Extendable / Table 7.5.4.2-1 */
    { 10, "Update FAR" },                                           /* Extendable / Table 7.5.4.3-1 */
    { 11, "Update Forwarding Parameters" },                         /* Extendable / Table 7.5.4.3-2 */
    { 12, "Update BAR (PFCP Session Report Response)" },              /* Extendable / Table 7.5.9.2-1 */
    { 13, "Update URR" },                                           /* Extendable / Table 7.5.4.4 */
    { 14, "Update QER" },                                           /* Extendable / Table 7.5.4.5 */
    { 15, "Remove PDR" },                                           /* Extendable / Table 7.5.4.6 */
    { 16, "Remove FAR" },                                           /* Extendable / Table 7.5.4.7 */
    { 17, "Remove URR" },                                           /* Extendable / Table 7.5.4.8 */
    { 18, "Remove QER" },                                           /* Extendable / Table 7.5.4.9 */
    { 19, "Cause" },                                                /* Fixed / Subclause 8.2.1 */
    { 20, "Source Interface" },                                     /* Extendable / Subclause 8.2.2 */
    { 21, "F-TEID" },                                               /* Extendable / Subclause 8.2.3 */
    { 22, "Network Instance" },                                     /* Variable Length / Subclause 8.2.4 */
    { 23, "SDF Filter" },                                           /* Extendable / Subclause 8.2.5 */
    { 24, "Application ID" },                                       /* Variable Length / Subclause 8.2.6 */
    { 25, "Gate Status" },                                          /* Extendable / Subclause 8.2.7 */
    { 26, "MBR" },                                                  /* Extendable / Subclause 8.2.8 */
    { 27, "GBR" },                                                  /* Extendable / Subclause 8.2.9 */
    { 28, "QER Correlation ID" },                                   /* Extendable / Subclause 8.2.10 */
    { 29, "Precedence" },                                           /* Extendable / Subclause 8.2.11 */
    { 30, "DL Transport Level Marking" },                           /* Extendable / Subclause 8.2.12 */
    { 31, "Volume Threshold" },                                     /* Extendable /Subclause 8.2.13 */
    { 32, "Time Threshold" },                                       /* Extendable /Subclause 8.2.14 */
    { 33, "Monitoring Time" },                                      /* Extendable /Subclause 8.2.15 */
    { 34, "Subsequent Volume Threshold" },                          /* Extendable /Subclause 8.2.16 */
    { 35, "Subsequent Time Threshold" },                            /* Extendable /Subclause 8.2.17 */
    { 36, "Inactivity Detection Time" },                            /* Extendable /Subclause 8.2.18 */
    { 37, "Reporting Triggers" },                                   /* Extendable /Subclause 8.2.19 */
    { 38, "Redirect Information" },                                 /* Extendable /Subclause 8.2.20 */
    { 39, "Report Type" },                                          /* Extendable / Subclause 8.2.21 */
    { 40, "Offending IE" },                                         /* Fixed / Subclause 8.2.22 */
    { 41, "Forwarding Policy" },                                    /* Extendable / Subclause 8.2.23 */
    { 42, "Destination Interface" },                                /* Extendable / Subclause 8.2.24 */
    { 43, "UP Function Features" },                                 /* Extendable / Subclause 8.2.25 */
    { 44, "Apply Action" },                                         /* Extendable / Subclause 8.2.26 */
    { 45, "Downlink Data Service Information" },                    /* Extendable / Subclause 8.2.27 */
    { 46, "Downlink Data Notification Delay" },                     /* Extendable / Subclause 8.2.28 */
    { 47, "DL Buffering Duration" },                                /* Extendable / Subclause 8.2.29 */
    { 48, "DL Buffering Suggested Packet Count" },                  /* Variable / Subclause 8.2.30 */
    { 49, "PFCPSMReq-Flags" },                                      /* Extendable / Subclause 8.2.31 */
    { 50, "PFCPSRRsp-Flags" },                                      /* Extendable / Subclause 8.2.32 */
    { 51, "Load Control Information" },                             /* Extendable / Table 7.5.3.3-1 */
    { 52, "Sequence Number" },                                      /* Fixed Length / Subclause 8.2.33 */
    { 53, "Metric" },                                               /* Fixed Length / Subclause 8.2.34 */
    { 54, "Overload Control Information" },                         /* Extendable / Table 7.5.3.4-1 */
    { 55, "Timer" },                                                /* Extendable / Subclause 8.2 35 */
    { 56, "Packet Detection Rule ID" },                             /* Extendable / Subclause 8.2 36 */
    { 57, "F-SEID" },                                               /* Extendable / Subclause 8.2 37 */
    { 58, "Application ID's PFDs" },                                /* Extendable / Table 7.4.3.1-2 */
    { 59, "PFD context" },                                          /* Extendable / Table 7.4.3.1-3 */
    { 60, "Node ID" },                                              /* Extendable / Subclause 8.2.38 */
    { 61, "PFD contents" },                                         /* Extendable / Subclause 8.2.39 */
    { 62, "Measurement Method" },                                   /* Extendable / Subclause 8.2.40 */
    { 63, "Usage Report Trigger" },                                 /* Extendable / Subclause 8.2.41 */
    { 64, "Measurement Period" },                                   /* Extendable / Subclause 8.2.42 */
    { 65, "FQ-CSID" },                                              /* Extendable / Subclause 8.2.43 */
    { 66, "Volume Measurement" },                                   /* Extendable / Subclause 8.2.44 */
    { 67, "Duration Measurement" },                                 /* Extendable / Subclause 8.2.45 */
    { 68, "Application Detection Information" },                    /* Extendable / Table 7.5.8.3-2 */
    { 69, "Time of First Packet" },                                 /* Extendable / Subclause 8.2.46 */
    { 70, "Time of Last Packet" },                                  /* Extendable / Subclause 8.2.47 */
    { 71, "Quota Holding Time" },                                   /* Extendable / Subclause 8.2.48 */
    { 72, "Dropped DL Traffic Threshold" },                         /* Extendable / Subclause 8.2.49 */
    { 73, "Volume Quota" },                                         /* Extendable / Subclause 8.2.50 */
    { 74, "Time Quota" },                                           /* Extendable / Subclause 8.2.51 */
    { 75, "Start Time" },                                           /* Extendable / Subclause 8.2.52 */
    { 76, "End Time" },                                             /* Extendable / Subclause 8.2.53 */
    { 77, "Query URR" },                                            /* Extendable / Table 7.5.4.10-1 */
    { 78, "Usage Report (in Session Modification Response)" },      /* Extendable / Table 7.5.5.2-1 */
    { 79, "Usage Report (Session Deletion Response)" },             /* Extendable / Table 7.5.7.2-1 */
    { 80, "Usage Report (Session Report Request)" },                /* Extendable / Table 7.5.8.3-1 */
    { 81, "URR ID" },                                               /* Extendable / Subclause 8.2.54 */
    { 82, "Linked URR ID" },                                        /* Extendable / Subclause 8.2.55 */
    { 83, "Downlink Data Report" },                                 /* Extendable / Table 7.5.8.2-1 */
    { 84, "Outer Header Creation" },                                /* Extendable / Subclause 8.2.56 */
    { 85, "Create BAR" },                                           /* Extendable / Table 7.5.2.6-1 */
    { 86, "Update BAR (Session Modification Request)" },            /* Extendable / Table 7.5.4.11-1 */
    { 87, "Remove BAR" },                                           /* Extendable / Table 7.5.4.12-1 */
    { 88, "BAR ID" },                                               /* Extendable / Subclause 8.2.57 */
    { 89, "CP Function Features" },                                 /* Extendable / Subclause 8.2.58 */
    { 90, "Usage Information" },                                    /* Extendable / Subclause 8.2.59 */
    { 91, "Application Instance ID" },                              /* Variable Length / Subclause 8.2.60 */
    { 92, "Flow Information" },                                     /* Extendable / Subclause 8.2.61 */
    { 93, "UE IP Address" },                                        /* Extendable / Subclause 8.2.62 */
    { 94, "Packet Rate" },                                          /* Extendable / Subclause 8.2.63 */
    { 95, "Outer Header Removal" },                                 /* Extendable / Subclause 8.2.64 */
    { 96, "Recovery Time Stamp" },                                  /* Extendable / Subclause 8.2.65 */
    { 97, "DL Flow Level Marking" },                                /* Extendable / Subclause 8.2.66 */
    { 98, "Header Enrichment" },                                    /* Extendable / Subclause 8.2.67 */
    { 99, "Error Indication Report" },                              /* Extendable / Table 7.5.8.4-1 */
    { 100, "Measurement Information" },                             /* Extendable / Subclause 8.2.68 */
    { 101, "Node Report Type" },                                    /* Extendable / Subclause 8.2.69 */
    { 102, "User Plane Path Failure Report" },                      /* Extendable / Table 7.4.5.1.2-1 */
    { 103, "Remote GTP-U Peer" },                                   /* Extendable / Subclause 8.2.70 */
    { 104, "UR-SEQN" },                                             /* Fixed Length / Subclause 8.2.71 */
    { 105, "Update Duplicating Parameters" },                       /* Extendable / Table 7.5.4.3-3 */
    { 106, "Activate Predefined Rules" },                           /* Variable Length / Subclause 8.2.72 */
    { 107, "Deactivate Predefined Rules" },                         /* Variable Length / Subclause 8.2.73 */
    { 108, "FAR ID" },                                              /* Extendable / Subclause 8.2.74 */
    { 109, "QER ID" },                                              /* Extendable / Subclause 8.2.75 */
    { 110, "OCI Flags" },                                           /* Extendable / Subclause 8.2.76 */
    { 111, "PFCP Association Release Request" },                      /* Extendable / Subclause 8.2.77 */
    { 112, "Graceful Release Period" },                             /* Extendable / Subclause 8.2.78 */
    { 113, "PDN Type" },                                            /* Fixed Length / Subclause 8.2.79 */
    { 114, "Failed Rule ID" },                                      /* Extendable / Subclause 8.2.80 */
    { 115, "Time Quota Mechanism" },                                /* Extendable / Subclause 8.2.81 */
    { 116, "User Plane IP Resource Information" },                  /* Extendable / Subclause 8.2.82 */
    { 117, "User Plane Inactivity Timer" },                         /* Extendable / Subclause 8.2.83 */
    { 118, "Aggregated URRs" },                                     /* Extendable / Table 7.5.2.4-2 */
    { 119, "Multiplier" },                                          /* Fixed Length / Subclause 8.2.84 */
    { 120, "Aggregated URR ID IE" },                                /* Fixed Length / Subclause 8.2.85 */
    { 121, "Subsequent Volume Quota" },                             /* Extendable / Subclause 8.2.86 */
    { 122, "Subsequent Time Quota" },                               /* Extendable / Subclause 8.2.87 */
    { 123, "RQI" },                                                 /* Extendable / Subclause 8.2.88 */
    { 124, "QFI" },                                                 /* Extendable / Subclause 8.2.89 */
    { 125, "Query URR Reference" },                                 /* Extendable / Subclause 8.2.90 */
    { 126, "Additional Usage Reports Information" },                /* Extendable / Subclause 8.2.91 */
    { 127, "Create Traffic Endpoint" },                             /* Extendable / Table 7.5.2.7 */
    { 128, "Created Traffic Endpoint" },                            /* Extendable / Table 7.5.3.5 */
    { 129, "Update Traffic Endpoint" },                             /* Extendable / Table 7.5.4.13 */
    { 130, "Remove Traffic Endpoint" },                             /* Extendable / Table 7.5.4.14 */
    { 131, "Traffic Endpoint ID" },                                 /* Extendable / Subclause 8.2.92*/
    { 132, "Ethernet Packet Filter"},                               /* Extendable / Table 7.5.2.2-3 */
    { 133, "MAC address"},                                          /* Extendable / Subclause 8.2.93 */
    { 134, "C-TAG"},                                                /* Extendable / Subclause 8.2.94 */
    { 135, "S-TAG"},                                                /* Extendable / Subclause 8.2.95 */
    { 136, "Ethertype"},                                            /* Extendable / Subclause 8.2.96 */
    { 137, "Proxying"},                                             /* Extendable / Subclause 8.2.97 */
    { 138, "Ethernet Filter ID"},                                   /* Extendable / Subclause 8.2.98 */
    { 139, "Ethernet Filter Properties"},                           /* Extendable / Subclause 8.2.99 */
    { 140, "Suggested Buffering Packets Count"},                    /* Extendable / Subclause 8.2.100 */
    { 141, "User ID"},                                              /* Extendable / Subclause 8.2.101 */
    { 142, "Ethernet PDU Session Information"},                     /* Extendable / Subclause 8.2.102 */
    { 143, "Ethernet Traffic Information"},                         /* Extendable / Table 7.5.8.3-3 */
    { 144, "MAC Addresses Detected"},                               /* Extendable / Subclause 8.2.103 */
    { 145, "MAC Addresses Removed"},                                /* Extendable / Subclause 8.2.104 */
    { 146, "Ethernet Inactivity Timer"},                            /* Extendable / Subclause 8.2.105 */
    { 147, "Additional Monitoring Time"},                           /* Extendable / Table 7.5.2.4-3 */
    { 148, "Event Quota"},                                          /* Extendable / Subclause 8.2.112 */
    { 149, "Event Threshold"},                                      /* Extendable / Subclause 8.2.113 */
    { 150, "Subsequent Event Quota"},                               /* Extendable / Subclause 8.2.106 */
    { 151, "Subsequent Event Threshold"},                           /* Extendable / Subclause 8.2.107 */
    { 152, "Trace Information"},                                    /* Extendable / Subclause 8.2.108 */
    { 153, "Framed-Route"},                                         /* Variable Length  / Subclause 8.2.109 */
    { 154, "Framed-Routing"},                                       /* Fixed Length  / Subclause 8.2.110 */
    { 155, "Framed-IPv6-Route"},                                    /* Variable Length  / Subclause 8.2.111 */
    { 156, "Event Time Stamp"},                                     /* Extendable / Subclause 8.2.114 */
    { 157, "Averaging Window"},                                     /* Extendable / Subclause 8.2.115 */
    { 158, "Paging Policy Indicator"},                              /* Extendable / Subclause 8.2.116 */
    //159 to 32767 Spare. For future use.
    //32768 to 65535 Vendor-specific IEs.
    {0, NULL}
};

static value_string_ext pfcp_ie_type_ext = VALUE_STRING_EXT_INIT(pfcp_ie_type);

/* PFCP Session funcs*/
static guint32
pfcp_get_frame(address ip, guint64 seid, guint32 *frame) {
    gboolean found = FALSE;
    wmem_list_frame_t *elem;
    pfcp_info_t *info;
    wmem_list_t *info_list;
    gchar *ip_str;

    /* First we get the seid list*/
    ip_str = address_to_str(wmem_packet_scope(), &ip);
    info_list = (wmem_list_t*)wmem_tree_lookup_string(pfcp_frame_tree, ip_str, 0);
    if (info_list != NULL) {
        elem = wmem_list_head(info_list);
        while (!found && elem) {
            info = (pfcp_info_t*)wmem_list_frame_data(elem);
            if (seid == info->seid) {
                *frame = info->frame;
                return 1;
            }
            elem = wmem_list_frame_next(elem);
        }
    }
    return 0;
}

static gboolean
pfcp_call_foreach_ip(const void *key _U_, void *value, void *data){
    wmem_list_frame_t * elem;
    wmem_list_t *info_list = (wmem_list_t *)value;
    pfcp_info_t *info;
    guint32* frame = (guint32*)data;

    /* We loop over the <seid, frame> list */
    elem = wmem_list_head(info_list);
    while (elem) {
        info = (pfcp_info_t*)wmem_list_frame_data(elem);
        if (info->frame == *frame) {
            wmem_list_frame_t * del = elem;
            /* proceed to next request */
            elem = wmem_list_frame_next(elem);
            /* If we find the frame we remove its information from the list */
            wmem_list_remove_frame(info_list, del);
            wmem_free(wmem_file_scope(), info);
        }
        else {
            elem = wmem_list_frame_next(elem);
        }
    }

    return FALSE;
}

static void
pfcp_remove_frame_info(guint32 *f) {
    /* For each ip node */
    wmem_tree_foreach(pfcp_frame_tree, pfcp_call_foreach_ip, (void *)f);
}

static void
pfcp_add_session(guint32 frame, guint32 session) {
    guint32 *f, *session_count;

    f = wmem_new0(wmem_file_scope(), guint32);
    session_count = wmem_new0(wmem_file_scope(), guint32);
    *f = frame;
    *session_count = session;
    g_hash_table_insert(pfcp_session_table, f, session_count);
}

static gboolean
pfcp_seid_exists(guint64 seid, wmem_list_t *seid_list) {
    wmem_list_frame_t *elem;
    guint32 *info;
    gboolean found;
    found = FALSE;
    elem = wmem_list_head(seid_list);
    while (!found && elem) {
        info = (guint32*)wmem_list_frame_data(elem);
        found = *info == seid;
        elem = wmem_list_frame_next(elem);
    }
    return found;
}

static gboolean
pfcp_ip_exists(address ip, wmem_list_t *ip_list) {
    wmem_list_frame_t *elem;
    address *info;
    gboolean found;
    found = FALSE;
    elem = wmem_list_head(ip_list);
    while (!found && elem) {
        info = (address*)wmem_list_frame_data(elem);
        found = addresses_equal(info, &ip);
        elem = wmem_list_frame_next(elem);
    }
    return found;
}

static gboolean
pfcp_info_exists(pfcp_info_t *wanted, wmem_list_t *info_list) {
    wmem_list_frame_t *elem;
    pfcp_info_t *info;
    gboolean found;
    found = FALSE;
    elem = wmem_list_head(info_list);
    while (!found && elem) {
        info = (pfcp_info_t*)wmem_list_frame_data(elem);
        found = wanted->seid == info->seid;
        elem = wmem_list_frame_next(elem);
    }
    return found;
}

static void
pfcp_fill_map(wmem_list_t *seid_list, wmem_list_t *ip_list, guint32 frame) {
    wmem_list_frame_t *elem_ip, *elem_seid;
    pfcp_info_t *pfcp_info;
    wmem_list_t * info_list; /* List of <seids,frames>*/
    guint32 *f, *session, *fr, *session_count;
    GHashTableIter iter;
    guint64 seid;
    gchar *ip;

    elem_ip = wmem_list_head(ip_list);

    while (elem_ip) {
        ip = address_to_str(wmem_file_scope(), (address*)wmem_list_frame_data(elem_ip));
        /* We check if a seid list exists for this ip */
        info_list = (wmem_list_t*)wmem_tree_lookup_string(pfcp_frame_tree, ip, 0);
        if (info_list == NULL) {
            info_list = wmem_list_new(wmem_file_scope());
        }

        /* We loop over the seid list */
        elem_seid = wmem_list_head(seid_list);
        while (elem_seid) {
            seid = *(guint64*)wmem_list_frame_data(elem_seid);
            f = wmem_new0(wmem_file_scope(), guint32);
            *f = frame;
            pfcp_info = wmem_new0(wmem_file_scope(), pfcp_info_t);
            pfcp_info->seid = seid;
            pfcp_info->frame = *f;

            if (pfcp_info_exists(pfcp_info, info_list)) {
                /* If the seid and ip already existed, that means that we need to remove old info about that session */
                /* We look for its session ID */
                session = (guint32 *)g_hash_table_lookup(pfcp_session_table, f);
                if (session) {
                    g_hash_table_iter_init(&iter, pfcp_session_table);

                    while (g_hash_table_iter_next(&iter, (gpointer*)&fr, (gpointer*)&session_count)) {
                        /* If the msg has the same session ID and it's not the upd req we have to remove its info */
                        if (*session_count == *session) {
                            /* If it's the session we are looking for, we remove all the frame information */
                            pfcp_remove_frame_info(fr);
                        }
                    }
                }
            }
            wmem_list_prepend(info_list, pfcp_info);
            elem_seid = wmem_list_frame_next(elem_seid);
        }
        wmem_tree_insert_string(pfcp_frame_tree, ip, info_list, 0);
        elem_ip = wmem_list_frame_next(elem_ip);
    }
}

static gboolean
pfcp_is_cause_accepted(guint8 cause) {
    return cause == 1;
}

/* Data structure attached to a conversation
*  of a session
*/
typedef struct pfcp_session_conv_info_t {
    struct pfcp_session_conv_info_t *next;
    GHashTable             *unmatched;
    GHashTable             *matched;
} pfcp_session_conv_info_t;

static pfcp_session_conv_info_t *pfcp_session_info_items = NULL;

/* Data structure attached to a conversation,
*  to keep track of request/response-pairs
*/
typedef struct pfcp_conv_info_t {
    struct pfcp_conv_info_t *next;
    wmem_map_t             *unmatched;
    wmem_map_t             *matched;
} pfcp_conv_info_t;

static pfcp_conv_info_t *pfcp_info_items = NULL;

/* structure used to track responses to requests using sequence number */
typedef struct pfcp_msg_hash_entry {
    gboolean is_request;    /* TRUE/FALSE */
    guint32 req_frame;      /* frame with request */
    nstime_t req_time;      /* req time */
    guint32 rep_frame;      /* frame with reply */
    gint seq_nr;            /* sequence number */
    guint msgtype;          /* messagetype */
} pfcp_msg_hash_t;

static guint
pfcp_sn_hash(gconstpointer k)
{
    const pfcp_msg_hash_t *key = (const pfcp_msg_hash_t *)k;

    return key->seq_nr;
}

static gboolean
pfcp_sn_equal_matched(gconstpointer k1, gconstpointer k2)
{
    const pfcp_msg_hash_t *key1 = (const pfcp_msg_hash_t *)k1;
    const pfcp_msg_hash_t *key2 = (const pfcp_msg_hash_t *)k2;

    if (key1->req_frame && key2->req_frame && (key1->req_frame != key2->req_frame)) {
        return 0;
    }

    if (key1->rep_frame && key2->rep_frame && (key1->rep_frame != key2->rep_frame)) {
        return 0;
    }

    return key1->seq_nr == key2->seq_nr;
}

static gboolean
pfcp_sn_equal_unmatched(gconstpointer k1, gconstpointer k2)
{
    const pfcp_msg_hash_t *key1 = (const pfcp_msg_hash_t *)k1;
    const pfcp_msg_hash_t *key2 = (const pfcp_msg_hash_t *)k2;

    return key1->seq_nr == key2->seq_nr;
}

static void
pfcp_track_session(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, pfcp_hdr_t * pfcp_hdr, wmem_list_t *seid_list, wmem_list_t *ip_list, guint64 last_seid _U_, address last_ip _U_)
{
    guint32 *session, frame_seid_cp;
    proto_item *it;

    /* PFCP session */
    if (tree) {
        session = (guint32*)g_hash_table_lookup(pfcp_session_table, &pinfo->num);
        if (session) {
            it = proto_tree_add_uint(tree, hf_pfcp_session, tvb, 0, 0, *session);
            PROTO_ITEM_SET_GENERATED(it);
        }
    }

    if (!PINFO_FD_VISITED(pinfo)) {
        /* If the message does not have any session ID */
        session = (guint32*)g_hash_table_lookup(pfcp_session_table, &pinfo->num);
        if (!session) {
            /* If the message is not a SEREQ, SERES, SMREQ, SERES, SDREQ, SDRES, SRREQ or SRRES then we remove its information from seid and ip lists */
            if ((pfcp_hdr->message != PFCP_MSG_SESSION_ESTABLISHMENT_REQUEST && pfcp_hdr->message != PFCP_MSG_SESSION_ESTABLISHMENT_RESPONSE &&
                pfcp_hdr->message != PFCP_MSG_SESSION_MODIFICATION_REQUEST && pfcp_hdr->message != PFCP_MSG_SESSION_MODIFICATION_RESPONSE &&
                pfcp_hdr->message != PFCP_MSG_SESSION_DELETION_REQUEST && pfcp_hdr->message != PFCP_MSG_SESSION_DELETION_RESPONSE &&
                pfcp_hdr->message != PFCP_MSG_SESSION_REPORT_REQUEST && pfcp_hdr->message != PFCP_MSG_SESSION_REPORT_RESPONSE)) {
                /* If the lists are not empty*/
                if (wmem_list_count(seid_list) && wmem_list_count(ip_list)) {
                    pfcp_remove_frame_info(&pinfo->num);
                }
            }
            if (pfcp_hdr->message == PFCP_MSG_SESSION_ESTABLISHMENT_REQUEST){
                /* If SEREQ and not already in the list then we create a new session*/
                pfcp_add_session(pinfo->num, pfcp_session_count++);
            }
            else if (pfcp_hdr->message != PFCP_MSG_SESSION_ESTABLISHMENT_RESPONSE) {
                /* We have to check if its seid == seid_cp and ip.dst == gsn_ipv4 from the lists, if that is the case then we have to assign
                the corresponding session ID */
                if ((pfcp_get_frame(pinfo->dst, (guint32)pfcp_hdr->seid, &frame_seid_cp) == 1)) {
                    /* Then we have to set its session ID */
                    session = (guint32*)g_hash_table_lookup(pfcp_session_table, &frame_seid_cp);
                    if (session != NULL) {
                        /* We add the corresponding session to the list so that when a response came we can associate its session ID*/
                        pfcp_add_session(pinfo->num, *session);
                    }
                }
            }
        }
    }
}

static void
dissect_pfcp_reserved(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_reserved, tvb, 0, length);
}

/* Functions for C-Tag and S-TAG
 * See 8.2.94 and 8.2.95
 */

/* From Tables G-2,3 of IEEE standard 802.1Q-2005 (and I-2,3,7 of 2011 and 2015 revisions) */
static const value_string pfcp_vlan_tag_pcp_vals[] = {
  { 0, "Best Effort (default), Drop Eligible"            },
  { 1, "Best Effort (default)"                           },
  { 2, "Critical Applications, Drop Eligible"            },
  { 3, "Critical Applications"                           },
  { 4, "Voice, < 10ms latency and jitter, Drop Eligible" },
  { 5, "Voice, < 10ms latency and jitter"                },
  { 6, "Internetwork Control"                            },
  { 7, "Network Control"                                 },
  { 0, NULL                                              }
};

static const true_false_string tfs_eligible_ineligible = {
    "Eligible",
    "Ineligible"
};

static int decode_pfcp_c_tag(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, gint offset)
{
    guint64 flags_val;

    static const int * pfcp_c_tag_flags[] = {
        &hf_pfcp_spare_b7_b3,
        &hf_pfcp_c_tag_flags_b2_vid,
        &hf_pfcp_c_tag_flags_b1_dei,
        &hf_pfcp_c_tag_flags_b0_pcp,
        NULL
    };
    /* Octet 5  Spare   VID   DEI   PCP */
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_c_tag_flags,
        ett_pfcp_c_tag, pfcp_c_tag_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT, &flags_val);
    offset += 1;

    //  Octet     8     7     6     5     4     3     2     1
    //    6    | C-VID                  |DEI|   PCP value     |
    proto_tree_add_item(tree, hf_pfcp_c_tag_cvid, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_pfcp_c_tag_dei_flag,
        ett_pfcp_c_tag_dei, pfcp_c_tag_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT | BMT_NO_TFS);
    proto_tree_add_item(tree, hf_pfcp_c_tag_pcp_value, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    // Octet 7 C-VID value
    proto_tree_add_item(tree, hf_pfcp_c_tag_cvid_value, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    return offset;
}

static int decode_pfcp_s_tag(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint offset)
{
    guint64 flags_val;

    static const int * pfcp_s_tag_flags[] = {
        &hf_pfcp_spare_b7_b3,
        &hf_pfcp_s_tag_flags_b2_vid,
        &hf_pfcp_s_tag_flags_b1_dei,
        &hf_pfcp_s_tag_flags_b0_pcp,
        NULL
    };
    /* Octet 5  Spare   VID   DEI   PCP */
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_s_tag_flags,
        ett_pfcp_s_tag, pfcp_s_tag_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT, &flags_val);
    offset += 1;

    //  Octet     8     7     6     5     4     3     2     1
    //    6    | S-VID                  |DEI|   PCP value     |
    proto_tree_add_item(tree, hf_pfcp_s_tag_svid, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_pfcp_s_tag_dei_flag,
        ett_pfcp_s_tag_dei, pfcp_s_tag_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT | BMT_NO_TFS);
    proto_tree_add_item(tree, hf_pfcp_s_tag_pcp_value, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    // Octet 7 S-VID value
    proto_tree_add_item(tree, hf_pfcp_s_tag_svid_value, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    return offset;
}

/*
 * 8.2.1    Cause
 */
static const value_string pfcp_cause_vals[] = {

    {  0, "Reserved" },
    {  1, "Request accepted(success)" },
    /* 2 - 63 Spare. */
    { 64, "Request rejected(reason not specified)" },
    { 65, "Session context not found" },
    { 66, "Mandatory IE missing" },
    { 67, "Conditional IE missing" },
    { 68, "Invalid length" },
    { 69, "Mandatory IE incorrect" },
    { 70, "Invalid Forwarding Policy" },
    { 71, "Invalid F - TEID allocation option" },
    { 72, "No established PFCP Association" },
    { 73, "Rule creation / modification Failure" },
    { 74, "PFCP entity in congestion" },
    { 75, "No resources available" },
    { 76, "Service not supported" },
    { 77, "System failure" },
    /* 78 to 255 Spare for future use in a response message. */
    {0, NULL}
};

static void
dissect_pfcp_cause(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item, guint16 length _U_, guint8 message_type _U_, pfcp_session_args_t *args)
{
    guint32 value;
    /* Octet 5 Cause value */
    proto_tree_add_item_ret_uint(tree, hf_pfcp2_cause, tvb, 0, 1, ENC_BIG_ENDIAN, &value);
    if (g_pfcp_session) {
        args->last_cause = (guint8)value;
    }
    proto_item_append_text(item, "%s", val_to_str_const(value, pfcp_cause_vals, "Unknown"));
}

/*
 * 8.2.2    Source Interface
 */
static const value_string pfcp_source_interface_vals[] = {

    { 0, "Access" },
    { 1, "Core" },
    { 2, "SGi-LAN/N6-LAN" },
    { 3, "CP-function" },
    { 0, NULL }
};
static int
decode_pfcp_source_interface(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item, gint offset)
{
    guint32 value;
    /* Octet 5 Spare    Interface value */
    proto_tree_add_item(tree, hf_pfcp_spare_h1, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item_ret_uint(tree, hf_pfcp_source_interface, tvb, offset, 1, ENC_BIG_ENDIAN, &value);
    offset += 1;

    proto_item_append_text(item, "%s", val_to_str_const(value, pfcp_source_interface_vals, "Unknown"));

    return offset;

}
static void
dissect_pfcp_source_interface(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    offset = decode_pfcp_source_interface(tvb, pinfo, tree, item, offset);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.3    F-TEID
 */
static void
dissect_pfcp_f_teid(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 fteid_flags_val;

    static const int * pfcp_fteid_flags[] = {
        &hf_pfcp_fteid_flg_spare,
        &hf_pfcp_fteid_flg_b3_ch_id,
        &hf_pfcp_fteid_flg_b2_ch,
        &hf_pfcp_fteid_flg_b1_v6,
        &hf_pfcp_fteid_flg_b0_v4,
        NULL
    };
    /* Octet 5  Spare  Spare  Spare  Spare  CHID  CH  V6  V4*/
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_f_teid_flags,
        ett_f_teid_flags, pfcp_fteid_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT | BMT_NO_TFS, &fteid_flags_val);
    offset += 1;
    /* The following flags are coded within Octet 5:
     * Bit 1 - V4: If this bit is set to "1" and the CH bit is not set, then the IPv4 address field shall be present,
     *         otherwise the IPv4 address field shall not be present.
     * Bit 2 - V6: If this bit is set to "1" and the CH bit is not set, then the IPv6 address field shall be present,
     *         otherwise the IPv6 address field shall not be present.
     * Bit 3 - CH (CHOOSE): If this bit is set to "1", then the TEID, IPv4 address and IPv6 address fields shall not be
     *         present and the UP function shall assign an F-TEID with an IP4 or an IPv6 address if the V4 or V6 bit is set respectively.
     *         This bit shall only be set by the CP function.
     * Bit 4 - CHID (CHOOSE_ID):If this bit is set to "1", then the UP function shall assign the same F-TEID to the
     *         PDRs requested to be created in a PFCP Session Establishment Request or PFCP Session Modification Request with
     *         the same CHOOSE ID value.
     *         This bit may only be set to "1" if the CH bit is set to "1".
     *         This bit shall only be set by the CP function.
     */

    if ((fteid_flags_val & 0x4) == 4) {
        if ((fteid_flags_val & 0x8) == 8) {
            proto_tree_add_item(tree, hf_pfcp_f_teid_ch_id, tvb, offset, 1, ENC_NA);
            offset += 1;
        }
    } else {

        /* Octet 6 to 9    TEID */
        proto_tree_add_item(tree, hf_pfcp_f_teid_teid, tvb, offset, 4, ENC_BIG_ENDIAN);
        proto_item_append_text(item, "TEID: 0x%s", tvb_bytes_to_str(wmem_packet_scope(), tvb, offset, 4));
        offset += 4;

        if ((fteid_flags_val & 0x1) == 1) {
            /* m to (m+3)    IPv4 address */
            proto_tree_add_item(tree, hf_pfcp_f_teid_ipv4, tvb, offset, 4, ENC_BIG_ENDIAN);
            proto_item_append_text(item, ", IPv4 %s", tvb_ip_to_str(tvb, offset));
            offset += 4;
        }
        if ((fteid_flags_val & 0x2) == 2) {
            /* p to (p+15)   IPv6 address */
            proto_tree_add_item(tree, hf_pfcp_f_teid_ipv6, tvb, offset, 16, ENC_NA);
            proto_item_append_text(item, ", IPv6 %s", tvb_ip6_to_str(tvb, offset));
            offset += 16;
        }
        /* If the value of CH bit is set to "0", but the value of CHID bit is "1" */
        if ((fteid_flags_val & 0x8) == 8) {
            proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_encoding_error, tvb, 0, 1);
        }
    }
    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.4    Network Instance
 */
static int
decode_pfcp_network_instance(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, gint offset, int length)
{

    int      name_len;

    if (length > 0) {
        name_len = tvb_get_guint8(tvb, offset);
        if (name_len < 0x41) {
            /* APN */
            guint8 *apn = NULL;
            int     tmp;

            name_len = tvb_get_guint8(tvb, offset);

            if (name_len < 0x20) {
                apn = tvb_get_string_enc(wmem_packet_scope(), tvb, offset + 1, length - 1, ENC_ASCII);
                for (;;) {
                    if (name_len >= length - 1)
                        break;
                    tmp = name_len;
                    name_len = name_len + apn[tmp] + 1;
                    apn[tmp] = '.';
                }
            } else {
                apn = tvb_get_string_enc(wmem_packet_scope(), tvb, offset, length, ENC_ASCII);
            }
            proto_tree_add_string(tree, hf_pfcp_network_instance, tvb, offset, length, apn);
            proto_item_append_text(item, "%s", apn);

        } else {
            /* Domain name*/
            const guint8* string_value;
            proto_tree_add_item_ret_string(tree, hf_pfcp_network_instance, tvb, offset, length, ENC_ASCII | ENC_NA, wmem_packet_scope(), &string_value);
            proto_item_append_text(item, "%s", string_value);
        }
    }

    return offset + length;
}
static void
dissect_pfcp_network_instance(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item , guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int      offset = 0;

    /* Octet 5   Network Instance
     * The Network instance field shall be encoded as an OctetString and shall contain an identifier
     * which uniquely identifies a particular Network instance (e.g. PDN instance) in the UP function.
     * It may be encoded as a Domain Name or an Access Point Name (APN)
     */
     /* Test for Printable character or length indicator(APN), assume first character of Domain name >= 0x41 */

    decode_pfcp_network_instance(tvb, pinfo, tree, item, offset, length);

}

/*
 * 8.2.5    SDF Filter
 */
static void
dissect_pfcp_sdf_filter(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 flags_val;
    guint32 fd_length;
    proto_tree *flow_desc_tree, *tos_tree, *spi_tree, *flow_label_tree, *sdf_filter_id_tree;

    static const int * pfcp_sdf_filter_flags[] = {
        &hf_pfcp_spare_h1,
        &hf_pfcp_sdf_filter_flags_b4_bid,
        &hf_pfcp_sdf_filter_flags_b3_fl,
        &hf_pfcp_sdf_filter_flags_b2_spi,
        &hf_pfcp_sdf_filter_flags_b1_ttc,
        &hf_pfcp_sdf_filter_flags_b0_fd,
        NULL
    };
    /* Octet 5  Spare   FL  SPI TTC FD*/
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_sdf_filter_flags,
        ett_pfcp_sdf_filter_flags, pfcp_sdf_filter_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT | BMT_NO_TFS, &flags_val);
    offset += 1;
    /* Octet 6 Spare*/
    proto_tree_add_item(tree, hf_pfcp_spare, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    if ((flags_val & 0x1) == 1) {
        /* FD (Flow Description): If this bit is set to "1",
         * then the Length of Flow Description and the Flow Description fields shall be present
         */
        flow_desc_tree = proto_item_add_subtree(item, ett_pfcp_flow_desc);
        /* m to (m+1)    Length of Flow Description */
        proto_tree_add_item_ret_uint(flow_desc_tree, hf_pfcp_flow_desc_len, tvb, offset, 2, ENC_BIG_ENDIAN, &fd_length);
        offset += 2;
        /* Flow Description
         * The Flow Description field, when present, shall be encoded as an OctetString
         * as specified in subclause 5.4.2 of 3GPP TS 29.212
         */
        proto_tree_add_item(flow_desc_tree, hf_pfcp_flow_desc, tvb, offset, fd_length, ENC_ASCII|ENC_NA);
        offset += fd_length;
    }
    if ((flags_val & 0x2) == 2) {
        /* TTC (ToS Traffic Class): If this bit is set to "1", then the ToS Traffic Class field shall be present */
        /* ToS Traffic Class field, when present, shall be encoded as an OctetString on two octets
         * as specified in subclause 5.3.15 of 3GPP TS 29.212
         */
        tos_tree = proto_item_add_subtree(item, ett_pfcp_tos);
        proto_tree_add_item(tos_tree, hf_pfcp_traffic_class, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset += 1;
        proto_tree_add_item(tos_tree, hf_pfcp_traffic_mask, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset += 1;
    }

    if ((flags_val & 0x4) == 4) {
        /* SPI (The Security Parameter Index) field, when present, shall be encoded as an OctetString on four octets and shall
         * contain the IPsec security parameter index (which is a 32-bit field),
         * as specified in subclause 5.3.51 of 3GPP TS 29.212
         */
        spi_tree = proto_item_add_subtree(item, ett_pfcp_spi);
        proto_tree_add_item(spi_tree, hf_pfcp_spi, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
    }
    if ((flags_val & 0x8) == 8) {
        /* FL (Flow Label), when present, shall be encoded as an OctetString on 3 octets as specified in
         * subclause 5.3.52 of 3GPP TS 29.212 and shall contain an IPv6 flow label (which is a 20-bit field).
         * The bits 8 to 5 of the octet "v" shall be spare and set to zero, and the remaining 20 bits shall
         * contain the IPv6 flow label.*/
        flow_label_tree = proto_item_add_subtree(item, ett_pfcp_flow_label);
        proto_tree_add_bits_item(flow_label_tree, hf_pfcp_flow_label_spare_bit, tvb, (offset<<3), 4, ENC_BIG_ENDIAN);
        proto_tree_add_item(flow_label_tree, hf_pfcp_flow_label, tvb, offset, 3, ENC_BIG_ENDIAN);
        offset += 3;
    }
    if ((flags_val & 0x10) == 16) {
        /* The SDF Filter ID, when present, shall be encoded as an Unsigned32 binary integer value.
         * It shall uniquely identify an SDF Filter among all the SDF Filters provisioned for a given PFCP Session. */
        sdf_filter_id_tree = proto_item_add_subtree(item, ett_pfcp_sdf_filter_id);
        proto_tree_add_item(sdf_filter_id_tree, hf_pfcp_sdf_filter_id, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}
/*
 * 8.2.6    Application ID
 */
static void
dissect_pfcp_application_id(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    /* Octet 5 to (n+4) Application Identifier
    * The Application Identifier shall be encoded as an OctetString (see 3GPP TS 29.212)
    */
    if (tvb_ascii_isprint(tvb, offset, length))
    {
        const guint8* string_value;
        proto_tree_add_item_ret_string(tree, hf_pfcp_application_id_str, tvb, offset, length, ENC_ASCII | ENC_NA, wmem_packet_scope(), &string_value);
        proto_item_append_text(item, "%s", string_value);
    }
    else
    {
        proto_tree_add_item(tree, hf_pfcp_application_id, tvb, offset, length, ENC_NA);
    }
}
/*
 * 8.2.7    Gate Status
 */
static const value_string pfcp_gate_status_vals[] = {
    { 0, "OPEN" },
    { 1, "CLOSED" },
    { 0, NULL }
};


static void
dissect_pfcp_gate_status(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    static const int * pfcp_gate_status_flags[] = {
        &hf_pfcp_gate_status_b3b2_ulgate,
        &hf_pfcp_gate_status_b0b1_dlgate,
        NULL
    };
    /* Octet 5  Spare   UL Gate DL Gate */
    proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_pfcp_gate_status,
        ett_pfcp_gate_status, pfcp_gate_status_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}


/*
 * 8.2.8    MBR
 */
static void
dissect_pfcp_mbr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    int len1 = (length != 10) ? length/2 : 5;

    /* In case length is not in accordance with documentation */
    if ( length != 10) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_encoding_error, tvb, 0, 1);
    }

    /* 5 to 9   UL MBR
    * The UL/DL MBR fields shall be encoded as kilobits per second (1 kbps = 1000 bps) in binary value
    */
    proto_tree_add_item(tree, hf_pfcp_ul_mbr, tvb, offset, len1, ENC_BIG_ENDIAN);
    offset += len1;

    /* 10 to 14 DL MBR */
    proto_tree_add_item(tree, hf_pfcp_dl_mbr, tvb, offset, len1, ENC_BIG_ENDIAN);
    offset += len1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.9    GBR
 */
static void
dissect_pfcp_gbr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    int len1 = (length != 10) ? length/2 : 5;

    /* In case length is not in accordance with documentation */
    if ( length != 10) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_encoding_error, tvb, 0, 1);
    }

    /* 5 to 9   UL GBR
    * The UL/DL MBR fields shall be encoded as kilobits per second (1 kbps = 1000 bps) in binary value
    */
    proto_tree_add_item(tree, hf_pfcp_ul_gbr, tvb, offset, len1, ENC_BIG_ENDIAN);
    offset += len1;

    /* 10 to 14 DL GBR */
    proto_tree_add_item(tree, hf_pfcp_dl_gbr, tvb, offset, len1, ENC_BIG_ENDIAN);
    offset += len1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.10   QER Correlation ID
 */
static void
dissect_pfcp_qer_correlation_id(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;
    /* 5 to 8   QER Correlation ID value */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_qer_correlation_id, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    proto_item_append_text(item, "%u", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.11   Precedence
 */
static void
dissect_pfcp_precedence(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;
    /* Octet 5 5 to 8   Precedence value */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_precedence, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    proto_item_append_text(item, "%u", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.12   Transport Level Marking
 */
static void
dissect_pfcp_transport_level_marking(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    /* Octet 5 to 6    ToS/Traffic Class
    * The ToS/Traffic Class shall be encoded on two octets as an OctetString.
    * The first octet shall contain the IPv4 Type-of-Service or the IPv6 Traffic-Class field and the second octet shall contain the ToS/Traffic Class mask field
    */
    proto_tree_add_item(tree, hf_pfcp_traffic_class, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;
    proto_tree_add_item(tree, hf_pfcp_traffic_mask, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.13   Volume Threshold
 */
static void
dissect_pfcp_volume_threshold(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 flags_val;

    static const int * pfcp_volume_threshold_flags[] = {
        &hf_pfcp_spare_b7_b3,
        &hf_pfcp_volume_threshold_b2_dlvol,
        &hf_pfcp_volume_threshold_b1_ulvol,
        &hf_pfcp_volume_threshold_b0_tovol,
        NULL
    };
    /* Octet 5  Spare   DLVOL   ULVOL   TOVOL*/
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_volume_threshold,
        ett_pfcp_volume_threshold, pfcp_volume_threshold_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT, &flags_val);
    offset += 1;

    /* The Total Volume, Uplink Volume and Downlink Volume fields shall be encoded as an Unsigned64 binary integer value.
    * They shall contain the total, uplink or downlink number of octets respectively.
    */
    if ((flags_val & 0x1) == 1) {
        /* m to (m+7)   Total Volume
        * TOVOL: If this bit is set to "1", then the Total Volume field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_volume_threshold_tovol, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
    }
    if ((flags_val & 0x2) == 2) {
        /* p to (p+7)    Uplink Volume
        * ULVOL: If this bit is set to "1", then the Uplink Volume field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_volume_threshold_ulvol, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
    }
    if ((flags_val & 0x4) == 4) {
        /* q to (q+7)   Downlink Volume
        * DLVOL: If this bit is set to "1", then the Downlink Volume field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_volume_threshold_dlvol, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}
/*
 * 8.2.14   Time Threshold
 */
static void
dissect_pfcp_time_threshold(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint value;

    /* Octet 5 to 8    Time Threshold
    * The Time Threshold field shall be encoded as an Unsigned32 binary integer value.
    * It shall contain the duration in seconds.
    */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_time_threshold, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    proto_item_append_text(item, "%u s", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.15   Monitoring Time
 */
static void
dissect_pfcp_monitoring_time(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    const gchar *time_str;
    int offset = 0;

    /* The Monitoring Time field shall indicate the monitoring time in UTC time.
    * Octets 5 to 8 shall be encoded in the same format as the first four octets
    * of the 64-bit timestamp format as defined in section 6 of IETF RFC 5905.
    */
    time_str = tvb_ntp_fmt_ts_sec(tvb, 0);
    proto_tree_add_string(tree, hf_pfcp_monitoring_time, tvb, offset, 4, time_str);
    proto_item_append_text(item, "%s", time_str);
    offset += 4;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.16   Subsequent Volume Threshold
 */
static void
dissect_pfcp_subseq_volume_threshold(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 flags_val;

    static const int * pfcp_subseq_volume_threshold_flags[] = {
        &hf_pfcp_spare_b7_b3,
        &hf_pfcp_subseq_volume_threshold_b2_dlvol,
        &hf_pfcp_subseq_volume_threshold_b1_ulvol,
        &hf_pfcp_subseq_volume_threshold_b0_tovol,
        NULL
    };
    /* Octet 5  Spare   DLVOL   ULVOL   TOVOL*/
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_subseq_volume_threshold,
        ett_pfcp_subseq_volume_threshold, pfcp_subseq_volume_threshold_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT, &flags_val);
    offset += 1;

    /* The Total Volume, Uplink Volume and Downlink Volume fields shall be encoded as an Unsigned64 binary integer value.
    * They shall contain the total, uplink or downlink number of octets respectively.
    */
    if ((flags_val & 0x1) == 1) {
        /* m to (m+7)   Total Volume
        * TOVOL: If this bit is set to "1", then the Total Volume field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_subseq_volume_threshold_tovol, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
    }
    if ((flags_val & 0x2) == 2) {
        /* p to (p+7)    Uplink Volume
        * ULVOL: If this bit is set to "1", then the Uplink Volume field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_subseq_volume_threshold_ulvol, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
    }
    if ((flags_val & 0x4) == 4) {
        /* q to (q+7)   Downlink Volume
        * DLVOL: If this bit is set to "1", then the Downlink Volume field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_subseq_volume_threshold_dlvol, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 * 8.2.17   Subsequent Time Threshold
 */
static void
dissect_pfcp_subsequent_time_threshold(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;
    /* 5 to 8   Subsequent Time Threshold */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_subsequent_time_threshold, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    proto_item_append_text(item, "%u s", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}
/*
 * 8.2.18   Inactivity Detection Time
 */
static void
dissect_pfcp_inactivity_detection_time(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;
    /* 5 to 8   Inactivity Detection Time */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_inactivity_detection_time, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    proto_item_append_text(item, "%u s", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 * 8.2.19   Reporting Triggers
 */
static void
dissect_pfcp_reporting_triggers(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    static const int * pfcp_reporting_triggers_o5_flags[] = {
        &hf_pfcp_reporting_triggers_o5_b7_liusa,
        &hf_pfcp_reporting_triggers_o5_b6_droth,
        &hf_pfcp_reporting_triggers_o5_b5_stopt,
        &hf_pfcp_reporting_triggers_o5_b4_start,
        &hf_pfcp_reporting_triggers_o5_b3_quhti,
        &hf_pfcp_reporting_triggers_o5_b2_timth,
        &hf_pfcp_reporting_triggers_o5_b1_volth,
        &hf_pfcp_reporting_triggers_o5_b0_perio,
        NULL
    };
    /* Octet 5 [Bits 15-08] LIUSA   DROTH   STOPT   START   QUHTI   TIMTH   VOLTH   PERIO */
    proto_tree_add_bitmask_list(tree, tvb, offset, 1, pfcp_reporting_triggers_o5_flags, ENC_BIG_ENDIAN);
    offset++;

    if (offset == length) {
        return;
    }

    static const int * pfcp_reporting_triggers_o6_flags[] = {
        &hf_pfcp_spare_b7_b5,
        &hf_pfcp_reporting_triggers_o6_b5_evequ,
        &hf_pfcp_reporting_triggers_o6_b4_eveth,
        &hf_pfcp_reporting_triggers_o6_b3_macar,
        &hf_pfcp_reporting_triggers_o6_b2_envcl,
        &hf_pfcp_reporting_triggers_o6_b1_timqu,
        &hf_pfcp_reporting_triggers_o6_b0_volqu,
        NULL
    };
    /* Octet 6 [Bits 07-00] SPARE   SPARE   EVEQU   EVETH   MACAR   ENVCL   TIMQU   VOLQU */
    proto_tree_add_bitmask_list(tree, tvb, offset, 1, pfcp_reporting_triggers_o6_flags, ENC_BIG_ENDIAN);
    offset++;

    if (offset == length) {
        return;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.20   Redirect Information
 */
static const value_string pfcp_redirect_address_type_vals[] = {

    { 0, "IPv4 address" },
    { 1, "IPv6 address" },
    { 2, "URL" },
    { 3, "SIP URI" },
    { 0, NULL }
};

static void
dissect_pfcp_redirect_information(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value, addr_len;

    /* Octet Spare  Redirect Address Type */
    proto_tree_add_item(tree, hf_pfcp_spare_h1, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item_ret_uint(tree, hf_pfcp_redirect_address_type, tvb, offset, 1, ENC_BIG_ENDIAN, &value);
    offset++;

    /* 6-7  Redirect Server Address Length=a */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_redirect_server_addr_len, tvb, offset, 2, ENC_BIG_ENDIAN, &addr_len);
    offset+=2;

    /* 8-(8+a)  Redirect Server Address */
    proto_tree_add_item(tree, hf_pfcp_redirect_server_address, tvb, offset, addr_len, ENC_UTF_8 | ENC_NA);
    offset += addr_len;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}
/*
 * 8.2.21   Report Type
 */
static void
dissect_pfcp_report_type(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    static const int * pfcp_report_type_flags[] = {
        &hf_pfcp_spare_b7_b4,
        &hf_pfcp_report_type_b3_upir,
        &hf_pfcp_report_type_b2_erir,
        &hf_pfcp_report_type_b1_usar,
        &hf_pfcp_report_type_b0_dldr,
        NULL
    };
    /* Octet 5  Spare   UPIR   ERIR    USAR    DLDR */
    proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_pfcp_report_type,
        ett_pfcp_report_type, pfcp_report_type_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.22   Offending IE
 */
static void
dissect_pfcp_offending_ie(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item, guint16 length _U_, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    guint32 value;
    /* Octet 5 to 6 Type of the offending IE */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_offending_ie, tvb, 0, 2, ENC_BIG_ENDIAN, &value);

    proto_item_append_text(item, "%s", val_to_str_const(value, pfcp_ie_type, "Unknown"));

}
/*
 * 8.2.23   Forwarding Policy
 */
static void
dissect_pfcp_forwarding_policy(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 id_len;

    /* Octet Forwarding Policy Identifier Length */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_forwarding_policy_id_len, tvb, offset, 1, ENC_BIG_ENDIAN, &id_len);
    offset += 1;

    proto_tree_add_item(tree, hf_pfcp_forwarding_policy_id, tvb, offset, id_len, ENC_NA);
    offset += id_len;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.24   Destination Interface
 */
static const value_string pfcp_dst_interface_vals[] = {

    { 0, "Access" },
    { 1, "Core" },
    { 2, "SGi-LAN/N6-LAN" },
    { 3, "CP- Function" },
    { 4, "LI Function" },
    { 0, NULL }
};

static void
dissect_pfcp_destination_interface(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;

    /* Octet 5    Spare    Interface value*/
    proto_tree_add_item(tree, hf_pfcp_spare_h1, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item_ret_uint(tree, hf_pfcp_dst_interface, tvb, offset, 1, ENC_BIG_ENDIAN, &value);
    offset++;

    proto_item_append_text(item, "%s", val_to_str_const(value, pfcp_dst_interface_vals, "Unknown"));

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.25   UP Function Features
 */
static void
dissect_pfcp_up_function_features(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    static const int * pfcp_up_function_features_o5_flags[] = {
        &hf_pfcp_up_function_features_o5_b7_treu,
        &hf_pfcp_up_function_features_o5_b6_heeu,
        &hf_pfcp_up_function_features_o5_b5_pfdm,
        &hf_pfcp_up_function_features_o5_b4_ftup,
        &hf_pfcp_up_function_features_o5_b3_trst,
        &hf_pfcp_up_function_features_o5_b2_dlbd,
        &hf_pfcp_up_function_features_o5_b1_ddnd,
        &hf_pfcp_up_function_features_o5_b0_bucp,
        NULL
    };
    static const int * pfcp_up_function_features_o6_flags[] = {
        &hf_pfcp_spare_b7_b6,
        &hf_pfcp_up_function_features_o6_b5_frrt,
        &hf_pfcp_up_function_features_o6_b4_trace,
        &hf_pfcp_up_function_features_o6_b3_quoac,
        &hf_pfcp_up_function_features_o6_b2_udbc,
        &hf_pfcp_up_function_features_o6_b1_pdiu,
        &hf_pfcp_up_function_features_o6_b0_empu,
        NULL
    };
    /* Octet 5  TREU    HEEU    PFDM    FTUP    TRST    DLBD    DDND    BUCP */
    /* Octet 6  Spare   Spare   FRRT    TRACE   QUOAC   UDBC    PDIU    EMPU */
    proto_tree_add_bitmask_list(tree, tvb, offset, 1, pfcp_up_function_features_o5_flags, ENC_BIG_ENDIAN);
    offset++;
    proto_tree_add_bitmask_list(tree, tvb, offset, 1, pfcp_up_function_features_o6_flags, ENC_BIG_ENDIAN);
    offset++;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.26   Apply Action
 */
static void
dissect_pfcp_apply_action(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    static const int * pfcp_apply_action_flags[] = {
        &hf_pfcp_spare_b7_b5,
        &hf_pfcp_apply_action_flags_b4_dupl,
        &hf_pfcp_apply_action_flags_b3_nocp,
        &hf_pfcp_apply_action_flags_b2_buff,
        &hf_pfcp_apply_action_flags_b1_forw,
        &hf_pfcp_apply_action_flags_b0_drop,
        NULL
    };
    /* Octet 5  Spare   Spare   Spare   DUPL    NOCP    BUFF    FORW    DROP */
    proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_pfcp_apply_action_flags,
        ett_pfcp_apply_action_flags, pfcp_apply_action_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT | BMT_NO_TFS);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.27   Downlink Data Service Information
 */
static void
dissect_pfcp_dl_data_service_inf(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 flags;

    static const int * pfcp_dl_data_service_inf_flags[] = {
        &hf_pfcp_spare_b7_b2,
        &hf_pfcp_dl_data_service_inf_b1_qfii,
        &hf_pfcp_dl_data_service_inf_b0_ppi,
        NULL
    };
    /* Octet 5  Spare   QFII    PPI */
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_dl_data_service_inf_flags,
        ett_pfcp_dl_data_service_inf, pfcp_dl_data_service_inf_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT | BMT_NO_TFS, &flags);
    offset += 1;

    /* The PPI flag in octet 5 indicates whether the Paging Policy Indication value in octet 'm' shall be present */
    if ((flags & 0x1) == 1) {
        /* m    Spare   Paging Policy Indication value
         * encoded as the DSCP in TOS (IPv4) or TC (IPv6) information received in the IP payload of the GTP-U packet
         * from the PGW (see IETF RFC 2474
         */
        proto_tree_add_item(tree, hf_pfcp_spare_b7_b6, tvb, offset, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(tree, hf_pfcp_ppi, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;
    }

    /* The QFII flag in octet 5 indicates whether the QFI value in octet 'p' shall be present */
    if ((flags & 0x2) == 2) {
        /* m    Spare   QFI value
         * encoded as the octet 5 of the QFI IE in subclause 8.2.89.
         */
        proto_tree_add_item(tree, hf_pfcp_spare_b7_b6, tvb, offset, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(tree, hf_pfcp_qfi, tvb, offset, 1, ENC_NA);
        offset++;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.28   Downlink Data Notification Delay
 */
static void
dissect_pfcp_dl_data_notification_delay(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;
    /* Octet 5 Delay Value in integer multiples of 50 millisecs, or zero */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_dl_data_notification_delay, tvb, offset, 1, ENC_BIG_ENDIAN, &value);
    offset += 1;

    proto_item_append_text(item, "%u ms", value * 50);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 * 8.2.29   DL Buffering Duration
 */
static const value_string pfcp_timer_unit_vals[] = {
    { 0, "value is incremented in multiples of 2 seconds" },
    { 1, "value is incremented in multiples of 1 minute" },
    { 2, "value is incremented in multiples of 10 minutes" },
    { 3, "value is incremented in multiples of 1 hour" },
    { 4, "value is incremented in multiples of 10 hour" },
    { 5, "values shall be interpreted as multiples of 1 minute(version 14.0.0)" },
    { 6, "values shall be interpreted as multiples of 1 minute(version 14.0.0)" },
    { 7, "value indicates that the timer is infinite" },
    { 0, NULL }
};

static void
dissect_pfcp_dl_buffering_dur(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item, guint16 length _U_, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 unit, value;

    /* Octet 5  Timer unit  Timer value */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_timer_unit, tvb, offset, 1, ENC_BIG_ENDIAN, &unit);
    proto_tree_add_item_ret_uint(tree, hf_pfcp_timer_value, tvb, offset, 1, ENC_BIG_ENDIAN, &value);
    offset++;

    if ((unit == 0) && (value == 0)) {
        proto_item_append_text(item, " Stopped");
    } else {
        switch (unit) {
        case 0:
            proto_item_append_text(item, "%u s", value * 2);
            break;
        case 1:
            proto_item_append_text(item, "%u min", value);
            break;
        case 2:
            proto_item_append_text(item, "%u min", value * 10);
            break;
        case 3:
            proto_item_append_text(item, "%u hours", value);
            break;
        case 4:
            proto_item_append_text(item, "%u hours", value * 10);
            break;
        case 7:
            proto_item_append_text(item, "Infinite (%u)", value);
            break;
            /* Value 5 and 6 */
        default:
            proto_item_append_text(item, "%u min", value);
            break;
        }
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.30   DL Buffering Suggested Packet Count
 */
static void
dissect_pfcp_dl_buffering_suggested_packet_count(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    guint32 value;
    /* Octet 5 to n+4 Packet Count Value
    * The length shall be set to 1 or 2 octets.
    */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_packet_count, tvb, 0, length, ENC_BIG_ENDIAN, &value);

    proto_item_append_text(item, "%u", value);
}
/*
 * 8.2.31   PFCPSMReq-Flags
 */
static void
dissect_pfcp_pfcpsmreq_flags(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    static const int * pfcp_pfcpsmreq_flags[] = {
        &hf_pfcp_spare_b7_b3,
        &hf_pfcp_pfcpsmreq_flags_b2_qaurr,
        &hf_pfcp_pfcpsmreq_flags_b1_sndem,
        &hf_pfcp_pfcpsmreq_flags_b0_drobu,
        NULL
    };
    /* Octet 5  Spare   Spare   Spare   Spare   Spare   QAURR   SNDEM   DROBU */
    proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_pfcp_pfcpsmreq_flags,
        ett_pfcp_pfcpsmreq, pfcp_pfcpsmreq_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.32   PFCPSRRsp-Flags
 */
static void
dissect_pfcp_pfcpsrrsp_flags(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    static const int * pfcp_pfcpsrrsp_flags[] = {
        &hf_pfcp_spare_b7_b1,
        &hf_pfcp_pfcpsrrsp_flags_b0_drobu,
        NULL
    };
    /* Octet 5  Spare   Spare   Spare   Spare   Spare   Spare   Spare   DROBU */
    proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_pfcp_pfcpsrrsp_flags,
        ett_pfcp_pfcpsrrsp, pfcp_pfcpsrrsp_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.33   Sequence Number
 */
static void
dissect_pfcp_sequence_number(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item, guint16 length _U_, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    guint32 value;
    /* Octet 5 to 8    Sequence Number */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_sequence_number, tvb, 0, 4, ENC_BIG_ENDIAN, &value);

    proto_item_append_text(item, "%u", value);

}

/*
 * 8.2.34   Metric
 */
static void
dissect_pfcp_metric(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item, guint16 length _U_, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    guint32 value;
    /* Octet 5  Metric */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_metric, tvb, 0, 1, ENC_BIG_ENDIAN, &value);

    proto_item_append_text(item, "%u", value);

}

/*
 * 8.2.35   Timer
 */
static void
dissect_pfcp_timer(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item, guint16 length _U_, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 unit, value;

    /* Octet 5  Timer unit  Timer value */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_timer_unit, tvb, offset, 1, ENC_BIG_ENDIAN, &unit);
    proto_tree_add_item_ret_uint(tree, hf_pfcp_timer_value, tvb, offset, 1, ENC_BIG_ENDIAN, &value);
    offset++;

    if ((unit == 0) && (value == 0)) {
        proto_item_append_text(item, " Stopped");
    } else {
        switch (unit) {
        case 0:
            proto_item_append_text(item, "%u s", value * 2);
            break;
        case 1:
            proto_item_append_text(item, "%u min", value);
            break;
        case 2:
            proto_item_append_text(item, "%u min", value * 10);
            break;
        case 3:
            proto_item_append_text(item, "%u hours", value);
            break;
        case 4:
            proto_item_append_text(item, "%u hours", value * 10);
            break;
        case 7:
            proto_item_append_text(item, "%u Infinite", value);
            break;
            /* Value 5 and 6 */
        default:
            proto_item_append_text(item, "%u min", value * 1);
            break;
        }
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.36   Packet Detection Rule ID (PDR ID)
 */
static int
decode_pfcp_pdr_id(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item, gint offset)
{
    guint32 rule_id;
    /* Octet 5 to 6 Rule ID*/
    proto_tree_add_item_ret_uint(tree, hf_pfcp_pdr_id, tvb, offset, 2, ENC_BIG_ENDIAN, &rule_id);
    offset += 2;

    proto_item_append_text(item, "%u", rule_id);

    return offset;
}

static void
dissect_pfcp_pdr_id(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    offset = decode_pfcp_pdr_id(tvb, pinfo, tree, item, offset);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}
/*
 * 8.2.37   F-SEID
 */
static void
dissect_pfcp_f_seid(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args)
{
    int offset = 0;
    guint64 f_seid_flags;
    address *ipv4 = NULL, *ipv6 = NULL;
    guint64 seid_cp, *seid;
    guint32 *session;

    static const int * pfcp_f_seid_flags[] = {
        &hf_pfcp_spare_b7,
        &hf_pfcp_spare_b6,
        &hf_pfcp_spare_b5,
        &hf_pfcp_spare_b4,
        &hf_pfcp_spare_b3,
        &hf_pfcp_spare_b2,
        &hf_pfcp_b1_v4,
        &hf_pfcp_b0_v6,
        NULL
    };
    /* Octet 5  Spare   Spare   Spare   Spare   Spare   Spare   V4  V6*/
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_f_seid_flags,
        ett_pfcp_f_seid_flags, pfcp_f_seid_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT | BMT_NO_TFS, &f_seid_flags);
    offset += 1;

    if ((f_seid_flags & 0x3) == 0) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_encoding_error, tvb, 0, 1);
        return;
    }
    /* Octet 6 to 13    SEID  */
    //proto_tree_add_item(tree, hf_pfcp_seid, tvb, offset, 8, ENC_BIG_ENDIAN);
    proto_tree_add_item_ret_uint64(tree, hf_pfcp_seid, tvb, offset, 8, ENC_BIG_ENDIAN, &seid_cp);
    proto_item_append_text(item, "SEID: 0x%s", tvb_bytes_to_str(wmem_packet_scope(), tvb, offset, 8));
    offset += 8;
    /* IPv4 address (if present)*/
    if ((f_seid_flags & 0x2) == 2) {
        ipv4 = wmem_new0(wmem_packet_scope(), address);
        proto_tree_add_item(tree, hf_pfcp_f_seid_ipv4, tvb, offset, 4, ENC_BIG_ENDIAN);
        proto_item_append_text(item, ", IPv4 %s", tvb_ip_to_str(tvb, offset));
        set_address_tvb(ipv4, AT_IPv4, 4, tvb, offset);
        offset += 4;
    }
    /* IPv6 address (if present)*/
    if ((f_seid_flags & 0x1) == 1) {
        ipv6 = wmem_new0(wmem_packet_scope(), address);
        proto_tree_add_item(tree, hf_pfcp_f_seid_ipv6, tvb, offset, 16, ENC_NA);
        proto_item_append_text(item, ", IPv6 %s", tvb_ip6_to_str(tvb, offset));
        set_address_tvb(ipv6, AT_IPv6, 16, tvb, offset);
        offset += 16;
    }

    if (g_pfcp_session) {
        session = (guint32 *)g_hash_table_lookup(pfcp_session_table, &pinfo->num);
        if (!session) {
            /* We save the seid so that we could assignate its corresponding session ID later */
            args->last_seid = seid_cp;
            if (!pfcp_seid_exists(seid_cp, args->seid_list)) {
                seid = wmem_new(wmem_packet_scope(), guint64);
                *seid = seid_cp;
                wmem_list_prepend(args->seid_list, seid);
            }
            if (ipv4 != NULL && !pfcp_ip_exists(*ipv4, args->ip_list)) {
                copy_address_wmem(wmem_packet_scope(), &args->last_ip, ipv4);
                wmem_list_prepend(args->ip_list, ipv4);
            }
            if (ipv6 != NULL && !pfcp_ip_exists(*ipv6, args->ip_list)) {
                copy_address_wmem(wmem_packet_scope(), &args->last_ip, ipv6);
                wmem_list_prepend(args->ip_list, ipv6);
            }
        }
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 *   8.2.38   Node ID
 */

static const value_string pfcp_node_id_type_vals[] = {

    { 0, "IPv4 address" },
    { 1, "IPv6 address" },
    { 2, "FQDN" },
    { 0, NULL }
};

static void
dissect_pfcp_node_id(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0, name_len, tmp;
    guint32 node_id_type;
    guint8 *fqdn = NULL;

    /* Octet 5    Spare Node ID Type*/
    proto_tree_add_item(tree, hf_pfcp_spare_h1, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item_ret_uint(tree, hf_pfcp_node_id_type, tvb, offset, 1, ENC_BIG_ENDIAN, &node_id_type);
    proto_item_append_text(item, "%s: ", val_to_str_const(node_id_type, pfcp_node_id_type_vals, "Unknown"));
    offset++;

    switch (node_id_type) {
        case 0:
            /* IPv4 address */
            proto_tree_add_item(tree, hf_pfcp_node_id_ipv4, tvb, offset, 4, ENC_BIG_ENDIAN);
            proto_item_append_text(item, "%s", tvb_ip_to_str(tvb, offset));
            offset += 4;
            break;
        case 1:
            /* IPv6 address */
            proto_tree_add_item(tree, hf_pfcp_node_id_ipv6, tvb, offset, 16, ENC_NA);
            proto_item_append_text(item, "%s", tvb_ip6_to_str(tvb, offset));
            offset += 16;
            break;
        case 2:
            /* FQDN, the Node ID value encoding shall be identical to the encoding of a FQDN
             * within a DNS message of section 3.1 of IETF RFC 1035 [27] but excluding the trailing zero byte.
             */
            if (length > 1) {
                name_len = tvb_get_guint8(tvb, offset);
                /* NOTE 1: The FQDN field in the IE is not encoded as a dotted string as commonly used in DNS master zone files. */
                if (name_len < 0x40) {
                    fqdn = tvb_get_string_enc(wmem_packet_scope(), tvb, offset + 1, length - 2, ENC_ASCII);
                    for (;;) {
                        if (name_len >= length - 2)
                            break;
                        tmp = name_len;
                        name_len = name_len + fqdn[tmp] + 1;
                        fqdn[tmp] = '.';
                    }
                }
                /* In case the FQDN field is incorrectly in dotted string form.*/
                else {
                    fqdn = tvb_get_string_enc(wmem_packet_scope(), tvb, offset, length - 1, ENC_ASCII);
                    proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_encoding_error, tvb, offset, length - 1);
                }
                proto_tree_add_string(tree, hf_pfcp_node_id_fqdn, tvb, offset, length - 1, fqdn);
                proto_item_append_text(item, "%s", fqdn);
                offset += length - 1;
            }
            break;
        default:
            break;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.39   PFD Contents
 */
static void
dissect_pfcp_pfd_contents(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 flags;
    guint32 len;

    static const int * pfcp_pfd_contents_flags[] = {
        &hf_pfcp_spare_b7_b4,
        &hf_pfcp_pfd_contents_flags_b3_cp,
        &hf_pfcp_pfd_contents_flags_b2_dn,
        &hf_pfcp_pfd_contents_flags_b1_url,
        &hf_pfcp_pfd_contents_flags_b0_fd,
        NULL
    };
    /* Octet 5  Spare   CP  DN  URL FD */
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_pfd_contents_flags,
        ett_pfcp_measurement_method_flags, pfcp_pfd_contents_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT, &flags);
    offset += 1;

    /* Bit 1 - FD (Flow Description): If this bit is set to "1", then the Length of Flow Description
     * and the Flow Description fields shall be present
     */
    if ((flags & 0x1) == 1) {
        /* The Flow Description field, when present, shall be encoded as an OctetString
        * as specified in subclause 6.4.3.7 of 3GPP TS 29.251
        */
        /* m to (m+1)   Length of Flow Description */
        proto_tree_add_item_ret_uint(tree, hf_pfcp_flow_desc_len, tvb, offset, 2, ENC_BIG_ENDIAN, &len);
        offset += 2;

        /* (m+2) to p   Flow Description */
        proto_tree_add_item(tree, hf_pfcp_flow_desc, tvb, offset, len, ENC_ASCII|ENC_NA);
        offset += len;
    }


    /* Bit 2 - URL (URL): If this bit is set to "1", then the Length of URL and the URL fields shall be present */
    if ((flags & 0x2) == 2) {
        /* q to (q+1)   Length of URL */
        proto_tree_add_item_ret_uint(tree, hf_pfcp_url_len, tvb, offset, 2, ENC_BIG_ENDIAN, &len);
        offset += 2;
        /* (q+2) to r   URL */
        proto_tree_add_item(tree, hf_pfcp_url, tvb, offset, len, ENC_NA);
        offset += len;

    }

    /* Bit 3 - DN (Domain Name): If this bit is set to "1", then the Length of Domain Name and
    * the Domain Name fields shall be present
    */
    if ((flags & 0x4) == 4) {
        /* s to (s+1)   Length of Domain Name */
        proto_tree_add_item_ret_uint(tree, hf_pfcp_dn_len, tvb, offset, 2, ENC_BIG_ENDIAN, &len);
        offset += 2;
        /* (s+2) to t   Domain Name */
        proto_tree_add_item(tree, hf_pfcp_dn, tvb, offset, len, ENC_NA);
        offset += len;
    }

    /* Bit 4 - CP (Custom PFD Content): If this bit is set to "1", then the Length of Custom PFD Content and
     * the Custom PFD Content fields shall be present
     */
    if ((flags & 0x8) == 8) {
        /* u to (u+1)   Length of Custom PFD Content */
        proto_tree_add_item_ret_uint(tree, hf_pfcp_cp_len, tvb, offset, 2, ENC_BIG_ENDIAN, &len);
        offset += 2;
        /* (u+2) to v   Custom PFD Content */
        proto_tree_add_item(tree, hf_pfcp_cp, tvb, offset, len, ENC_NA);
        offset += len;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.40   Measurement Method
 */
static void
dissect_pfcp_measurement_method(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    static const int * pfcp_measurement_method_flags[] = {
        &hf_pfcp_spare_b7_b3,
        &hf_pfcp_measurement_method_flags_b2_event,
        &hf_pfcp_measurement_method_flags_b1_volume,
        &hf_pfcp_measurement_method_flags_b0_durat,
        NULL
    };
    /* Octet 5  Spare   Spare   Spare   Spare   Spare   EVENT   VOLUM   DURAT */
    proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_pfcp_measurement_method_flags,
        ett_pfcp_measurement_method_flags, pfcp_measurement_method_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.41   Usage Report Trigger
 */
static void
dissect_pfcp_usage_report_trigger(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    static const int * pfcp_usage_report_trigger_o5_flags[] = {
        &hf_pfcp_usage_report_trigger_o5_b7_immer,
        &hf_pfcp_usage_report_trigger_o5_b6_droth,
        &hf_pfcp_usage_report_trigger_o5_b5_stopt,
        &hf_pfcp_usage_report_trigger_o5_b4_start,
        &hf_pfcp_usage_report_trigger_o5_b3_quhti,
        &hf_pfcp_usage_report_trigger_o5_b2_timth,
        &hf_pfcp_usage_report_trigger_o5_b1_volth,
        &hf_pfcp_usage_report_trigger_o5_b0_perio,
        NULL
    };
    /* Octet 5  IMMER   DROTH   STOPT   START   QUHTI   TIMTH   VOLTH   PERIO */
    proto_tree_add_bitmask_list(tree, tvb, offset, 1, pfcp_usage_report_trigger_o5_flags, ENC_BIG_ENDIAN);
    offset++;

    if (offset == length) {
        return;
    }

    static const int * pfcp_usage_report_trigger_o6_flags[] = {
        &hf_pfcp_usage_report_trigger_o6_b7_eveth,
        &hf_pfcp_usage_report_trigger_o6_b6_macar,
        &hf_pfcp_usage_report_trigger_o6_b5_envcl,
        &hf_pfcp_usage_report_trigger_o6_b4_monit,
        &hf_pfcp_usage_report_trigger_o6_b3_termr,
        &hf_pfcp_usage_report_trigger_o6_b2_liusa,
        &hf_pfcp_usage_report_trigger_o6_b1_timqu,
        &hf_pfcp_usage_report_trigger_o6_b0_volqu,
        NULL
    };
    /* Octet 6  EVETH   MACAR   ENVCL   MONIT   TERMR   LIUSA   TIMQU   VOLQU */
    proto_tree_add_bitmask_list(tree, tvb, offset, 1, pfcp_usage_report_trigger_o6_flags, ENC_BIG_ENDIAN);
    offset++;

    if (offset == length) {
        return;
    }

    static const int * pfcp_usage_report_trigger_o7_flags[] = {
        &hf_pfcp_usage_report_trigger_o7_b0_evequ,
        NULL
    };
    /* Octet 7  Spare   Spare   Spare   Spare   Spare   Spare   Spare   EVEQU */
    proto_tree_add_bitmask_list(tree, tvb, offset, 1, pfcp_usage_report_trigger_o7_flags, ENC_BIG_ENDIAN);
    offset++;

    if (offset == length) {
        return;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.42   Measurement Period
 */
static void
dissect_pfcp_measurement_period(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;
    /* 5 to 8   Measurement Period*/
    proto_tree_add_item_ret_uint(tree, hf_pfcp_measurement_period, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    proto_item_append_text(item, "%u", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 * 8.2.43   Fully qualified PDN Connection Set Identifier (FQ-CSID)
 */
static const value_string pfcp_fq_csid_node_id_type_vals[] = {

    { 0, "Node-Address is a global unicast IPv4 address" },
    { 1, "Node-Address is a global unicast IPv6 address" },
    { 2, "Node-Address is a 4 octets long field" },
    { 0, NULL }
};

static void
dissect_pfcp_fq_csid(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 node_id_type, num_csid;

    /* Octet 5  FQ-CSID Node-ID Type    Number of CSIDs= m*/
    proto_tree_add_item_ret_uint(tree, hf_pfcp_fq_csid_node_id_type, tvb, offset, 1, ENC_BIG_ENDIAN, &node_id_type);
    proto_tree_add_item_ret_uint(tree, hf_pfcp_num_csid, tvb, offset, 1, ENC_BIG_ENDIAN, &num_csid);
    offset++;

    /* 6 to p   Node-Address  */
    switch (node_id_type) {
    case 0:
        /* 0    indicates that Node-Address is a global unicast IPv4 address and p = 9 */
        proto_tree_add_item(tree, hf_pfcp_fq_csid_node_id_ipv4, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
        break;
    case 1:
        /* 1    indicates that Node-Address is a global unicast IPv6 address and p = 21 */
        proto_tree_add_item(tree, hf_pfcp_fq_csid_node_id_ipv6, tvb, offset, 16, ENC_NA);
        offset += 16;
        break;
    case 2:
        /* 2    indicates that Node-Address is a 4 octets long field with a 32 bit value stored in network order, and p= 9
         *      Most significant 20 bits are the binary encoded value of (MCC * 1000 + MNC).
         *      Least significant 12 bits is a 12 bit integer assigned by an operator to an MME, SGW-C, SGW-U, PGW-C or PGW-U
         */
        proto_tree_add_item(tree, hf_pfcp_fq_csid_node_id_mcc_mnc, tvb, offset, 4, ENC_BIG_ENDIAN);
        proto_tree_add_item(tree, hf_pfcp_fq_csid_node_id_int, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
        break;
    default:
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
        break;
    }

    while (num_csid > 0) {
        proto_tree_add_item(tree, hf_pfcp_fq_csid, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
        num_csid--;
    }
    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.44   Volume Measurement
 */
static void
dissect_pfcp_volume_measurement(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 flags;

    static const int * pfcp_volume_measurement_flags[] = {
        &hf_pfcp_spare_b7_b3,
        &hf_pfcp_volume_measurement_b2_dlvol,
        &hf_pfcp_volume_measurement_b1_ulvol,
        &hf_pfcp_volume_measurement_b0_tovol,
        NULL
    };
    /* Octet 5  Spare   DLVOL   ULVOL   TOVOL*/
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_volume_measurement,
        ett_pfcp_volume_measurement, pfcp_volume_measurement_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT, &flags);
    offset += 1;

    /* Bit 1 - TOVOL: If this bit is set to "1", then the Total Volume field shall be present*/
    if ((flags & 0x1) == 1) {
        /* m to (m+7)   Total Volume */
        proto_tree_add_item(tree, hf_pfcp_vol_meas_tovol, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
    }
    /* Bit 2 - ULVOL: If this bit is set to "1", then the Total Volume field shall be present*/
    if ((flags & 0x2) == 2) {
        /* p to (p+7)   Uplink Volume */
        proto_tree_add_item(tree, hf_pfcp_vol_meas_ulvol, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
    }
    /* Bit 3 - DLVOL: If this bit is set to "1", then the Total Volume field shall be present*/
    if ((flags & 0x4) == 4) {
        /*q to (q+7)    Downlink Volume */
        proto_tree_add_item(tree, hf_pfcp_vol_meas_dlvol, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.45   Duration Measurement
 */
static void
dissect_pfcp_duration_measurement(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;
    /* 5 to 8   Duration value*/
    proto_tree_add_item_ret_uint(tree, hf_pfcp_duration_measurement, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    proto_item_append_text(item, "%u s", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}
/*
 * 8.2.46   Time of First Packet
 */
static void
dissect_pfcp_time_of_first_packet(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    const gchar *time_str;

    /* Octets 5 to 8 shall be encoded in the same format as the first four octets of the 64-bit timestamp
     * format as defined in section 6 of IETF RFC 5905
     */

    time_str = tvb_ntp_fmt_ts_sec(tvb, 0);
    proto_tree_add_string(tree, hf_pfcp_time_of_first_packet, tvb, offset, 4, time_str);
    proto_item_append_text(item, "%s", time_str);
    offset += 4;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}
/*
 * 8.2.47   Time of Last Packet
 */
static void
dissect_pfcp_time_of_last_packet(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    const gchar *time_str;

    /* Octets 5 to 8 shall be encoded in the same format as the first four octets of the 64-bit timestamp
    * format as defined in section 6 of IETF RFC 5905
    */

    time_str = tvb_ntp_fmt_ts_sec(tvb, 0);
    proto_tree_add_string(tree, hf_pfcp_time_of_last_packet, tvb, offset, 4, time_str);
    proto_item_append_text(item, "%s", time_str);
    offset += 4;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}
/*
 * 8.2.48   Quota Holding Time
 */
static void
dissect_pfcp_quota_holding_time(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;
    /* Octet 5 to 8    Time Quota value
    * TThe Time Quota value shall be encoded as an Unsigned32 binary integer value. It contains a duration in seconds
    */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_quota_holding_time, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    proto_item_append_text(item, "%u s", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.49   Dropped DL Traffic Threshold
 */
static void
dissect_pfcp_dropped_dl_traffic_threshold(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 flags_val;

    static const int * pfcp_dropped_dl_traffic_threshold_flags[] = {
        &hf_pfcp_dropped_dl_traffic_threshold_b1_dlby,
        &hf_pfcp_dropped_dl_traffic_threshold_b0_dlpa,
        NULL
    };
    /* Octet 5  Spare   DLBY    DLPA*/
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_dropped_dl_traffic_threshold,
        ett_pfcp_dropped_dl_traffic_threshold, pfcp_dropped_dl_traffic_threshold_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT, &flags_val);
    offset += 1;

    if ((flags_val & 0x1) == 1) {
        /* m to (m+7)   Downlink Packets
        * DLPA: If this bit is set to "1", then the Downlink Packets field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_downlink_packets, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
    }

    if ((flags_val & 0x2) == 2) {
        /* o to (o+7)   Number of Bytes of Downlink Data
        * DLBY: If this bit is set to "1", then the Number of Bytes of Downlink Data field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_bytes_downlink_data, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}
/*
 * 8.2.50   Volume Quota
 */
static void
dissect_pfcp_volume_quota(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 flags_val;

    static const int * pfcp_volume_quota_flags[] = {
        &hf_pfcp_spare_b7_b3,
        &hf_pfcp_volume_quota_b2_dlvol,
        &hf_pfcp_volume_quota_b1_ulvol,
        &hf_pfcp_volume_quota_b0_tovol,
        NULL
    };
    /* Octet 5  Spare   DLVOL   ULVOL   TOVOL*/
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_volume_quota,
        ett_pfcp_volume_quota, pfcp_volume_quota_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT, &flags_val);
    offset += 1;

    /* The Total Volume, Uplink Volume and Downlink Volume fields shall be encoded as an Unsigned64 binary integer value.
    * They shall contain the total, uplink or downlink number of octets respectively.
    */
    if ((flags_val & 0x1) == 1) {
        /* m to (m+7)   Total Volume
        * TOVOL: If this bit is set to "1", then the Total Volume field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_volume_quota_tovol, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
    }
    if ((flags_val & 0x2) == 2) {
        /* p to (p+7)    Uplink Volume
        * ULVOL: If this bit is set to "1", then the Uplink Volume field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_volume_quota_ulvol, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
    }
    if ((flags_val & 0x4) == 4) {
        /* q to (q+7)   Downlink Volume
        * DLVOL: If this bit is set to "1", then the Downlink Volume field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_volume_quota_dlvol, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}
/*
 * 8.2.51   Time Quota
 */
static void
dissect_pfcp_time_quota(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;
    /* Octet 5 to 8    Time Quota value
    * TThe Time Quota value shall be encoded as an Unsigned32 binary integer value. It contains a duration in seconds
    */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_time_quota, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    proto_item_append_text(item, "%u s", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.52   Start Time
 */
static void
dissect_pfcp_start_time(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    const gchar *time_str;
    int offset = 0;

    /* The Start Time field shall contain a UTC time. Octets 5 to 8 are encoded in the same format as
    * the first four octets of the 64-bit timestamp format as defined in section 6 of IETF RFC 5905 [26].
    */
    time_str = tvb_ntp_fmt_ts_sec(tvb, 0);
    proto_tree_add_string(tree, hf_pfcp_start_time, tvb, offset, 4, time_str);
    proto_item_append_text(item, "%s", time_str);
    offset += 4;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.53   End Time
 */
static void
dissect_pfcp_end_time(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    const gchar *time_str;
    int offset = 0;

    /* The End Time field shall contain a UTC time. Octets 5 to 8 are encoded in the same format as
    * the first four octets of the 64-bit timestamp format as defined in section 6 of IETF RFC 5905 [26].
    */
    time_str = tvb_ntp_fmt_ts_sec(tvb, 0);
    proto_tree_add_string(tree, hf_pfcp_end_time, tvb, offset, 4, time_str);
    proto_item_append_text(item, "%s", time_str);
    offset += 4;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.54   URR ID
 */
static int
decode_pfcp_urr_id(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint offset)
{
    guint32 urr_id;
    guint8 urr_id_flag;
    /* Octet 5 to 8 URR ID value
    * The bit 8 of octet 5 is used to indicate if the Rule ID is dynamically allocated by the CP function
    * or predefined in the UP function. If set to 0, it indicates that the Rule is dynamically provisioned
    * by the CP Function. If set to 1, it indicates that the Rule is predefined in the UP Function
    */
    urr_id_flag = tvb_get_guint8(tvb, offset) & 0x80;

    proto_tree_add_item(tree, hf_pfcp_urr_id_flg, tvb, offset, 4, ENC_BIG_ENDIAN);
    proto_tree_add_item_ret_uint(tree, hf_pfcp_urr_id, tvb, offset, 4, ENC_BIG_ENDIAN, &urr_id);
    offset += 4;

    proto_item_append_text(item, "%s %u",
        ((urr_id_flag)? pfcp_id_predef_dynamic_tfs.true_string : pfcp_id_predef_dynamic_tfs.false_string),
        (urr_id & 0x7fffffff));

    return offset;
}

static void
dissect_pfcp_urr_id(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    offset = decode_pfcp_urr_id(tvb, pinfo, tree, item, offset);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.55   Linked URR ID IE
 */
static void
dissect_pfcp_linked_urr_id(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;
    /* Octet 5 to 8 Linked URR ID value
    * The Linked URR ID value shall be encoded as an Unsigned32 binary integer value
    */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_linked_urr_id, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    proto_item_append_text(item, "%u", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.56   Outer Header Creation
 */

static const value_string pfcp_outer_hdr_desc_vals[] = {

    { 0x0100, "GTP-U/UDP/IPv4 " },
    { 0x0200, "GTP-U/UDP/IPv6 " },
    { 0x0300, "GTP-U/UDP/IPv4/IPv6 " },
    { 0x0400, "UDP/IPv4 " },
    { 0x0800, "UDP/IPv6 " },
    { 0x0C00, "UDP/IPv4/IPv6 " },
    { 0x1000, "IPv4 " },
    { 0x2000, "IPv6 " },
    { 0x3000, "IPv4/IPv6 " },
    { 0x4000, "C-TAG " },
    { 0x8000, "S-TAG " },
    { 0, NULL }
};

static void
dissect_pfcp_outer_header_creation(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;

    /* Octet 5  Outer Header Creation Description */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_outer_hdr_desc, tvb, offset, 2, ENC_BIG_ENDIAN, &value);
    offset += 2;

    /* m to (m+3)   TEID
     * The TEID field shall be present if the Outer Header Creation Description requests the creation of a GTP-U header.
     * Otherwise it shall not be present
     */
    if ((value & 0x0100) || (value & 0x0200)) {
        proto_tree_add_item(tree, hf_pfcp_outer_hdr_creation_teid, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
    }

    /*
    * p to (p+3)   IPv4
    * The IPv4 Address field shall be present if the Outer Header Creation Description requests the creation of a IPv4 header
    */
    if ((value & 0x0100) || (value & 0x0400) || (value & 0x1000)) {
        proto_tree_add_item(tree, hf_pfcp_outer_hdr_creation_ipv4, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
    }

    /*
    * q to (q+15)   IPv6
    * The IPv6 Address field shall be present if the Outer Header Creation Description requests the creation of a IPv6 header
    */
    if ((value & 0x0200) || (value & 0x0800) || (value & 0x2000)) {
        proto_tree_add_item(tree, hf_pfcp_outer_hdr_creation_ipv6, tvb, offset, 16, ENC_NA);
        offset += 16;
    }

    /*
    * r to (r+1)   Port Number
    * The Port Number field shall be present if the Outer Header Creation Description requests the creation of a UDP/IP header
    */
    if ((value & 0x0400) || (value & 0x0800)) {
        proto_tree_add_item(tree, hf_pfcp_outer_hdr_creation_port, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
    }

    /*
    * t to (t+2)   C-TAG
    * The C-TAG field shall be present if the Outer Header Creation Description requests the setting of the C-Tag in Ethernet packet
    */
    if (value & 0x4000) {
        offset = decode_pfcp_c_tag(tvb, pinfo, tree, item, offset);
    }

    /*
    * u to (u+2)   S-TAG
    * The S-TAG field shall be present if the Outer Header Creation Description requests the setting of the S-Tag in Ethernet packet
    */
    if (value & 0x8000) {
        offset = decode_pfcp_s_tag(tvb, pinfo, tree, item, offset);
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.57   BAR ID
 */
static int
decode_pfcp_bar_id(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item, guint16 offset)
{
    guint32 value;
    /* Octet 5 BAR ID value
    * The BAR ID value shall be encoded as a binary integer value
    */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_bar_id, tvb, offset, 1, ENC_BIG_ENDIAN, &value);
    offset++;
    proto_item_append_text(item, "%u", value);

    return offset;
}
static void
dissect_pfcp_bar_id(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    offset = decode_pfcp_bar_id(tvb, pinfo, tree, item, offset);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.58   CP Function Features
 */
static void
dissect_pfcp_cp_function_features(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    static const int * pfcp_cp_function_features_flags[] = {
        &hf_pfcp_cp_function_features_b1_ovrl,
        &hf_pfcp_cp_function_features_b0_load,
        NULL
    };
    /* Octet 5
     * 5/1 LOAD
     * 5/2 OVRL
     */
    proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_pfcp_cp_function_features,
        ett_pfcp_cp_function_features, pfcp_cp_function_features_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.59   Usage Information
 */
static void
dissect_pfcp_usage_information(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    static const int * pfcp_usage_information_flags[] = {
        &hf_pfcp_spare_h1,
        &hf_pfcp_usage_information_b3_ube,
        &hf_pfcp_usage_information_b2_uae,
        &hf_pfcp_usage_information_b1_aft,
        &hf_pfcp_usage_information_b0_bef,
        NULL
    };
    /* Octet 5  Spare   UBE UAE AFT BEF */
    proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_pfcp_usage_information,
        ett_pfcp_usage_information, pfcp_usage_information_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.60   Application Instance ID
 */
static void
dissect_pfcp_application_instance_id(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    /* Octet 5 5 to (n+4)   Application Instance Identifier
     * The Application Instance Identifier shall be encoded as an OctetString (see 3GPP TS 29.212)
     */
    if (tvb_ascii_isprint(tvb, offset, length))
    {
        const guint8* string_value;
        proto_tree_add_item_ret_string(tree, hf_pfcp_application_instance_id_str, tvb, offset, length, ENC_ASCII | ENC_NA, wmem_packet_scope(), &string_value);
        proto_item_append_text(item, "%s", string_value);
    }
    else
    {
        proto_tree_add_item(tree, hf_pfcp_application_instance_id, tvb, offset, length, ENC_NA);
    }
}

/*
 * 8.2.61   Flow Information
 */
static const value_string pfcp_flow_dir_vals[] = {
    { 0, "Unspecified" },
    { 1, "Downlink (traffic to the UE)" },
    { 2, "Uplink (traffic from the UE)" },
    { 3, "Bidirectional" },
    { 0, NULL }
};

static void
dissect_pfcp_flow_inf(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 len;
    /* Octet 5 Spare    Flow Direction */
    proto_tree_add_item(tree, hf_pfcp_spare_b7_b3, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_pfcp_flow_dir, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /* 6 to 7   Length of Flow Description */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_flow_desc_len, tvb, offset, 2, ENC_BIG_ENDIAN, &len);
    offset += 2;
    /* Flow Description
    * The Flow Description field, when present, shall be encoded as an OctetString
    * as specified in subclause 5.4.2 of 3GPP TS 29.212
    */
    proto_tree_add_item(tree, hf_pfcp_flow_desc, tvb, offset, len, ENC_ASCII|ENC_NA);
    offset += len;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.62   UE IP Address
 */
static const true_false_string pfcp_ue_ip_add_sd_flag_vals = {
    "Destination IP address",
    "Source IP address",
};

static void
dissect_pfcp_ue_ip_address(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 ue_ip_address_flags;

    static const int * pfcp_ue_ip_address_flags[] = {
        &hf_pfcp_spare_b7_b4,
        &hf_pfcp_ue_ip_address_flag_b3_v6d,
        &hf_pfcp_ue_ip_address_flag_b2_sd,
        &hf_pfcp_ue_ip_address_flag_b1_v4,
        &hf_pfcp_ue_ip_address_flag_b0_v6,
        NULL
    };
    /* Octet 5  Spare   IPv6D   S/D     V4      V6*/
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_ue_ip_address_flags,
        ett_pfcp_ue_ip_address_flags, pfcp_ue_ip_address_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT | BMT_NO_TFS, &ue_ip_address_flags);
    offset += 1;

    /* IPv4 address (if present)*/
    if ((ue_ip_address_flags & 0x2) == 2) {
        proto_tree_add_item(tree, hf_pfcp_ue_ip_addr_ipv4, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
    }
    /* IPv6 address (if present)*/
    if ((ue_ip_address_flags & 0x1) == 1) {
        proto_tree_add_item(tree, hf_pfcp_ue_ip_add_ipv6, tvb, offset, 16, ENC_NA);
        offset += 16;
    }
    /* IPv6 Prefix Delegation Bits (if present)*/
    if ((ue_ip_address_flags & 0x8) == 8) {
        proto_tree_add_item(tree, hf_pfcp_ue_ip_add_ipv6_prefix, tvb, offset, 1, ENC_NA);
        offset += 1;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.63   Packet Rate
 */
static const value_string pfcp_pr_time_unit_vals[] = {
    { 0, "Minute" },
    { 1, "6 minutes" },
    { 2, "Hour" },
    { 3, "Day" },
    { 4, "Week" },
    { 0, NULL }
};

static void
dissect_pfcp_packet_rate(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 flags;

    static const int * pfcp_packet_rate_flags[] = {
        &hf_pfcp_spare_b7_b2,
        &hf_pfcp_packet_rate_b1_dlpr,
        &hf_pfcp_packet_rate_b0_ulpr,
        NULL
    };
    /* Octet 5  Spare   DLPR    ULPR */
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_packet_rate,
        ett_pfcp_packet_rate, pfcp_packet_rate_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT, &flags);
    offset += 1;

    /* Bit 1 - ULPR (Uplink Packet Rate): If this bit is set to "1", then octets m to (m+2) shall be present */
    if ((flags & 0x1) == 1) {
        /* m */
        proto_tree_add_item(tree, hf_pfcp_spare_b7_b3, tvb, offset, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(tree, hf_pfcp_ul_time_unit, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset += 1;
        /* (m+1) to (m+2)   Maximum Uplink Packet Rate */
        proto_tree_add_item(tree, hf_pfcp_max_ul_pr, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
    }
    /* Bit 2 - DLPR (Downlink Packet Rate): If this bit is set to "1", then octets p to (p+2) shall be present*/
    if ((flags & 0x2) == 2) {
        proto_tree_add_item(tree, hf_pfcp_spare_b7_b3, tvb, offset, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(tree, hf_pfcp_dl_time_unit, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset += 1;
        /* (m+1) to (m+2)   Maximum Uplink Packet Rate */
        proto_tree_add_item(tree, hf_pfcp_max_dl_pr, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.64   Outer Header Removal
 */
static const value_string pfcp_out_hdr_desc_vals[] = {
    { 0, "GTP-U/UDP/IPv4" },
    { 1, "GTP-U/UDP/IPv6" },
    { 2, "UDP/IPv4" },
    { 3, "UDP/IPv6 " },
    { 4, "IPv4" },
    { 5, "IPv6 " },
    { 6, "GTP-U/UDP/IP" },
    { 7, "VLAN S-TAG" },
    { 8, "S-TAG and C-TAG" },
    { 0, NULL }
};

static void
dissect_pfcp_outer_hdr_rem(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;
    /* Octet 5 to (n+4) Application Identifier
    * The Application Identifier shall be encoded as an OctetString (see 3GPP TS 29.212)
    */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_out_hdr_desc, tvb, offset, 1, ENC_BIG_ENDIAN, &value);
    offset++;

    proto_item_append_text(item, "%s", val_to_str_const(value, pfcp_out_hdr_desc_vals, "Unknown"));

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}
 /*
 * 8.2.65   Recovery Time Stamp
 */

static void
dissect_pfcp_recovery_time_stamp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    const gchar *time_str;
    int offset = 0;

    /* indicates the UTC time when the node started. Octets 5 to 8 are encoded in the same format as
    * the first four octets of the 64-bit timestamp format as defined in section 6 of IETF RFC 5905 [26].
    */
    time_str = tvb_ntp_fmt_ts_sec(tvb, 0);
    proto_tree_add_string(tree, hf_pfcp_recovery_time_stamp, tvb, offset, 4, time_str);
    proto_item_append_text(item, "%s", time_str);
    offset += 4;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.66   DL Flow Level Marking
 */
static void
dissect_pfcp_dl_flow_level_marking(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 flags_val;

    static const int * pfcp_dl_flow_level_marking_flags[] = {
        &hf_pfcp_spare_b7_b2,
        &hf_pfcp_dl_flow_level_marking_b1_sci,
        &hf_pfcp_dl_flow_level_marking_b0_ttc,
        NULL
    };
    /* Octet 5  Spare   SCI TTC*/
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_dl_flow_level_marking,
        ett_pfcp_pfcp_dl_flow_level_marking, pfcp_dl_flow_level_marking_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT, &flags_val);
    offset += 1;

    /* Bit 1 - TTC (ToS/Traffic Class): If this bit is set to "1",
     * then the ToS/Traffic Class field shall be present
     */
    if ((flags_val & 0x1) == 1) {
        /* m to (m+1)    ToS/Traffic Class
        * The ToS/Traffic Class shall be encoded on two octets as an OctetString.
        * The first octet shall contain the IPv4 Type-of-Service or the IPv6 Traffic-Class field and
        * the second octet shall contain the ToS/Traffic Class mask field
        */
        proto_tree_add_item(tree, hf_pfcp_traffic_class, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset += 1;
        proto_tree_add_item(tree, hf_pfcp_traffic_mask, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset += 1;
    }
    /* SCI (Service Class Indicator): If this bit is set to "1",
     * then the Service Class Indicator field shall be present
     */
    if ((flags_val & 0x2) == 2) {
        /* Octets p and (p+1) of the Service Class Indicator field, when present,
        * shall be encoded respectively as octets 2 and 3 of the Service Class Indicator Extension Header
        * specified in Figure 5.2.2.3-1 of 3GPP TS 29.281
        */
        proto_tree_add_item(tree, hf_pfcp_sci, tvb, offset, 2, ENC_BIG_ENDIAN);
        offset += 2;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.67   Header Enrichment
 */
static const value_string pfcp_header_type_vals[] = {
    { 0, "HTTP" },
    { 0, NULL }
};

static void
dissect_pfcp_header_enrichment(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 len;

    /* Octet 5 Spare    Header Type
    */
    proto_tree_add_item(tree, hf_pfcp_spare_b7_b5, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_pfcp_header_type, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset++;

    /* 6    Length of Header Field Name */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_hf_len, tvb, offset, 1, ENC_BIG_ENDIAN, &len);
    offset++;

    /* 7 to m Header Field Name
     * Header Field Name shall be encoded as an OctetString
     */
    if (tvb_ascii_isprint(tvb, offset, len))
        proto_tree_add_item(tree, hf_pfcp_hf_name_str, tvb, offset, len, ENC_ASCII | ENC_NA);
    else
        proto_tree_add_item(tree, hf_pfcp_hf_name, tvb, offset, len, ENC_NA);
    offset+= len;

    /* p    Length of Header Field Value*/
    proto_tree_add_item_ret_uint(tree, hf_pfcp_hf_val_len, tvb, offset, 1, ENC_BIG_ENDIAN, &len);
    offset++;

    /* (p+1) to q   Header Field Value */
    if (tvb_ascii_isprint(tvb, offset, len))
        proto_tree_add_item(tree, hf_pfcp_hf_val_str, tvb, offset, len, ENC_ASCII | ENC_NA);
    else
        proto_tree_add_item(tree, hf_pfcp_hf_val, tvb, offset, len, ENC_NA);
    offset += len;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 * 8.2.68   Measurement Information
 */
static void
dissect_pfcp_measurement_info(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    static const int * pfcp_measurement_info_flags[] = {
        &hf_pfcp_spare_b7_b4,
        &hf_pfcp_measurement_info_b3_istm,
        &hf_pfcp_measurement_info_b2_radi,
        &hf_pfcp_measurement_info_b1_inam,
        &hf_pfcp_measurement_info_b0_mbqe,
        NULL
    };
    /* Octet 5  Spare   ISTM    INAM    MBQE */
    proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_pfcp_measurement_info,
        ett_pfcp_measurement_info, pfcp_measurement_info_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.69   Node Report Type
 */
static void
dissect_pfcp_node_report_type(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    static const int * pfcp_node_report_type_flags[] = {
        &hf_pfcp_spare_b7_b1,
        &hf_pfcp_node_report_type_b0_upfr,
        NULL
    };
    /* Octet 5  Spare   INAM    MBQE */
    proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_pfcp_node_report_type,
        ett_pfcp_node_report_type, pfcp_node_report_type_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.70   Remote GTP-U Peer
 */
static void
dissect_pfcp_remote_gtp_u_peer(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 flags;

    static const int * pfcp_remote_gtp_u_peer_flags[] = {
        &hf_pfcp_spare_b7_b2,
        &hf_pfcp_remote_gtp_u_peer_flags_b1_v4,
        &hf_pfcp_remote_gtp_u_peer_flags_b0_v6,
        NULL
    };
    /* Octet 5  Spare   V4  V6*/
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_remote_gtp_u_peer_flags,
        ett_pfcp_remote_gtp_u_peer, pfcp_remote_gtp_u_peer_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT | BMT_NO_TFS, &flags);
    offset += 1;

    /* IPv4 address (if present)*/
    if ((flags & 0x2) == 2) {
        proto_tree_add_item(tree, hf_pfcp_remote_gtp_u_peer_ipv4, tvb, offset, 4, ENC_BIG_ENDIAN);
        proto_item_append_text(item, "IPv4 %s ", tvb_ip_to_str(tvb, offset));
        offset += 4;
    }
    /* IPv6 address (if present)*/
    if ((flags & 0x1) == 1) {
        proto_tree_add_item(tree, hf_pfcp_remote_gtp_u_peer_ipv6, tvb, offset, 16, ENC_NA);
        proto_item_append_text(item, "IPv6 %s ", tvb_ip6_to_str(tvb, offset));
        offset += 16;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.71   UR-SEQN
 */
static void
dissect_pfcp_ur_seqn(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length _U_, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    guint value;

    /* 5 to 8   UR-SEQN
    * The UR-SEQN value shall be encoded as an Unsigned32 binary integer value
    */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_ur_seqn, tvb, 0, 4, ENC_BIG_ENDIAN, &value);

    proto_item_append_text(item, "%u", value);


}

/*
 * 8.2.72   Activate Predefined Rules
 */
static void
dissect_pfcp_act_predef_rules(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    /* Octet 5 to (n+4) Predefined Rules Name
    * The Predefined Rules Name field shall be encoded as an OctetString
    */
    proto_tree_add_item(tree, hf_pfcp_predef_rules_name, tvb, offset, length, ENC_NA);
}
/*
 * 8.2.73   Deactivate Predefined Rules
 */
static void
dissect_pfcp_deact_predef_rules(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    /* Octet 5 to (n+4) Predefined Rules Name
    * The Predefined Rules Name field shall be encoded as an OctetString
    */
    proto_tree_add_item(tree, hf_pfcp_predef_rules_name, tvb, offset, length, ENC_NA);
}
/*
 * 8.2.74   FAR ID
 */
static int
decode_pfcp_far_id(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, gint offset)
{
    guint32 far_id;
    guint8 far_id_flag;
    /* Octet 5 to 8 FAR ID value
     * The bit 8 of octet 5 is used to indicate if the Rule ID is dynamically allocated
     * by the CP function or predefined in the UP function. If set to 0, it indicates that
     * the Rule is dynamically provisioned by the CP Function. If set to 1, it indicates that
     * the Rule is predefined in the UP Function.
     */
    far_id_flag = tvb_get_guint8(tvb,offset) & 0x80;

    proto_tree_add_item(tree, hf_pfcp_far_id_flg, tvb, offset, 4, ENC_BIG_ENDIAN);
    proto_tree_add_item_ret_uint(tree, hf_pfcp_far_id, tvb, offset, 4, ENC_BIG_ENDIAN, &far_id);
    offset += 4;

    proto_item_append_text(item, "%s %u",
        ((far_id_flag)? pfcp_id_predef_dynamic_tfs.true_string : pfcp_id_predef_dynamic_tfs.false_string),
        (far_id & 0x7fffffff));

    return offset;
}

static void
dissect_pfcp_far_id(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    offset = decode_pfcp_far_id(tvb, pinfo, tree, item, offset);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.75   QER ID
 */
static int
decode_pfcp_qer_id(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint offset)
{
    guint32 qer_id;
    guint8 qer_id_flag;
    /* Octet 5 to 8 QER ID value
    * The bit 8 of octet 5 is used to indicate if the Rule ID is dynamically allocated by the CP function
    * or predefined in the UP function. If set to 0, it indicates that the Rule is dynamically provisioned
    * by the CP Function. If set to 1, it indicates that the Rule is predefined in the UP Function
    */
    qer_id_flag = tvb_get_guint8(tvb, offset) & 0x80;

    proto_tree_add_item(tree, hf_pfcp_qer_id_flg, tvb, offset, 4, ENC_BIG_ENDIAN);
    proto_tree_add_item_ret_uint(tree, hf_pfcp_qer_id, tvb, offset, 4, ENC_BIG_ENDIAN, &qer_id);
    offset += 4;

    proto_item_append_text(item, "%s %u",
        ((qer_id_flag)? pfcp_id_predef_dynamic_tfs.true_string : pfcp_id_predef_dynamic_tfs.false_string),
        (qer_id & 0x7fffffff));

    return offset;
}
static void
dissect_pfcp_qer_id(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    offset = decode_pfcp_qer_id(tvb, pinfo, tree, item, offset);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.76   OCI Flags
 */
static void
dissect_pfcp_oci_flags(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    static const int * pfcp_oci_flags_flags[] = {
        &hf_pfcp_spare_b7_b1,
        &hf_pfcp_oci_flags_b0_aoci,
        NULL
    };
    /* Octet 5  Spare   AOCI */
    proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_pfcp_oci_flags,
        ett_pfcp_oci_flags, pfcp_oci_flags_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.77   PFCP Association Release Request
 */
static void
dissect_pfcp_pfcp_assoc_rel_req(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    static const int * pfcp_pfcp_assoc_rel_req_flags[] = {
        &hf_pfcp_spare_b7_b1,
        &hf_pfcp_pfcp_assoc_rel_req_b0_sarr,
        NULL
    };
    /* Octet 5  Spare    SARR */
    proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_pfcp_pfcp_assoc_rel_req_flags,
        ett_pfcp_assoc_rel_req_flags, pfcp_pfcp_assoc_rel_req_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.78   Graceful Release Period
 */
static void
dissect_pfcp_graceful_release_period(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item, guint16 length _U_, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 unit, value;

    /* Octet 5  Timer unit  Timer value */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_timer_unit, tvb, offset, 1, ENC_BIG_ENDIAN, &unit);
    proto_tree_add_item_ret_uint(tree, hf_pfcp_timer_value, tvb, offset, 1, ENC_BIG_ENDIAN, &value);
    offset++;

    if ((unit == 0) && (value == 0)) {
        proto_item_append_text(item, " Stopped");
    } else {
        switch (unit) {
        case 0:
            proto_item_append_text(item, "%u s", value * 2);
            break;
        case 1:
            proto_item_append_text(item, "%u min", value);
            break;
        case 2:
            proto_item_append_text(item, "%u min", value * 10);
            break;
        case 3:
            proto_item_append_text(item, "%u hours", value);
            break;
        case 4:
            proto_item_append_text(item, "%u hours", value * 10);
            break;
        case 7:
            proto_item_append_text(item, "%u Infinite", value);
            break;
            /* Value 5 and 6 */
        default:
            proto_item_append_text(item, "%u min", value * 1);
            break;
        }
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}
/*
 * 8.2.79    PDN Type
 */
static const value_string pfcp_pdn_type_vals[] = {
    { 0, "Reserved" },
    { 1, "IPv4" },
    { 2, "IPv6" },
    { 3, "IPv4V6" },
    { 4, "Non-IP" },
    { 5, "Ethernet" },
    { 0, NULL }
};

static void
dissect_pfcp_pdn_type(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;
    /* Octet 5  Application Identifier
    * The Application Identifier shall be encoded as an OctetString (see 3GPP TS 29.212)
    */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_pdn_type, tvb, offset, 1, ENC_BIG_ENDIAN, &value);
    offset++;

    proto_item_append_text(item, "%s", val_to_str_const(value, pfcp_pdn_type_vals, "Unknown"));

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}
/*
 * 8.2.80    Failed Rule ID
 */
static const value_string pfcp_failed_rule_id_type_vals[] = {
    { 0, "PDR" },
    { 1, "FAR" },
    { 2, "QER" },
    { 3, "URR" },
    { 4, "BAR" },
    { 0, NULL }
};

static void
dissect_pfcp_failed_rule_id(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 rule_type;

    /* Octet 5  Rule ID Type */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_failed_rule_id_type, tvb, offset, 1, ENC_BIG_ENDIAN, &rule_type);
    offset++;

    proto_item_append_text(item, "%s: ", val_to_str_const(rule_type, pfcp_failed_rule_id_type_vals, "Unknown"));

    /* 6 to p  Rule ID value
    * The length and the value of the Rule ID value field shall be set as specified for the
    * PDR ID, FAR ID, QER ID, URR ID and BAR ID IE types respectively.
    */
    switch (rule_type) {
        case 0:
            /* PDR ID */
            offset = decode_pfcp_pdr_id(tvb, pinfo, tree, item, offset);
            break;
        case 1:
            /* FAR ID */
            offset = decode_pfcp_far_id(tvb, pinfo, tree, item, offset);
            break;
        case 2:
            /* QER ID */
            offset = decode_pfcp_qer_id(tvb, pinfo, tree, item, offset);
            break;
        case 3:
            /* URR ID */
            offset = decode_pfcp_urr_id(tvb, pinfo, tree, item, offset);
            break;
        case 4:
            /* BAR ID */
            offset = decode_pfcp_bar_id(tvb, pinfo, tree, item, offset);
            break;
        default:
            break;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}
/*
 * 8.2.81    Time Quota Mechanism
 */
static const value_string pfcp_time_qouta_mechanism_bti_type_vals[] = {
    { 0, "CTP" },
    { 1, "DTP" },
    { 0, NULL }
};

static void
dissect_pfcp_time_qouta_mechanism(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 bti_type;

    /* Octet 5  BIT Type */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_time_qouta_mechanism_bti_type, tvb, offset, 1, ENC_BIG_ENDIAN, &bti_type);
    offset++;

    proto_item_append_text(item, "%s", val_to_str_const(bti_type, pfcp_time_qouta_mechanism_bti_type_vals, "Unknown"));

    /* Base Time Interval
    * The Base Time Interval, shall be encoded as an Unsigned32
    * as specified in subclause 7.2.29 of 3GPP TS 32.299
    */
    proto_tree_add_item(tree, hf_pfcp_time_qouta_mechanism_bti, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset += 4;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}
/*
 * 8.2.82    User Plane IP Resource Information
 */
static void
dissect_pfcp_user_plane_ip_resource_infomation(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 upiri_flags_val;
    guint32 upiri_teid_range;

    static const int * pfcp_upiri_flags[] = {
        &hf_pfcp_spare_b7_b6,
        &hf_pfcp_upiri_flg_b6_assosi,
        &hf_pfcp_upiri_flg_b5_assoni,
        &hf_pfcp_upiri_flg_b2b4_teidri,
        &hf_pfcp_upiri_flags_b1_v6,
        &hf_pfcp_upiri_flags_b0_v4,
        NULL
    };
    /* Octet 5  Spare  ASSOSI  ASSONI  TEIDRI  TEIDRI  TEIDRI  V6  V4*/
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_upiri_flags,
        ett_pfcp_upiri_flags, pfcp_upiri_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT | BMT_NO_TFS, &upiri_flags_val);

    /* The following flags are coded within Octet 5:
     * Bit 1   - V4: If this bit is set to "1" and the CH bit is not set, then the IPv4 address field shall be present,
     *           otherwise the IPv4 address field shall not be present.
     * Bit 2   - V6: If this bit is set to "1" and the CH bit is not set, then the IPv6 address field shall be present,
     *           otherwise the IPv6 address field shall not be present.
     * Bit 3-5 - TEIDRI (TEID Range Indication): the value of this field indicates the number of bits in the most significant
     *           octet of a TEID that are used to partition the TEID range, e.g. if this field is set to "4", then the first
     *           4 bits in the TEID are used to partition the TEID range.
     * Bit 6   - ASSONI (Associated Network Instance): if this bit is set to "1", then the Network Instance field shall be present,
     *           otherwise the Network Instance field shall not be present,
     *           i.e. User Plane IP Resource Information provided can be used by CP function for any Network Instance of
     *           GTP-U user plane in the UP function.
     * Bit 7   - ASSOSI (Associated Source Interface): if this bit is set to "1", then the Source Interface field shall be present,
     *           otherwise the Source Interface field shall not be present.
     */

    /* Octet 5, bit 3-5, TEID Range Indication */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_upiri_teidri, tvb, offset, 1, ENC_BIG_ENDIAN, &upiri_teid_range);
    offset += 1;

    if (upiri_teid_range > 0)
    {
        /* Octet 6    TEID Range */
        proto_tree_add_item(tree, hf_pfcp_upiri_teid_range, tvb, offset, 1, ENC_BIG_ENDIAN);
        offset++;
    }

    if ((upiri_flags_val & 0x1) == 1) {
        /* m to (m+3)    IPv4 address */
        proto_tree_add_item(tree, hf_pfcp_upiri_ipv4, tvb, offset, 4, ENC_BIG_ENDIAN);
        offset += 4;
    }
    if ((upiri_flags_val & 0x2) == 2) {
        /* p to (p+15)   IPv6 address */
        proto_tree_add_item(tree, hf_pfcp_upiri_ipv6, tvb, offset, 16, ENC_NA);
        offset += 16;
    }
    if ((upiri_flags_val & 0x20) == 0x20) {
        /* k to (l)   Network Instance */
        guint16 ni_len = length - offset;
        if ((upiri_flags_val & 0x40) == 0x40) {
            ni_len--;
        }
        offset = decode_pfcp_network_instance(tvb, pinfo, tree, item, offset, ni_len);
    }
    if ((upiri_flags_val & 0x40) == 0x40) {
        /* r   Source Interface */
        offset = decode_pfcp_source_interface(tvb, pinfo, tree, item, offset);
    }
    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.83    User Plane Inactivity Timer
 */
static void
dissect_pfcp_user_plane_inactivity_timer(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;
    /*
    * The User Plane Inactivity Timer field shall be encoded as an Unsigned32 binary integer value.
    * The timer value "0" shall be interpreted as an indication that
    * user plane inactivity detection and reporting is stopped.
    */

    /* 5 to 8   Inactivity Timer */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_user_plane_inactivity_timer, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    if(value == 0)
        proto_item_append_text(item, " (Stopped)");

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.84    Multiplier
 */
static void
dissect_pfcp_multiplier(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length _U_, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{

    /* 5 to 12  Value-Digits */
    proto_tree_add_item(tree, hf_pfcp_multiplier_value_digits, tvb, 0, 8, ENC_BIG_ENDIAN);

    /* 12 to 15  Exponent */
    proto_tree_add_item(tree, hf_pfcp_multiplier_exponent, tvb, 8, 4, ENC_BIG_ENDIAN);

}

/*
 * 8.2.85    Aggregated URR ID IE
 */
static void
dissect_pfcp_aggregated_urr_id_ie(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item, guint16 length _U_, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    guint32 value;
    /* 5 to 8  URR ID */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_aggregated_urr_id_ie_urr_id, tvb, 0, 4, ENC_BIG_ENDIAN, &value);

    proto_item_append_text(item, "%u", value);
}

/*
 * 8.2.86   Subsequent Volume Quota
 */
static void
dissect_pfcp_subsequent_volume_quota(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 flags_val;

    static const int * pfcp_subsequent_volume_quota_flags[] = {
        &hf_pfcp_spare_b7_b3,
        &hf_pfcp_subsequent_volume_quota_b2_dlvol,
        &hf_pfcp_subsequent_volume_quota_b1_ulvol,
        &hf_pfcp_subsequent_volume_quota_b0_tovol,
        NULL
    };
    /* Octet 5  Spare   DLVOL   ULVOL   TOVOL*/
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_subsequent_volume_quota,
        ett_pfcp_subsequent_volume_quota, pfcp_subsequent_volume_quota_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT, &flags_val);
    offset += 1;

    /* The Total Volume, Uplink Volume and Downlink Volume fields shall be encoded as an Unsigned64 binary integer value.
    * They shall contain the total, uplink or downlink number of octets respectively.
    */
    if ((flags_val & 0x1) == 1) {
        /* m to (m+7)   Total Volume
        * TOVOL: If this bit is set to "1", then the Total Volume field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_subsequent_volume_quota_tovol, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
    }
    if ((flags_val & 0x2) == 2) {
        /* p to (p+7)    Uplink Volume
        * ULVOL: If this bit is set to "1", then the Uplink Volume field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_subsequent_volume_quota_ulvol, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
    }
    if ((flags_val & 0x4) == 4) {
        /* q to (q+7)   Downlink Volume
        * DLVOL: If this bit is set to "1", then the Downlink Volume field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_subsequent_volume_quota_dlvol, tvb, offset, 8, ENC_BIG_ENDIAN);
        offset += 8;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 * 8.2.87   Subsequent Time Quota
 */
static void
dissect_pfcp_subsequent_time_quota(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint value;

    /* Octet 5 to 8 Time Quota
    * The Time Quota field shall be encoded as an Unsigned32 binary integer value.
    * It shall contain the duration in seconds.
    */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_subsequent_time_quota, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    proto_item_append_text(item, "%u s", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.88   RQI
 */
static void
dissect_pfcp_rqi(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    proto_tree_add_item(tree, hf_pfcp_spare_b7_b1, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_pfcp_rqi_flag, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
    return;
}

/*
 * 8.2.89   QFI
 */
static void
dissect_pfcp_qfi(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    /*     Octets 5 SPARE   QFI
     *    The Application Identifier shall be encoded as an OctetString
     */
    proto_tree_add_item(tree, hf_pfcp_spare_b7_b6, tvb, offset, 1, ENC_BIG_ENDIAN);
    proto_tree_add_item(tree, hf_pfcp_qfi, tvb, offset, 1, ENC_NA);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
    return;
}

/*
 * 8.2.90   Querry URR Reference
 */
static void
dissect_pfcp_query_urr_reference(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    /* Octets 5 to 8 Query URR Reference value
     * The Query URR Reference value shall be encoded as an Unsigned32 binary integer value.
     * It shall contain the reference of a query request for URR(s).
     */
    proto_tree_add_item(tree, hf_pfcp_query_urr_reference, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset += 4;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.91    Additional Usage Reports Information
 */
static void
dissect_pfcp_additional_usage_reports_information(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    /*
     *    Octet    8      7   6      5      4      3      2         1
     *    5    | AURI |   Number of Additional Usage Reports value  |
     *    6    |    Number of Additional Usage Reports value     |
     *
     *  The Number of Additional Usage Reports value shall be encoded as
     *  an unsigned binary integer value on 15 bits.
     *  Bit 7 of Octet 5 is the most significant bit and bit 1 of Octet 6 is the least significant bit.
     *  The bit 8 of octet 5 shall encode the AURI (Additional Usage Reports Indication) flag{...}.
    */
    static const int * pfcp_additional_usage_reports_information_flags[] = {
        &hf_pfcp_additional_usage_reports_information_b15_auri,
        &hf_pfcp_additional_usage_reports_information_b14_b0_number_value,
        NULL
    };
    proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_pfcp_additional_usage_reports_information,
            ett_pfcp_additional_usage_reports_information, pfcp_additional_usage_reports_information_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT);
    offset += 2;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 *   8.2.92 Traffic Endpoint ID
 */
static void dissect_pfcp_traffic_endpoint_id(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    proto_tree_add_item(tree, hf_pfcp_traffic_endpoint_id, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
    return;
}

/*
 *   8.2.93 MAC Address
 */
static void dissect_pfcp_mac_address(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 flags_val;

    static const int * pfcp_mac_address_flags[] = {
        &hf_pfcp_spare_b7_b4,
        &hf_pfcp_mac_address_flags_b3_udes,
        &hf_pfcp_mac_address_flags_b2_usou,
        &hf_pfcp_mac_address_flags_b1_dest,
        &hf_pfcp_mac_address_flags_b0_sour,
        NULL
    };
    /* Octet 5  Spare   EDES    USOU   DEST   SOUR */
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_mac_address_flags,
        ett_pfcp_mac_address, pfcp_mac_address_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT | BMT_NO_TFS, &flags_val);
    offset += 1;

    // Octets "m to (m+5)" or "n to (n+5)" and "o to (o+5)" or "p to (p+5)", if present,
    // shall contain a MAC address value (12-digit hexadecimal numbers).
    if ((flags_val & 0x1) == 1) {
        /* m to (m+5)   Source MAC Address
        * SOUR: If this bit is set to "1", then the Source MAC Address field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_mac_address_source_mac_address, tvb, offset, 6, ENC_NA);
        offset += 6;
    }

    if ((flags_val & 0x2) == 2) {
        /* n to (n+5)    Destination MAC Address
        * DEST: If this bit is set to "1", then the Destination MAC Address field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_mac_address_dest_mac_address, tvb, offset, 6, ENC_NA);
        offset += 6;
    }

    if ((flags_val & 0x4) == 4) {
        /* o to (o+5)   Upper Source MAC Address
        * USOU: If this bit is set to "1", then the Upper Source MAC Address field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_mac_address_upper_source_mac_address, tvb, offset, 6, ENC_NA);
        offset += 6;
    }

    if ((flags_val & 0x8) == 8) {
        /* p to (p+5)   Upper Destination MAC Address
        * UDES: If this bit is set to "1", then the Upper Destination MAC Address field shall be present
        */
        proto_tree_add_item(tree, hf_pfcp_mac_address_upper_dest_mac_address, tvb, offset, 6, ENC_NA);
        offset += 6;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
    return;
}

/*
 *   8.2.94 C-TAG (Customer-VLAN tag)
 */
static void dissect_pfcp_c_tag(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    offset = decode_pfcp_c_tag(tvb, pinfo, tree, item, offset);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
    return;
}

/*
 *   8.2.95 S-TAG (Service-VLAN tag)
 */
static void dissect_pfcp_s_tag(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    offset = decode_pfcp_s_tag(tvb, pinfo, tree, item, offset);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
    return;
}

/*
 *   8.2.96 Ethertype
 */
static void dissect_pfcp_ethertype(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    proto_tree_add_item(tree, hf_pfcp_ethertype, tvb, offset, 1, ENC_NA);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
    return;
}

/*
 *   8.2.97 Proxying
 */
static void dissect_pfcp_proxying(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 flags_val;

    static const int * pfcp_proxying_flags[] = {
        &hf_pfcp_spare_b7_b2,
        &hf_pfcp_proxying_flags_b1_ins,
        &hf_pfcp_proxying_flags_b0_arp,
        NULL
    };
    /* Octet 5  Spare  INS   ARP */
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_proxying_flags,
        ett_pfcp_proxying, pfcp_proxying_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT, &flags_val);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
    return;
}

/*
 *   8.2.98 Ethertype Filter ID
 */
static void dissect_pfcp_ethertype_filter_id(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    proto_tree_add_item(tree, hf_pfcp_ethertype_filter_id, tvb, offset, 4, ENC_NA);
    offset += 4;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
    return;
}

/*
 *   8.2.99 Ethernet Filter Properties
 */
static void dissect_pfcp_ethernet_filter_properties(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 flags_val;

    static const int * pfcp_ethernet_filter_properties_flags[] = {
        &hf_pfcp_spare_b7_b1,
        &hf_pfcp_ethertype_filter_properties_flags_b0_bide,
        NULL
    };
    /* Octet 5  Spare  BIDE */
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_ethertype_filter_properties_flags,
        ett_pfcp_ethernet_filter_properties, pfcp_ethernet_filter_properties_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT, &flags_val);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
    return;
}

/*
 * 8.2.100   Suggested Buffering Packets Count
 */
static void
dissect_pfcp_suggested_buffering_packets_count(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;
    /* 5   Packet count value */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_suggested_buffering_packets_count_packet_count, tvb, offset, 1, ENC_BIG_ENDIAN, &value);
    offset += 1;

    proto_item_append_text(item, "%u packets", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 *   8.2.101 User ID
 */
static void dissect_pfcp_user_id(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint64 flags_val;
    guint32 length_imsi, length_imei, length_msisdn, length_nai;
    const gchar *mei_str;

    static const int * pfcp_user_id_flags[] = {
        &hf_pfcp_spare_b7_b3,
        &hf_pfcp_user_id_flags_b3_naif,
        &hf_pfcp_user_id_flags_b2_msisdnf,
        &hf_pfcp_user_id_flags_b1_imeif,
        &hf_pfcp_user_id_flags_b0_imsif,
        NULL
    };
    /* Octet 5  Spare   IMEIF   IMSIF */
    proto_tree_add_bitmask_with_flags_ret_uint64(tree, tvb, offset, hf_pfcp_user_id_flags,
        ett_pfcp_user_id, pfcp_user_id_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT | BMT_NO_TFS, &flags_val);
    offset += 1;

    /* Bit 1 - IMSIF: If this bit is set to "1", then the Length of IMSI and IMSI fields shall be present */
    if ((flags_val & 0x1) == 1) {
        /* 6   Length of IMSI */
        proto_tree_add_item_ret_uint(tree, hf_pfcp_user_id_length_of_imsi, tvb, offset, 1, ENC_BIG_ENDIAN, &length_imsi);
        offset += 1;
        /* 7 to (a)    IMSI */
        dissect_e212_imsi(tvb, pinfo, tree,  offset, length_imsi, FALSE);
        offset += length_imsi;
    }

    /* Bit 2 - IMEIF: If this bit is set to "1", then the Length of IMEI and IMEI fields shall be present */
    if ((flags_val & 0x2) == 2) {
        /* b   Length of IMEI */
        proto_tree_add_item_ret_uint(tree, hf_pfcp_user_id_length_of_imei, tvb, offset, 1, ENC_BIG_ENDIAN, &length_imei);
        offset += 1;

        /* (b+1) to c    IMEI */
        /* Fetch the BCD encoded digits from tvb low half byte, formating the digits according to
        * a default digit set of 0-9 returning "?" for overdecadic digits a pointer to the EP
        * allocated string will be returned.
        */
        mei_str = tvb_bcd_dig_to_wmem_packet_str( tvb, offset, length_imei, NULL, FALSE);
        proto_tree_add_string(tree, hf_pfcp_user_id_imei, tvb, offset, length_imei, mei_str);
        offset += length_imei;
    }

    /* Bit 3 - MSIDNF: If this bit is set to "1", then the Length of MSISDN and MSISDN fields shall be present */
    if ((flags_val & 0x4) == 4) {
        /* d   Length of MSISDN */
        proto_tree_add_item_ret_uint(tree, hf_pfcp_user_id_length_of_msisdn, tvb, offset, 1, ENC_BIG_ENDIAN, &length_msisdn);
        offset += 1;
        /* (d+1) to e    MSISDN */
        dissect_e164_msisdn(tvb, tree, offset, length_msisdn, E164_ENC_BCD);
        offset += length_msisdn;
    }

    /* Bit 4 - NAIF: If this bit is set to "1", then the Length of NAI and NAI fields shall be present */
    if ((flags_val & 0x8) == 8) {
        /* f   Length of NAI */
        proto_tree_add_item_ret_uint(tree, hf_pfcp_user_id_length_of_nai, tvb, offset, 1, ENC_BIG_ENDIAN, &length_nai);
        offset += 1;
        /* (f+1) to g    NAI */
        proto_tree_add_item(tree, hf_pfcp_user_id_nai, tvb, offset, length_nai, ENC_ASCII|ENC_NA);
        offset += length_nai;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
    return;
}

/*
 *   8.2.102 Ethernet PDU Session Information
 */
static void dissect_pfcp_ethernet_pdu_session_information(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;

    static const int * pfcp_ethernet_pdu_session_information_flags[] = {
        &hf_pfcp_spare_b7_b1,
        &hf_pfcp_ethernet_pdu_session_information_flags_b0_ethi,
        NULL
    };
    /* Octet 5  Spare   IMEIF   IMSIF */
    proto_tree_add_bitmask_with_flags(tree, tvb, offset, hf_pfcp_ethernet_pdu_session_information_flags,
        ett_pfcp_ethernet_pdu_session_information, pfcp_ethernet_pdu_session_information_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT | BMT_NO_TFS);
    offset += 1;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
    return;
}

/*
 * 8.2.103   MAC Addresses Detected
 */
static void
dissect_pfcp_mac_addresses_detected(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value, i;

    /* 5   Number of MAC addresses  */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_mac_addresses_detected_number_of_mac_addresses, tvb, offset, 1, ENC_BIG_ENDIAN, &value);
    offset += 1;

    /* o to (o+6) MAC Address  */
    for (i = 0; i < value; i++)
    {
        proto_tree_add_item(tree, hf_pfcp_mac_addresses_detected_mac_address, tvb, offset, 6, ENC_NA);
        offset += 6;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 * 8.2.104   MAC Addresses Removed
 */
static void
dissect_pfcp_mac_addresses_removed(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value, i;

    /* 5   Number of MAC addresses  */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_mac_addresses_removed_number_of_mac_addresses, tvb, offset, 1, ENC_BIG_ENDIAN, &value);
    offset += 1;

    /* o to (o+6) MAC Address  */
    for (i = 0; i < value; i++)
    {
        proto_tree_add_item(tree, hf_pfcp_mac_addresses_removed_mac_address, tvb, offset, 6, ENC_NA);
        offset += 6;
    }

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 * 8.2.105    Ethernet Inactivity Timer
 */
static void
dissect_pfcp_ethernet_inactivity_timer(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;
    /*
    * The Ethernet Inactivity Timer field shall be encoded as an Unsigned32 binary integer value.
    */

    /* 5 to 8   Inactivity Timer */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_ethernet_inactivity_timer, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }

}

/*
 * 8.2.106   Subsequent Event Quota
 */
static void
dissect_pfcp_subsequent_event_quota(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item, guint16 length _U_, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;
    /*
    * The Subsequent Event Quota field shall be encoded as an Unsigned32 binary integer value.
    */

    /* 5 to 8   Subsequent Event Quota */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_subsequent_event_quota, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    proto_item_append_text(item, "%u", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 * 8.2.107   Subsequent Event Threshold
 */
static void
dissect_pfcp_subsequent_event_threshold(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item, guint16 length _U_, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;
    /*
    * The Subsequent Event Threshold field shall be encoded as an Unsigned32 binary integer value.
    */

    /* 5 to 8   Subsequent Event Threshold */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_subsequent_event_threshold, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    proto_item_append_text(item, "%u", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 * 8.2.108   Trace Information
 */
static void
dissect_pfcp_trace_information(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length _U_, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 length_trigger_events, length_list_interfaces, length_ipaddress;

    /* 5 to 7   MCC MNC */
    offset = dissect_e212_mcc_mnc(tvb, pinfo, tree, offset, E212_NONE, TRUE);

    /* 8 to 10   Trace ID */
    proto_tree_add_item(tree, hf_pfcp_trace_information_trace_id, tvb, offset, 3, ENC_BIG_ENDIAN);
    offset += 3;

    /* 11   Length of Trigger Events */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_trace_information_length_trigger_events, tvb, offset, 1, ENC_BIG_ENDIAN, &length_trigger_events);
    offset += 1;

    /* 12 to m   Trigger Events */
    proto_tree_add_item(tree, hf_pfcp_trace_information_trigger_events, tvb, offset, length_trigger_events, ENC_NA);
    offset += length_trigger_events;

    /* m+1   Session Trace Depth */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_trace_information_session_trace_depth, tvb, offset, 1, ENC_BIG_ENDIAN, &length_trigger_events);
    offset += 1;

    /* m+2   Length of List of Interfaces */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_trace_information_length_list_interfaces, tvb, offset, 1, ENC_BIG_ENDIAN, &length_list_interfaces);
    offset += 1;

    /* (m+3) to p   List of Interfaces */
    proto_tree_add_item(tree, hf_pfcp_trace_information_list_interfaces, tvb, offset, length_trigger_events, ENC_NA);
    offset += length_list_interfaces;

    /* p+1   Length of IP address of Trace Collection Entity  */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_trace_information_length_ipaddress, tvb, offset, 1, ENC_BIG_ENDIAN, &length_ipaddress);
    offset += 1;

    /* (p+2) to q   IP Address */
    proto_tree_add_item(tree, hf_pfcp_trace_information_ipaddress, tvb, offset, length_ipaddress, ENC_NA);
    offset += length_ipaddress;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 * 8.2.109    Frame-Route
 */
static void
dissect_pfcp_frame_route(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    /* Octet 5 to (n+4) Frame-Route
    * The Frame-Route field shall be encoded as an Octet String as the value part of the Framed-Route AVP specified in IETF RFC 2865
    */
    proto_tree_add_item(tree, hf_pfcp_frame_route, tvb, 0, length, ENC_NA);
}

/*
 * 8.2.110    Frame-Routing
 */
static void
dissect_pfcp_frame_routing(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    /* Octet 5 to (n+4) Frame-Routing
    * The Frame-Routing field shall be encoded as an Octet String as the value part of the Framed-Routing AVP specified in IETF RFC 2865
    */
    proto_tree_add_item(tree, hf_pfcp_frame_routing, tvb, 0, length, ENC_NA);
}

/*
 * 8.2.111    Frame-IPv6-Route
 */
static void
dissect_pfcp_frame_ipv6_route(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    /* Octet 5 to (n+4) Frame-IPv6-Route
    * The Frame-IPv6-Route field shall be encoded as an Octet String as the value part of the Framed-IPv6-Route AVP specified in IETF RFC 2865
    */
    proto_tree_add_item(tree, hf_pfcp_frame_ipv6_route, tvb, 0, length, ENC_NA);
}

/*
 * 8.2.112   Event Quota
 */
static void
dissect_pfcp_event_quota(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item, guint16 length _U_, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;

    /* 5 to 8   Event Quota
    * The Event Quota field shall be encoded as an Unsigned32 binary integer value.
    */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_event_quota, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    proto_item_append_text(item, "%u", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 * 8.2.113   Event Threshold
 */
static void
dissect_pfcp_event_threshold(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item, guint16 length _U_, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;

    /* 5 to 8   Event Threshold
    * The Event Threshold field shall be encoded as an Unsigned32 binary integer value.
    */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_event_threshold, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    proto_item_append_text(item, "%u", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 * 8.2.114   Event Time Stamp
 */
static void
dissect_pfcp_event_time_stamp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    const gchar *time_str;
    int offset = 0;

    /* The Event Time Stamp field shall contain a UTC time.
    * Octets 5 to 8 shall be encoded in the same format as the first four octets
    * of the 64-bit timestamp format as defined in section 6 of IETF RFC 5905.
    */
    time_str = tvb_ntp_fmt_ts_sec(tvb, 0);
    proto_tree_add_string(tree, hf_pfcp_event_time_stamp, tvb, offset, 4, time_str);
    proto_item_append_text(item, "%s", time_str);
    offset += 4;

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 * 8.2.115   Averaging Window
 */
static void
dissect_pfcp_averaging_window(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item, guint16 length _U_, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;

    /* 5 to 8   Averaging Window
    * The Averaging Window field shall be encoded as an Unsigned32 binary integer value.
    */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_averaging_window, tvb, offset, 4, ENC_BIG_ENDIAN, &value);
    offset += 4;

    proto_item_append_text(item, "%u", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/*
 * 8.2.116   Paging Policy Indicator (PPI)
 */
static void
dissect_pfcp_paging_policy_indicator(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, proto_item *item _U_, guint16 length, guint8 message_type _U_, pfcp_session_args_t *args _U_)
{
    int offset = 0;
    guint32 value;

    /* Octet 5  Paging Policy Indicator (PPI)
    * The PPI shall be encoded as a value between 0 and 7, as specified in clause 5.5.3.7 of 3GPP TS 38.415
    */
    proto_tree_add_item_ret_uint(tree, hf_pfcp_paging_policy_indicator, tvb, offset, 1, ENC_BIG_ENDIAN, &value);
    offset++;

    proto_item_append_text(item, "%u", value);

    if (offset < length) {
        proto_tree_add_expert(tree, pinfo, &ei_pfcp_ie_data_not_decoded, tvb, offset, -1);
    }
}

/* Array of functions to dissect IEs
* (tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
*/
typedef struct _pfcp_ie {
    void(*decode) (tvbuff_t *, packet_info *, proto_tree *, proto_item *, guint16, guint8, pfcp_session_args_t *);
} pfcp_ie_t;

static const pfcp_ie_t pfcp_ies[] = {
/*      0 */    { dissect_pfcp_reserved },
/*      1 */    { dissect_pfcp_create_pdr },                                    /* Create PDR                                       Extendable / Table 7.5.2.2-1 */
/*      2 */    { dissect_pfcp_pdi },                                           /* PDI                                              Extendable / Table 7.5.2.2-2 */
/*      3 */    { dissect_pfcp_create_far },                                    /* Create FAR                                       Extendable / Table 7.5.2.3-1 */
/*      4 */    { dissect_pfcp_forwarding_parameters },                         /* Forwarding Parameters                            Extendable / Table 7.5.2.3-2 */
/*      5 */    { dissect_pfcp_duplicating_parameters },                        /* Duplicating Parameters                           Extendable / Table 7.5.2.3-3 */
/*      6 */    { dissect_pfcp_create_urr },                                    /* Create URR                                       Extendable / Table 7.5.2.4-1 */
/*      7 */    { dissect_pfcp_create_qer },                                    /* Create QER                                       Extendable / Table 7.5.2.5-1 */
/*      8 */    { dissect_pfcp_created_pdr },                                   /* Created PDR                                      Extendable / Table 7.5.3.2-1 */
/*      9 */    { dissect_pfcp_update_pdr },                                    /* Update PDR                                       Extendable / Table 7.5.4.2-1 */
/*     10 */    { dissect_pfcp_update_far },                                    /* Update FAR                                       Extendable / Table 7.5.4.3-1 */
/*     11 */    { dissect_pfcp_upd_forwarding_param },                          /* Update Forwarding Parameters                     Extendable / Table 7.5.4.3-2 */
/*     12 */    { dissect_pfcp_update_bar },                                    /* Update BAR (PFCP Session Report Response)        Extendable / Table 7.5.9.2-1 */
/*     13 */    { dissect_pfcp_update_urr },                                    /* Update URR                                       Extendable / Table 7.5.4.4 */
/*     14 */    { dissect_pfcp_update_qer },                                    /* Update QER                                       Extendable / Table 7.5.4.5 */
/*     15 */    { dissect_pfcp_remove_pdr },                                    /* Remove PDR                                       Extendable / Table 7.5.4.6 */
/*     16 */    { dissect_pfcp_remove_far },                                    /* Remove FAR                                       Extendable / Table 7.5.4.7 */
/*     17 */    { dissect_pfcp_remove_urr },                                    /* Remove URR                                       Extendable / Table 7.5.4.8 */
/*     18 */    { dissect_pfcp_remove_qer },                                    /* Remove QER                                       Extendable / Table 7.5.4.9 */
/*     19 */    { dissect_pfcp_cause },                                         /* Cause                                            Fixed / Subclause 8.2.1 */
/*     20 */    { dissect_pfcp_source_interface },                              /* Source Interface                                 Extendable / Subclause 8.2.2 */
/*     21 */    { dissect_pfcp_f_teid },                                        /* F-TEID                                           Extendable / Subclause 8.2.3 */
/*     22 */    { dissect_pfcp_network_instance },                              /* Network Instance                                 Variable Length / Subclause 8.2.4 */
/*     23 */    { dissect_pfcp_sdf_filter },                                    /* SDF Filter                                       Extendable / Subclause 8.2.5 */
/*     24 */    { dissect_pfcp_application_id },                                /* Application ID                                   Variable Length / Subclause 8.2.6 */
/*     25 */    { dissect_pfcp_gate_status },                                   /* Gate Status                                     Extendable / Subclause 8.2.7 */
/*     26 */    { dissect_pfcp_mbr },                                           /* MBR                                             Extendable / Subclause 8.2.8 */
/*     27 */    { dissect_pfcp_gbr },                                           /* GBR                                             Extendable / Subclause 8.2.9 */
/*     28 */    { dissect_pfcp_qer_correlation_id },                            /* QER Correlation ID                              Extendable / Subclause 8.2.10 */
/*     29 */    { dissect_pfcp_precedence },                                    /* Precedence                                      Extendable / Subclause 8.2.11 */
/*     30 */    { dissect_pfcp_transport_level_marking },                       /* Transport Level Marking                         Extendable / Subclause 8.2.12 */
/*     31 */    { dissect_pfcp_volume_threshold },                              /* Volume Threshold                                Extendable /Subclause 8.2.13 */
/*     32 */    { dissect_pfcp_time_threshold },                                /* Time Threshold                                  Extendable /Subclause 8.2.14 */
/*     33 */    { dissect_pfcp_monitoring_time },                               /* Monitoring Time                                 Extendable /Subclause 8.2.15 */
/*     34 */    { dissect_pfcp_subseq_volume_threshold },                       /* Subsequent Volume Threshold                     Extendable /Subclause 8.2.16 */
/*     35 */    { dissect_pfcp_subsequent_time_threshold },                     /* Subsequent Time Threshold                       Extendable /Subclause 8.2.17 */
/*     36 */    { dissect_pfcp_inactivity_detection_time },                     /* Inactivity Detection Time                       Extendable /Subclause 8.2.18 */
/*     37 */    { dissect_pfcp_reporting_triggers },                            /* Reporting Triggers                              Extendable /Subclause 8.2.19 */
/*     38 */    { dissect_pfcp_redirect_information },                          /* Redirect Information                            Extendable /Subclause 8.2.20 */
/*     39 */    { dissect_pfcp_report_type },                                   /* Report Type                                     Extendable / Subclause 8.2.21 */
/*     40 */    { dissect_pfcp_offending_ie },                                  /* Offending IE                                    Fixed / Subclause 8.2.22 */
/*     41 */    { dissect_pfcp_forwarding_policy },                             /* Forwarding Policy                               Extendable / Subclause 8.2.23 */
/*     42 */    { dissect_pfcp_destination_interface },                         /* Destination Interface                           Extendable / Subclause 8.2.24 */
/*     43 */    { dissect_pfcp_up_function_features },                          /* UP Function Features                            Extendable / Subclause 8.2.25 */
/*     44 */    { dissect_pfcp_apply_action },                                  /* Apply Action                                    Extendable / Subclause 8.2.26 */
/*     45 */    { dissect_pfcp_dl_data_service_inf },                           /* Downlink Data Service Information               Extendable / Subclause 8.2.27 */
/*     46 */    { dissect_pfcp_dl_data_notification_delay },                    /* Downlink Data Notification Delay                Extendable / Subclause 8.2.28 */
/*     47 */    { dissect_pfcp_dl_buffering_dur },                              /* DL Buffering Duration                           Extendable / Subclause 8.2.29 */
/*     48 */    { dissect_pfcp_dl_buffering_suggested_packet_count },           /* DL Buffering Suggested Packet Count             Variable / Subclause 8.2.30 */
/*     49 */    { dissect_pfcp_pfcpsmreq_flags },                               /* PFCPSMReq-Flags                                 Extendable / Subclause 8.2.31 */
/*     50 */    { dissect_pfcp_pfcpsrrsp_flags },                               /* PFCPSRRsp-Flags                                 Extendable / Subclause 8.2.32 */
/*     51 */    { dissect_pfcp_load_control_information },                      /* Load Control Information                        Extendable / Table 7.5.3.3-1 */
/*     52 */    { dissect_pfcp_sequence_number },                               /* Sequence Number                                 Fixed Length / Subclause 8.2.33 */
/*     53 */    { dissect_pfcp_metric },                                        /* Metric                                          Fixed Length / Subclause 8.2.34 */
/*     54 */    { dissect_pfcp_overload_control_information },                  /* Overload Control Information                    Extendable / Table 7.5.3.4-1 */
/*     55 */    { dissect_pfcp_timer },                                         /* Timer                                           Extendable / Subclause 8.2 35 */
/*     56 */    { dissect_pfcp_pdr_id },                                        /* Packet Detection Rule ID                        Extendable / Subclause 8.2 36 */
/*     57 */    { dissect_pfcp_f_seid },                                        /* F-SEID                                          Extendable / Subclause 8.2 37 */
/*     58 */    { dissect_pfcp_application_ids_pfds },                          /* Application ID's PFDs                           Extendable / Table 7.4.3.1-2 */
/*     59 */    { dissect_pfcp_pfd_context },                                   /* PFD context                                     Extendable / Table 7.4.3.1-3 */
/*     60 */    { dissect_pfcp_node_id },                                       /* Node ID                                         Extendable / Subclause 8.2.38 */
/*     61 */    { dissect_pfcp_pfd_contents },                                  /* PFD contents                                    Extendable / Subclause 8.2.39 */
/*     62 */    { dissect_pfcp_measurement_method },                            /* Measurement Method                              Extendable / Subclause 8.2.40 */
/*     63 */    { dissect_pfcp_usage_report_trigger },                          /* Usage Report Trigger                            Extendable / Subclause 8.2.41 */
/*     64 */    { dissect_pfcp_measurement_period },                            /* Measurement Period                              Extendable / Subclause 8.2.42 */
/*     65 */    { dissect_pfcp_fq_csid },                                       /* FQ-CSID                                         Extendable / Subclause 8.2.43 */
/*     66 */    { dissect_pfcp_volume_measurement },                            /* Volume Measurement                              Extendable / Subclause 8.2.44 */
/*     67 */    { dissect_pfcp_duration_measurement },                          /* Duration Measurement                            Extendable / Subclause 8.2.45 */
/*     68 */    { dissect_pfcp_application_detection_inf },                     /* Application Detection Information               Extendable / Table 7.5.8.3-2 */
/*     69 */    { dissect_pfcp_time_of_first_packet },                          /* Time of First Packet                            Extendable / Subclause 8.2.46 */
/*     70 */    { dissect_pfcp_time_of_last_packet },                           /* Time of Last Packet                             Extendable / Subclause 8.2.47 */
/*     71 */    { dissect_pfcp_quota_holding_time },                            /* Quota Holding Time                              Extendable / Subclause 8.2.48 */
/*     72 */    { dissect_pfcp_dropped_dl_traffic_threshold },                  /* Dropped DL Traffic Threshold                    Extendable / Subclause 8.2.49 */
/*     73 */    { dissect_pfcp_volume_quota },                                  /* Volume Quota                                    Extendable / Subclause 8.2.50 */
/*     74 */    { dissect_pfcp_time_quota },                                    /* Time Quota                                      Extendable / Subclause 8.2.51 */
/*     75 */    { dissect_pfcp_start_time },                                    /* Start Time                                      Extendable / Subclause 8.2.52 */
/*     76 */    { dissect_pfcp_end_time },                                      /* End Time                                        Extendable / Subclause 8.2.53 */
/*     77 */    { dissect_pfcp_pfcp_query_urr },                                /* Query URR                                       Extendable / Table 7.5.4.10-1 */
/*     78 */    { dissect_pfcp_usage_report_smr },                              /* Usage Report (in Session Modification Response) Extendable / Table 7.5.5.2-1 */
/*     79 */    { dissect_pfcp_usage_report_sdr },                              /* Usage Report (Session Deletion Response)        Extendable / Table 7.5.7.2-1 */
/*     80 */    { dissect_pfcp_usage_report_srr },                              /* Usage Report (Session Report Request)           Extendable / Table 7.5.8.3-1 */
/*     81 */    { dissect_pfcp_urr_id },                                        /* URR ID                                          Extendable / Subclause 8.2.54 */
/*     82 */    { dissect_pfcp_linked_urr_id },                                 /* Linked URR ID                                   Extendable / Subclause 8.2.55 */
/*     83 */    { dissect_pfcp_downlink_data_report },                          /* Downlink Data Report                            Extendable / Table 7.5.8.2-1 */
/*     84 */    { dissect_pfcp_outer_header_creation },                         /* Outer Header Creation                           Extendable / Subclause 8.2.56 */
/*     85 */    { dissect_pfcp_create_bar },                                    /* Create BAR                                      Extendable / Table 7.5.2.6-1 */
/*     86 */    { dissect_pfcp_update_bar_smr },                                /* Update BAR (Session Modification Request)       Extendable / Table 7.5.4.11-1 */
/*     87 */    { dissect_pfcp_remove_bar },                                    /* Remove BAR                                      Extendable / Table 7.5.4.12-1 */
/*     88 */    { dissect_pfcp_bar_id },                                        /* BAR ID                                          Extendable / Subclause 8.2.57 */
/*     89 */    { dissect_pfcp_cp_function_features },                          /* CP Function Features                            Extendable / Subclause 8.2.58 */
/*     90 */    { dissect_pfcp_usage_information },                             /* Usage Information                               Extendable / Subclause 8.2.59 */
/*     91 */    { dissect_pfcp_application_instance_id },                       /* Application Instance ID                         Variable Length / Subclause 8.2.60 */
/*     92 */    { dissect_pfcp_flow_inf },                                      /* Flow Information                                Extendable / Subclause 8.2.61 */
/*     93 */    { dissect_pfcp_ue_ip_address },                                 /* UE IP Address                                   Extendable / Subclause 8.2.62 */
/*     94 */    { dissect_pfcp_packet_rate },                                   /* Packet Rate                                     Extendable / Subclause 8.2.63 */
/*     95 */    { dissect_pfcp_outer_hdr_rem },                                 /* Outer Header Removal                            Extendable / Subclause 8.2.64 */
/*     96 */    { dissect_pfcp_recovery_time_stamp },                           /* Recovery Time Stamp                             Extendable / Subclause 8.2.65 */
/*     97 */    { dissect_pfcp_dl_flow_level_marking },                         /* DL Flow Level Marking                           Extendable / Subclause 8.2.66 */
/*     98 */    { dissect_pfcp_header_enrichment },                             /* Header Enrichment                               Extendable / Subclause 8.2.67 */
/*     99 */    { dissect_pfcp_error_indication_report },                       /* Error Indication Report                         Extendable / Table 7.5.8.4-1 */
/*    100 */    { dissect_pfcp_measurement_info },                              /* Measurement Information                         Extendable / Subclause 8.2.68 */
/*    101 */    { dissect_pfcp_node_report_type },                              /* Node Report Type                                Extendable / Subclause 8.2.69 */
/*    102 */    { dissect_pfcp_user_plane_path_failure_report },                /* User Plane Path Failure Report                  Extendable / Table 7.4.5.1.2-1 */
/*    103 */    { dissect_pfcp_remote_gtp_u_peer },                             /* Remote GTP-U Peer                               Extendable / Subclause 8.2.70 */
/*    104 */    { dissect_pfcp_ur_seqn },                                       /* UR-SEQN                                         Fixed Length / Subclause 8.2.71 */
/*    105 */    { dissect_pfcp_update_duplicating_parameters },                 /* Update Duplicating Parameters                   Extendable / Table 7.5.4.3-3 */
/*    106 */    { dissect_pfcp_act_predef_rules },                              /* Activate Predefined Rules                       Variable Length / Subclause 8.2.72 */
/*    107 */    { dissect_pfcp_deact_predef_rules },                            /* Deactivate Predefined Rules                     Variable Length / Subclause 8.2.73 */
/*    108 */    { dissect_pfcp_far_id },                                        /* FAR ID                                          Extendable / Subclause 8.2.74 */
/*    109 */    { dissect_pfcp_qer_id },                                        /* QER ID                                          Extendable / Subclause 8.2.75 */
/*    110 */    { dissect_pfcp_oci_flags },                                     /* OCI Flags                                       Extendable / Subclause 8.2.76 */
/*    111 */    { dissect_pfcp_pfcp_assoc_rel_req },                            /* PFCP Association Release Request                Extendable / Subclause 8.2.77 */
/*    112 */    { dissect_pfcp_graceful_release_period },                       /* Graceful Release Period                         Extendable / Subclause 8.2.78 */
/*    113 */    { dissect_pfcp_pdn_type },                                      /* PDN Type                                        Fixed Length / Subclause 8.2.79 */
/*    114 */    { dissect_pfcp_failed_rule_id },                                /* Failed Rule ID                                  Extendable / Subclause 8.2.80 */
/*    115 */    { dissect_pfcp_time_qouta_mechanism },                          /* Time Quota Mechanism                            Extendable / Subclause 8.2.81 */
/*    116 */    { dissect_pfcp_user_plane_ip_resource_infomation },             /* User Plane IP Resource Information              Extendable / Subclause 8.2.82 */
/*    117 */    { dissect_pfcp_user_plane_inactivity_timer },                   /* User Plane Inactivity Timer                     Extendable / Subclause 8.2.83 */
/*    118 */    { dissect_pfcp_aggregated_urrs },                               /* Aggregated URRs                                 Extendable / Table 7.5.2.4-2 */
/*    119 */    { dissect_pfcp_multiplier },                                    /* Multiplier                                      Fixed Length / Subclause 8.2.84 */
/*    120 */    { dissect_pfcp_aggregated_urr_id_ie },                          /* Aggregated URR ID IE                            Fixed Length / Subclause 8.2.85 */
/*    121 */    { dissect_pfcp_subsequent_volume_quota },                       /* Subsequent Volume Quota                         Extendable / Subclause 8.2.86 */
/*    122 */    { dissect_pfcp_subsequent_time_quota },                         /* Subsequent Time Quota                           Extendable / Subclause 8.2.87 */
/*    123 */    { dissect_pfcp_rqi },                                           /* RQI                                             Extendable / Subclause 8.2.88 */
/*    124 */    { dissect_pfcp_qfi },                                           /* QFI                                             Extendable / Subclause 8.2.89 */
/*    125 */    { dissect_pfcp_query_urr_reference },                           /* Query URR Reference                             Extendable / Subclause 8.2.90 */
/*    126 */    { dissect_pfcp_additional_usage_reports_information },          /* Additional Usage Reports Information            Extendable /  Subclause 8.2.91 */
/*    127 */    { dissect_pfcp_create_traffic_endpoint },                       /* Create Traffic Endpoint                         Extendable / Table 7.5.2.7 */
/*    128 */    { dissect_pfcp_created_traffic_endpoint },                      /* Created Traffic Endpoint                        Extendable / Table 7.5.3.5 */
/*    129 */    { dissect_pfcp_update_traffic_endpoint },                       /* Update Traffic Endpoint                         Extendable / Table 7.5.4.13 */
/*    130 */    { dissect_pfcp_remove_traffic_endpoint },                       /* Remove Traffic Endpoint                         Extendable / Table 7.5.4.14 */
/*    131 */    { dissect_pfcp_traffic_endpoint_id },                           /* Traffic Endpoint ID                             Extendable / Subclause 8.2.92 */
/*    132 */    { dissect_pfcp_ethernet_packet_filter },                        /* Ethernet Packet Filter IE                       Extendable / Table 7.5.2.2-3 */
/*    133 */    { dissect_pfcp_mac_address },                                   /* MAC address                                     Extendable / Subclause 8.2.93 */
/*    134 */    { dissect_pfcp_c_tag },                                         /* C-TAG                                           Extendable / Subclause 8.2.94 */
/*    135 */    { dissect_pfcp_s_tag },                                         /* S-TAG                                           Extendable / Subclause 8.2.95 */
/*    136 */    { dissect_pfcp_ethertype },                                     /* Ethertype                                       Extendable / Subclause 8.2.96 */
/*    137 */    { dissect_pfcp_proxying },                                      /* Proxying                                        Extendable / Subclause 8.2.97 */
/*    138 */    { dissect_pfcp_ethertype_filter_id },                           /* Ethernet Filter ID                              Extendable / Subclause 8.2.98 */
/*    139 */    { dissect_pfcp_ethernet_filter_properties },                    /* Ethernet Filter Properties                      Extendable / Subclause 8.2.99  */
/*    140 */    { dissect_pfcp_suggested_buffering_packets_count },             /* Suggested Buffering Packets Count               Extendable / Subclause 8.2.100  */
/*    141 */    { dissect_pfcp_user_id },                                       /* User ID                                         Extendable / Subclause 8.2.101  */
/*    142 */    { dissect_pfcp_ethernet_pdu_session_information },              /* Ethernet PDU Session Information                Extendable / Subclause 8.2.102  */
/*    143 */    { dissect_pfcp_ethernet_traffic_information },                  /* Ethernet Traffic Information                    Extendable / Table 7.5.8.3-3  */
/*    144 */    { dissect_pfcp_mac_addresses_detected },                        /* MAC Addresses Detected                          Extendable / Subclause 8.2.103  */
/*    145 */    { dissect_pfcp_mac_addresses_removed },                         /* MAC Addresses Removed                           Extendable / Subclause 8.2.104  */
/*    146 */    { dissect_pfcp_ethernet_inactivity_timer },                     /* Ethernet Inactivity Timer                       Extendable / Subclause 8.2.105  */
/*    147 */    { dissect_pfcp_additional_monitoring_time },                    /* Additional Monitoring Time                      Extendable / Table 7.5.2.4-3  */
/*    148 */    { dissect_pfcp_event_quota },                                   /* Event Quota                                     Extendable / Subclause 8.2.112  */
/*    149 */    { dissect_pfcp_event_threshold },                               /* Event Threshold                                 Extendable / Subclause 8.2.113  */
/*    150 */    { dissect_pfcp_subsequent_event_quota },                        /* Subsequent Event Quota                          Extendable / Subclause 8.2.106  */
/*    151 */    { dissect_pfcp_subsequent_event_threshold },                    /* Subsequent Event Threshold                      Extendable / Subclause 8.2.107  */
/*    152 */    { dissect_pfcp_trace_information },                             /* Trace Information                               Extendable / Subclause 8.2.108  */
/*    153 */    { dissect_pfcp_frame_route },                                   /* Frame-Route                                     Variable Length / Subclause 8.2.109  */
/*    154 */    { dissect_pfcp_frame_routing },                                 /* Frame-Routing                                   Fixed Length / Subclause 8.2.110  */
/*    155 */    { dissect_pfcp_frame_ipv6_route },                              /* Frame-IPv6-Route                                Variable Length / Subclause 8.2.111  */
/*    156 */    { dissect_pfcp_event_time_stamp },                              /* Event Time Stamp                                Extendable / Subclause 8.2.114  */
/*    157 */    { dissect_pfcp_averaging_window },                              /* Averaging Window                                Extendable / Subclause 8.2.115  */
/*    158 */    { dissect_pfcp_paging_policy_indicator },                       /* Paging Policy Indicator                         Extendable / Subclause 8.2.116  */
//159 to 32767 Spare. For future use.
//32768 to 65535 Vendor-specific IEs.
    { NULL },                                                        /* End of List */
};

#define NUM_PFCP_IES (sizeof(pfcp_ies)/sizeof(pfcp_ie_t))
/* Set up the array to hold "etts" for each IE*/
gint ett_pfcp_elem[NUM_PFCP_IES-1];

static pfcp_msg_hash_t *
pfcp_match_response(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, gint seq_nr, guint msgtype, pfcp_conv_info_t *pfcp_info, guint8 last_cause)
{
    pfcp_msg_hash_t   pcr, *pcrp = NULL;
    guint32 *session;

    pcr.seq_nr = seq_nr;
    pcr.req_time = pinfo->abs_ts;

    switch (msgtype) {
    case PFCP_MSG_HEARTBEAT_REQUEST:
    case PFCP_MSG_PFD_MANAGEMENT_REQUEST:
    case PFCP_MSG_ASSOCIATION_SETUP_REQUEST:
    case PFCP_MSG_ASSOCIATION_UPDATE_REQUEST:
    case PFCP_MSG_ASSOCIATION_RELEASE_REQUEST:
    case PFCP_MSG_NODE_REPORT_REQEUST:
    case PFCP_MSG_SESSION_SET_DELETION_REQUEST:
    case PFCP_MSG_SESSION_ESTABLISHMENT_REQUEST:
    case PFCP_MSG_SESSION_MODIFICATION_REQUEST:
    case PFCP_MSG_SESSION_DELETION_REQUEST:
    case PFCP_MSG_SESSION_REPORT_REQUEST:
        pcr.is_request = TRUE;
        pcr.req_frame = pinfo->num;
        pcr.rep_frame = 0;
        break;
    case PFCP_MSG_HEARTBEAT_RESPONSE:
    case PFCP_MSG_PFD_MANAGEMENT_RESPONSE:
    case PFCP_MSG_ASSOCIATION_SETUP_RESPONSE:
    case PFCP_MSG_ASSOCIATION_UPDATE_RESPONSE:
    case PFCP_MSG_ASSOCIATION_RELEASE_RESPONSE:
    case PFCP_MSG_VERSION_NOT_SUPPORTED_RESPONSE:
    case PFCP_MSG_NODE_REPORT_RERESPONSE:
    case PFCP_MSG_SESSION_SET_DELETION_RESPONSE:
    case PFCP_MSG_SESSION_ESTABLISHMENT_RESPONSE:
    case PFCP_MSG_SESSION_MODIFICATION_RESPONSE:
    case PFCP_MSG_SESSION_DELETION_RESPONSE:
    case PFCP_MSG_SESSION_REPORT_RESPONSE:

        pcr.is_request = FALSE;
        pcr.req_frame = 0;
        pcr.rep_frame = pinfo->num;
        break;
    default:
        pcr.is_request = FALSE;
        pcr.req_frame = 0;
        pcr.rep_frame = 0;
        break;
    }

    pcrp = (pfcp_msg_hash_t *)wmem_map_lookup(pfcp_info->matched, &pcr);

    if (pcrp) {
        pcrp->is_request = pcr.is_request;
    } else {
        /* no match, let's try to make one */
        switch (msgtype) {
        case PFCP_MSG_HEARTBEAT_REQUEST:
        case PFCP_MSG_PFD_MANAGEMENT_REQUEST:
        case PFCP_MSG_ASSOCIATION_SETUP_REQUEST:
        case PFCP_MSG_ASSOCIATION_UPDATE_REQUEST:
        case PFCP_MSG_ASSOCIATION_RELEASE_REQUEST:
        case PFCP_MSG_NODE_REPORT_REQEUST:
        case PFCP_MSG_SESSION_SET_DELETION_REQUEST:
        case PFCP_MSG_SESSION_ESTABLISHMENT_REQUEST:
        case PFCP_MSG_SESSION_MODIFICATION_REQUEST:
        case PFCP_MSG_SESSION_DELETION_REQUEST:
        case PFCP_MSG_SESSION_REPORT_REQUEST:

            pcr.seq_nr = seq_nr;

            pcrp = (pfcp_msg_hash_t *)wmem_map_remove(pfcp_info->unmatched, &pcr);

            /* if we can't reuse the old one, grab a new chunk */
            if (!pcrp) {
                pcrp = wmem_new(wmem_file_scope(), pfcp_msg_hash_t);
            }
            pcrp->seq_nr = seq_nr;
            pcrp->req_frame = pinfo->num;
            pcrp->req_time = pinfo->abs_ts;
            pcrp->rep_frame = 0;
            pcrp->msgtype = msgtype;
            pcrp->is_request = TRUE;
            wmem_map_insert(pfcp_info->unmatched, pcrp, pcrp);
            return NULL;
            break;
        case PFCP_MSG_HEARTBEAT_RESPONSE:
        case PFCP_MSG_PFD_MANAGEMENT_RESPONSE:
        case PFCP_MSG_ASSOCIATION_SETUP_RESPONSE:
        case PFCP_MSG_ASSOCIATION_UPDATE_RESPONSE:
        case PFCP_MSG_ASSOCIATION_RELEASE_RESPONSE:
        case PFCP_MSG_VERSION_NOT_SUPPORTED_RESPONSE:
        case PFCP_MSG_NODE_REPORT_RERESPONSE:
        case PFCP_MSG_SESSION_SET_DELETION_RESPONSE:
        case PFCP_MSG_SESSION_ESTABLISHMENT_RESPONSE:
        case PFCP_MSG_SESSION_MODIFICATION_RESPONSE:
        case PFCP_MSG_SESSION_DELETION_RESPONSE:
        case PFCP_MSG_SESSION_REPORT_RESPONSE:

            pcr.seq_nr = seq_nr;
            pcrp = (pfcp_msg_hash_t *)wmem_map_lookup(pfcp_info->unmatched, &pcr);

            if (pcrp) {
                if (!pcrp->rep_frame) {
                    wmem_map_remove(pfcp_info->unmatched, pcrp);
                    pcrp->rep_frame = pinfo->num;
                    pcrp->is_request = FALSE;
                    wmem_map_insert(pfcp_info->matched, pcrp, pcrp);
                }
            }
            break;
        default:
            break;
        }
    }

    /* we have found a match */
    if (pcrp) {
        proto_item *it;

        if (pcrp->is_request) {
            it = proto_tree_add_uint(tree, hf_pfcp_response_in, tvb, 0, 0, pcrp->rep_frame);
            PROTO_ITEM_SET_GENERATED(it);
        } else {
            nstime_t ns;

            it = proto_tree_add_uint(tree, hf_pfcp_response_to, tvb, 0, 0, pcrp->req_frame);
            PROTO_ITEM_SET_GENERATED(it);
            nstime_delta(&ns, &pinfo->abs_ts, &pcrp->req_time);
            it = proto_tree_add_time(tree, hf_pfcp_response_time, tvb, 0, 0, &ns);
            PROTO_ITEM_SET_GENERATED(it);
            if (g_pfcp_session && !PINFO_FD_VISITED(pinfo)) {
                /* PFCP session */
                /* If it's not already in the list */
                session = (guint32 *)g_hash_table_lookup(pfcp_session_table, &pinfo->num);
                if (!session) {
                    session = (guint32 *)g_hash_table_lookup(pfcp_session_table, &pcrp->req_frame);
                    if (session != NULL) {
                        pfcp_add_session(pinfo->num, *session);
                    }
                }

                if (!pfcp_is_cause_accepted(last_cause)){
                    /* If the cause is not accepted then we have to remove all the session information about its corresponding request */
                    pfcp_remove_frame_info(&pcrp->req_frame);
                }
            }
        }
    }
    return pcrp;
}

/* 7.2.3.3  Grouped Information Elements */

static void
dissect_pfcp_grouped_ie(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, int ett_index, pfcp_session_args_t *args)
{
    int         offset = 0;
    tvbuff_t   *new_tvb;
    proto_tree *grouped_tree;

    proto_item_append_text(item, "[Grouped IE]");
    grouped_tree = proto_item_add_subtree(tree, ett_index);

    new_tvb = tvb_new_subset_length(tvb, offset, length);
    dissect_pfcp_ies_common(new_tvb, pinfo, grouped_tree, 0, message_type, args);

}

static void
dissect_pfcp_pdi(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_ID_PDI], args);
}

static void
dissect_pfcp_create_pdr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_ID_CREATE_PDR], args);
}

static void
dissect_pfcp_create_far(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_CREATE_FAR], args);
}

static void
dissect_pfcp_forwarding_parameters(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_FORWARDING_PARAMETERS], args);
}

static void
dissect_pfcp_duplicating_parameters(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_DUPLICATING_PARAMETERS], args);
}

static void
dissect_pfcp_create_urr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_CREATE_URR], args);
}

static void
dissect_pfcp_create_qer(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_CREATE_QER], args);
}

static void
dissect_pfcp_created_pdr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_CREATED_PDR], args);
}

static void
dissect_pfcp_update_pdr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_UPDATE_PDR], args);
}

static void
dissect_pfcp_update_far(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_UPDATE_FAR], args);
}

static void
dissect_pfcp_upd_forwarding_param(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_UPD_FORWARDING_PARAM], args);
}

static void
dissect_pfcp_update_bar(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_UPDATE_BAR], args);
}

static void
dissect_pfcp_update_urr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_UPDATE_URR], args);
}

static void
dissect_pfcp_update_qer(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_UPDATE_QER], args);
}

static void
dissect_pfcp_remove_pdr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_REMOVE_PDR], args);
}

static void
dissect_pfcp_remove_far(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_REMOVE_FAR], args);
}

static void
dissect_pfcp_remove_urr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_REMOVE_URR], args);
}

static void
dissect_pfcp_remove_qer(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_REMOVE_QER], args);
}

static void
dissect_pfcp_load_control_information(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_LOAD_CONTROL_INFORMATION], args);
}

static void
dissect_pfcp_overload_control_information(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_OVERLOAD_CONTROL_INFORMATION], args);
}

static void
dissect_pfcp_application_ids_pfds(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_APPLICATION_IDS_PFDS], args);
}

static void
dissect_pfcp_pfd_context(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_PFD_CONTEXT], args);
}


static void
dissect_pfcp_application_detection_inf(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_APPLICATION_DETECTION_INF], args);
}

static void
dissect_pfcp_pfcp_query_urr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_QUERY_URR], args);
}

static void
dissect_pfcp_usage_report_smr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_USAGE_REPORT_SMR], args);
}

static void
dissect_pfcp_usage_report_sdr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_USAGE_REPORT_SDR], args);
}

static void
dissect_pfcp_usage_report_srr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_USAGE_REPORT_SRR], args);
}

static void
dissect_pfcp_downlink_data_report(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_DOWNLINK_DATA_REPORT], args);
}

static void
dissect_pfcp_create_bar(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_CREATE_BAR], args);
}

static void
dissect_pfcp_update_bar_smr(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_UPDATE_BAR_SMR], args);
}

static void
dissect_pfcp_remove_bar(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_REMOVE_BAR], args);
}

static void
dissect_pfcp_error_indication_report(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_ERROR_INDICATION_REPORT], args);
}

static void
dissect_pfcp_user_plane_path_failure_report(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_USER_PLANE_PATH_FAILURE_REPORT], args);
}

static void
dissect_pfcp_update_duplicating_parameters(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_UPDATE_DUPLICATING_PARAMETERS], args);
}

static void
dissect_pfcp_aggregated_urrs(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_AGGREGATED_URRS], args);
}

static void
dissect_pfcp_create_traffic_endpoint(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_CREATE_TRAFFIC_ENDPOINT], args);
}

static void
dissect_pfcp_created_traffic_endpoint(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_CREATED_TRAFFIC_ENDPOINT], args);
}

static void
dissect_pfcp_update_traffic_endpoint(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_UPDATE_TRAFFIC_ENDPOINT], args);
}

static void
dissect_pfcp_remove_traffic_endpoint(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_REMOVE_TRAFFIC_ENDPOINT], args);
}

static void
dissect_pfcp_ethernet_packet_filter(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_ETHERNET_PACKET_FILTER], args);
}

static void
dissect_pfcp_ethernet_traffic_information(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_ETHERNET_TRAFFIC_INFORMATION], args);
}

static void
dissect_pfcp_additional_monitoring_time(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, proto_item *item, guint16 length, guint8 message_type, pfcp_session_args_t *args)
{
    dissect_pfcp_grouped_ie(tvb, pinfo, tree, item, length, message_type, ett_pfcp_elem[PFCP_IE_ADDITIONAL_MONITORING_TIME], args);
}

static void
dissect_pfcp_ies_common(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, gint offset, guint8 message_type, pfcp_session_args_t *args)
{
    proto_tree *ie_tree;
    proto_item *ti;
    tvbuff_t   *ie_tvb;
    guint16 type, length;
    guint16 enterprise_id;

    /* 8.1.1    Information Element Format */
    /*
    Octets      8   7   6   5   4   3   2   1
    1 to 2      Type = xxx (decimal)
    3 to 4      Length = n
    p to (p+1)  Enterprise ID
    k to (n+4)  IE specific data or content of a grouped IE

    If the Bit 8 of Octet 1 is not set, this indicates that the IE is defined by 3GPP and the Enterprise ID is absent.
    If Bit 8 of Octet 1 is set, this indicates that the IE is defined by a vendor and the Enterprise ID is present
    identified by the Enterprise ID
    */

    /*Enterprise ID : if the IE type value is within the range of 32768 to 65535,
     * this field shall contain the IANA - assigned "SMI Network Management Private Enterprise Codes"
     * value of the vendor defining the IE.
     */
    /* Length: this field contains the length of the IE excluding the first four octets, which are common for all IEs */

    /* Process the IEs*/
    while (offset < (gint)tvb_reported_length(tvb)) {
        /* Octet 1 -2 */
        type = tvb_get_ntohs(tvb, offset);
        length = tvb_get_ntohs(tvb, offset + 2);

        if ((type & 0x8000) == 0x8000 ) {
            enterprise_id = tvb_get_ntohs(tvb, offset + 4);
            ie_tree = proto_tree_add_subtree_format(tree, tvb, offset, 4 + length, ett_pfcp_ie, &ti, "Enterprise %s specific IE: %u",
                try_enterprises_lookup(enterprise_id),
                type);

            proto_tree_add_item(ie_tree, hf_pfcp2_enterprise_ie, tvb, offset, 2, ENC_BIG_ENDIAN);
            offset += 2;

            proto_tree_add_item(ie_tree, hf_pfcp2_ie_len, tvb, offset, 2, ENC_BIG_ENDIAN);
            offset += 2;

            /* Bit 8 of Octet 1 is set, this indicates that the IE is defined by a vendor and the Enterprise ID is present */
            proto_tree_add_item(ie_tree, hf_pfcp_enterprise_id, tvb, offset, 2, ENC_BIG_ENDIAN);

            /*
            * 5.6.3    Modifying the Rules of an Existing PFCP Session
            *
            * Updating the Rule including the IEs to be removed with a null length,
            * e.g. by including the Update URR IE in the PFCP Session Modification Request
            * with the IE(s) to be removed with a null length.
            */
            if (length == 0) {
                proto_item_append_text(ti, "[IE to be removed]");

                /* Adding offset for EnterpriseID as Bit 8 of Octet 1 is set, the Enterprise ID is present */
                offset += 2;

            } else {
                /* give the whole IE to the subdissector */
                ie_tvb = tvb_new_subset_length(tvb, offset - 4, length+4);
                if (!dissector_try_uint_new(pfcp_enterprise_ies_dissector_table, enterprise_id, ie_tvb, pinfo, ie_tree, FALSE, ti)) {
                    proto_tree_add_item(ie_tree, hf_pfcp_enterprise_data, ie_tvb, 6, -1, ENC_NA);
                }
            }
            offset += length;
        } else {
            int tmp_ett;
            if (type < (NUM_PFCP_IES - 1)) {
                tmp_ett = ett_pfcp_elem[type];
            } else {
                tmp_ett = ett_pfcp_ie;
            }
            ie_tree = proto_tree_add_subtree_format(tree, tvb, offset, 4 + length, tmp_ett, &ti, "%s : ",
                val_to_str_ext_const(type, &pfcp_ie_type_ext, "Unknown"));

            proto_tree_add_item(ie_tree, hf_pfcp2_ie, tvb, offset, 2, ENC_BIG_ENDIAN);
            offset += 2;
            proto_tree_add_item(ie_tree, hf_pfcp2_ie_len, tvb, offset, 2, ENC_BIG_ENDIAN);
            offset += 2;

            /*
            * 5.6.3    Modifying the Rules of an Existing PFCP Session
            *
            * Updating the Rule including the IEs to be removed with a null length,
            * e.g. by including the Update URR IE in the PFCP Session Modification Request
            * with the IE(s) to be removed with a null length.
            */
            if( length == 0 ) {
                proto_item_append_text(ti, "[IE to be removed]");
            } else {
                if (type < (NUM_PFCP_IES -1)) {
                    ie_tvb = tvb_new_subset_length(tvb, offset, length);
                    if(pfcp_ies[type].decode){
                        (*pfcp_ies[type].decode) (ie_tvb, pinfo, ie_tree, ti, length, message_type, args);
                    } else {
                        /* NULL function pointer, we have no decoding function*/
                        proto_tree_add_expert(ie_tree, pinfo, &ei_pfcp_ie_not_decoded_null, tvb, offset, length);
                    }
                } else {
                    /* IE id outside of array, We have no decoding function for it */
                    proto_tree_add_expert(ie_tree, pinfo, &ei_pfcp_ie_not_decoded_to_large, tvb, offset, length);
                }
            }
            offset += length;
        }
    }
}

static int
dissect_pfcp(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void *data _U_)
{
    proto_item          *item;
    proto_tree          *sub_tree;
    int                  offset = 0;
    guint64              pfcp_flags;
    guint8               message_type, cause_aux;
    guint32              length;
    guint32              length_remaining;
    int                  seq_no = 0;
    conversation_t      *conversation;
    pfcp_conv_info_t    *pfcp_info;
    pfcp_session_args_t *args = NULL;
    pfcp_hdr_t          *pfcp_hdr = NULL;

    static const int * pfcp_hdr_flags[] = {
        &hf_pfcp_version,
        &hf_pfcp_spare_b4,
        &hf_pfcp_spare_b3,
        &hf_pfcp_spare_b2,
        &hf_pfcp_mp_flag,
        &hf_pfcp_s_flag,
        NULL
    };

    pfcp_hdr = wmem_new0(wmem_packet_scope(), pfcp_hdr_t);

    /* Setting the SEID to -1 to say that the SEID is not valid for this packet */
    pfcp_hdr->seid = -1;

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "PFCP");
    col_clear(pinfo->cinfo, COL_INFO);

    message_type = tvb_get_guint8(tvb, 1);
    col_set_str(pinfo->cinfo, COL_INFO, val_to_str_ext_const(message_type, &pfcp_message_type_ext, "Unknown"));
    if (g_pfcp_session) {
        args = wmem_new0(wmem_packet_scope(), pfcp_session_args_t);
        args->last_cause = 1;                                         /* It stores the last cause decoded. Cause accepted by default */
        /* We create the auxiliary lists */
        args->seid_list = wmem_list_new(wmem_packet_scope());
        args->ip_list = wmem_list_new(wmem_packet_scope());
    }

    /* Do we have a conversation for this connection? */
    conversation = find_or_create_conversation(pinfo);

    /* Do we already know this conversation? */
    pfcp_info = (pfcp_conv_info_t *)conversation_get_proto_data(conversation, proto_pfcp);
    if (pfcp_info == NULL) {
        /* No. Attach that information to the conversation,
        * and add it to the list of information structures.
        */
        pfcp_info = wmem_new(wmem_file_scope(), pfcp_conv_info_t);
        /* Request/response matching tables */
        pfcp_info->matched = wmem_map_new(wmem_file_scope(), pfcp_sn_hash, pfcp_sn_equal_matched);
        pfcp_info->unmatched = wmem_map_new(wmem_file_scope(), pfcp_sn_hash, pfcp_sn_equal_unmatched);

        conversation_add_proto_data(conversation, proto_pfcp, pfcp_info);
    }

    item = proto_tree_add_item(tree, proto_pfcp, tvb, 0, -1, ENC_NA);
    sub_tree = proto_item_add_subtree(item, ett_pfcp);

    /* 7.2.2    Message Header */
    /*
        Octet     8     7     6     5     4     3     2     1
          1    | Version         |Spare|Spare|Spare|  MP  |  S  |
          2    |        Message Type                            |
          3    |        Message Length (1st Octet)              |
          4    |        Message Length (2nd Octet)              |
        m to   | If S flag is set to 1, then SEID shall be      |
        k(m+7) | placed into octets 5-12. Otherwise, SEID field |
               | is not present at all.                         |
        n to   | Sequence Number                                |
        (n+2)  |                                                |
        (n+3)  |         Spare                                  |

    */
    /* Octet 1 */
    proto_tree_add_bitmask_with_flags_ret_uint64(sub_tree, tvb, offset, hf_pfcp_hdr_flags,
        ett_pfcp_flags, pfcp_hdr_flags, ENC_BIG_ENDIAN, BMT_NO_FALSE | BMT_NO_INT, &pfcp_flags);
    offset += 1;

    /* Octet 2 Message Type */
    pfcp_hdr->message = tvb_get_guint8(tvb, offset);
    proto_tree_add_uint(sub_tree, hf_pfcp_msg_type, tvb, offset, 1, pfcp_hdr->message);
    offset += 1;

    /* Octet 3 - 4 Message Length */
    proto_tree_add_item_ret_uint(sub_tree, hf_pfcp_msg_length, tvb, offset, 2, ENC_BIG_ENDIAN, &length);
    offset += 2;
    /*
     * The length field shall indicate the length of the message in octets
     * excluding the mandatory part of the PFCP header (the first 4 octets).
     */
    length_remaining = tvb_reported_length_remaining(tvb, offset);
    if (length != length_remaining) {
        proto_tree_add_expert_format(sub_tree, pinfo, &ei_pfcp_ie_encoding_error, tvb, offset, -1, "Invalid Length for the message: %d instead of %d", length, length_remaining);
    }

    if ((pfcp_flags & 0x1) == 1) {
        /* If S flag is set to 1, then SEID shall be placed into octets 5-12*/
        /* Session Endpoint Identifier 8 Octets */
        pfcp_hdr->seid = tvb_get_ntohi64(tvb, offset);
        proto_tree_add_uint64(sub_tree, hf_pfcp_seid, tvb, offset, 8, pfcp_hdr->seid);
        offset += 8;
    }
    /* 7.2.2.2    PFCP Header for Node Related Messages */
    /*
        Octet     8     7     6     5     4     3     2     1
          1    | Version         |Spare|Spare|Spare| MP=0 | S=0 |
          2    |        Message Type                            |
          3    |        Message Length (1st Octet)              |
          4    |        Message Length (2nd Octet)              |
          5    |        Sequence Number (1st Octet)             |
          6    |        Sequence Number (2st Octet)             |
          7    |        Sequence Number (3st Octet)             |
          8    |             Spare                              |
          */
    proto_tree_add_item_ret_uint(sub_tree, hf_pfcp_seqno, tvb, offset, 3, ENC_BIG_ENDIAN, &seq_no);
    offset += 3;

    if ((pfcp_flags & 0x2) == 0x2) {
        /* If the "MP" flag is set to "1", then bits 8 to 5 of octet 16 shall indicate the message priority.*/
        proto_tree_add_item(sub_tree, hf_pfcp_mp, tvb, offset, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(sub_tree, hf_pfcp_spare_h0, tvb, offset, 1, ENC_BIG_ENDIAN);
    } else {
        proto_tree_add_item(sub_tree, hf_pfcp_spare_oct, tvb, offset, 1, ENC_BIG_ENDIAN);
    }
    offset++;

    /* Dissect the IEs in the message */
    dissect_pfcp_ies_common(tvb, pinfo, sub_tree, offset, message_type, args);

    /* Use sequence number to track Req/Resp pairs */
    cause_aux = 16; /* Cause accepted by default. Only used when args is NULL */
    if (args && !PINFO_FD_VISITED(pinfo)) {
        /* We insert the lists inside the table*/
        pfcp_fill_map(args->seid_list, args->ip_list, pinfo->num);
        cause_aux = args->last_cause;
    }
    pfcp_match_response(tvb, pinfo, sub_tree, seq_no, message_type, pfcp_info, cause_aux);
    if (args) {
        pfcp_track_session(tvb, pinfo, sub_tree, pfcp_hdr, args->seid_list, args->ip_list, args->last_seid, args->last_ip);
    }

    return tvb_reported_length(tvb);
}

/* Enterprise IE decoding 3GPP */
static int
dissect_pfcp_3gpp_enterprise_ies(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
    proto_item *top_item = (proto_item *)data;
    /* We are give the complete ie, but the first 6 octets are dissected in the pfcp dissector*/
    proto_item_append_text(top_item, " Enterprise ID set to '10415' shall not be used for the vendor specific IEs.");
    proto_tree_add_expert(tree, pinfo, &ei_pfcp_enterprise_ie_3gpp, tvb, 0, -1);

    return tvb_reported_length(tvb);
}

static void
pfcp_init(void)
{
    pfcp_session_count = 1;
    pfcp_session_table = g_hash_table_new(g_int_hash, g_int_equal);
    pfcp_frame_tree = wmem_tree_new(wmem_file_scope());
}

static void
pfcp_cleanup(void)
{
    pfcp_session_conv_info_t *pfcp_info;

    /* Free up state attached to the pfcp_info structures */
    for (pfcp_info = pfcp_session_info_items; pfcp_info != NULL; ) {
        pfcp_session_conv_info_t *next;

        g_hash_table_destroy(pfcp_info->matched);
        pfcp_info->matched=NULL;
        g_hash_table_destroy(pfcp_info->unmatched);
        pfcp_info->unmatched=NULL;

        next = pfcp_info->next;
        pfcp_info = next;
    }

    /* Free up state attached to the pfcp session structures */
    pfcp_info_items = NULL;

    if (pfcp_session_table != NULL) {
        g_hash_table_destroy(pfcp_session_table);
    }
    pfcp_session_table = NULL;
}

void
proto_register_pfcp(void)
{

    static hf_register_info hf_pfcp[] = {

        { &hf_pfcp_msg_type,
        { "Message Type", "pfcp.msg_type",
        FT_UINT8, BASE_DEC | BASE_EXT_STRING, &pfcp_message_type_ext, 0x0,
        NULL, HFILL }
        },
        { &hf_pfcp_msg_length,
        { "Length", "pfcp.length",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }
        },
        { &hf_pfcp_hdr_flags,
        { "Flags", "pfcp.flags",
        FT_UINT8, BASE_HEX, NULL, 0x0,
        NULL, HFILL }
        },
        { &hf_pfcp_version,
        { "Version", "pfcp.version",
        FT_UINT8, BASE_DEC, NULL, 0xe0,
        NULL, HFILL }
        },
        { &hf_pfcp_mp_flag,
        { "Message Priority (MP)", "pfcp.mp_flag",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }
        },
        { &hf_pfcp_s_flag,
        { "SEID (S)", "pfcp.s",
        FT_BOOLEAN, 8, NULL, 0x01,
        NULL, HFILL }
        },
        { &hf_pfcp_spare_b2,
        { "Spare", "pfcp.spare_b2",
        FT_UINT8, BASE_DEC, NULL, 0x04,
        NULL, HFILL }
        },
        { &hf_pfcp_spare_b3,
        { "Spare", "pfcp.spare_b3",
        FT_UINT8, BASE_DEC, NULL, 0x08,
        NULL, HFILL }
        },
        { &hf_pfcp_spare_b4,
        { "Spare", "pfcp.spare_b4",
        FT_UINT8, BASE_DEC, NULL, 0x10,
        NULL, HFILL }
        },
        { &hf_pfcp_spare_b5,
        { "Spare", "pfcp.spare_b5",
        FT_UINT8, BASE_DEC, NULL, 0x20,
        NULL, HFILL }
        },
        { &hf_pfcp_spare_b6,
        { "Spare", "pfcp.spare_b6",
        FT_UINT8, BASE_DEC, NULL, 0x40,
        NULL, HFILL }
        },
        { &hf_pfcp_spare_b7,
        { "Spare", "pfcp.spare_b7",
        FT_UINT8, BASE_DEC, NULL, 0x80,
        NULL, HFILL }
        },
        { &hf_pfcp_spare_b7_b6,
        { "Spare", "pfcp.spare_b7_b6",
        FT_UINT8, BASE_DEC, NULL, 0xc0,
        NULL, HFILL }
        },
        { &hf_pfcp_spare_b7_b5,
        { "Spare", "pfcp.spare_b7_b5",
        FT_UINT8, BASE_DEC, NULL, 0xe0,
        NULL, HFILL }
        },
        { &hf_pfcp_spare_b7_b4,
        { "Spare", "pfcp.spare_b7_b4",
        FT_UINT8, BASE_DEC, NULL, 0xf0,
        NULL, HFILL }
        },
        { &hf_pfcp_spare_b7_b3,
        { "Spare", "pfcp.spare_b7_b3",
        FT_UINT8, BASE_DEC, NULL, 0xf8,
        NULL, HFILL }
        },
        { &hf_pfcp_spare_b7_b2,
        { "Spare", "pfcp.spare_b7_b2",
        FT_UINT8, BASE_DEC, NULL, 0xfc,
        NULL, HFILL }
        },
        { &hf_pfcp_spare_b7_b1,
        { "Spare", "pfcp.spare_b7_b1",
        FT_UINT8, BASE_DEC, NULL, 0xfe,
        NULL, HFILL }
        },
        { &hf_pfcp_spare_oct,
        { "Spare", "pfcp.spare_oct",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }
        },
        { &hf_pfcp_spare_h0,
        { "Spare", "pfcp.spare_h0",
        FT_UINT8, BASE_DEC, NULL, 0x0f,
        NULL, HFILL }
        },
        { &hf_pfcp_spare_h1,
        { "Spare", "pfcp.spare_h1",
        FT_UINT8, BASE_DEC, NULL, 0xf0,
        NULL, HFILL }
        },
        { &hf_pfcp_spare,
        { "Spare", "pfcp.spare",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }
        },
        { &hf_pfcp_seid,
        { "SEID", "pfcp.seid",
        FT_UINT64, BASE_HEX, NULL, 0x0,
        NULL, HFILL }
        },
        { &hf_pfcp_seqno,
        { "Sequence Number", "pfcp.seqno",
        FT_UINT24, BASE_DEC, NULL, 0x0,
        NULL, HFILL }
        },
        { &hf_pfcp_response_in,
        { "Response In", "pfcp.response_in",
        FT_FRAMENUM, BASE_NONE, FRAMENUM_TYPE(FT_FRAMENUM_RESPONSE), 0x0,
        "The response to this PFCP request is in this frame", HFILL }
        },
        { &hf_pfcp_response_to,
        { "Response To", "pfcp.response_to",
        FT_FRAMENUM, BASE_NONE, FRAMENUM_TYPE(FT_FRAMENUM_RESPONSE), 0x0,
        "This is a response to the PFCP request in this frame", HFILL }
        },
        { &hf_pfcp_response_time,
        { "Response Time", "pfcp.response_time",
        FT_RELATIVE_TIME, BASE_NONE, NULL, 0x0,
        "The time between the Request and the Response", HFILL }
        },
        { &hf_pfcp_session,
        { "Session", "pfcp.session",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }
        },
        { &hf_pfcp_mp,
        { "Message Priority", "pfcp.mp",
        FT_UINT24, BASE_DEC, NULL, 0xf0,
        NULL, HFILL }
        },
        { &hf_pfcp_enterprise_id,
        { "Enterprise ID",    "pfcp.enterprise_id",
        FT_UINT16, BASE_ENTERPRISES, STRINGS_ENTERPRISES,
        0x0, NULL, HFILL } },
        { &hf_pfcp_enterprise_data,
        { "Enterprise IE Data",    "pfcp.enterprise_ie_data",
            FT_BYTES, BASE_NONE, NULL, 0,
            NULL, HFILL }
        },
        { &hf_pfcp2_ie,
        { "IE Type", "pfcp.ie_type",
        FT_UINT16, BASE_DEC | BASE_EXT_STRING, &pfcp_ie_type_ext, 0x0,
        NULL, HFILL }
        },
        { &hf_pfcp2_enterprise_ie,
        { "Enterprise specific IE Type", "pfcp.enterprise_ie",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL }
        },
        { &hf_pfcp2_ie_len,
        { "IE Length", "pfcp.ie_len",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL }
        },
        { &hf_pfcp_recovery_time_stamp,
        { "Recovery Time Stamp", "pfcp.recovery_time_stamp",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }
        },
        { &hf_pfcp2_cause,
        { "Cause", "pfcp.cause",
        FT_UINT8, BASE_DEC, VALS(pfcp_cause_vals), 0x0,
        NULL, HFILL }
        },
        { &hf_pfcp_node_id_type,
        { "Node ID Type", "pfcp.node_id_type",
            FT_UINT8, BASE_DEC, VALS(pfcp_node_id_type_vals), 0x0f,
            NULL, HFILL }
        },
        { &hf_pfcp_node_id_ipv4,
        { "Node ID IPv4", "pfcp.node_id_ipv4",
            FT_IPv4, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_node_id_ipv6,
        { "Node ID IPv6", "pfcp.node_id_ipv6",
            FT_IPv6, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_node_id_fqdn,
        { "Node ID FQDN", "pfcp.node_id_fqdn",
            FT_STRING, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_f_seid_flags,
        { "Flags", "pfcp.f_seid_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_b0_v6,
        { "V6 (IPv6)", "pfcp.f_seid_flags.v6",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_b1_v4,
        { "V4 (IPv4)", "pfcp.f_seid_flags.v4",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_f_seid_ipv4,
        { "IPv4 address", "pfcp.f_seid.ipv4",
            FT_IPv4, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_f_seid_ipv6,
        { "IPv6 address", "pfcp.f_seid.ipv6",
            FT_IPv6, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_pdr_id,
        { "Rule ID", "pfcp.pdr_id",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_precedence,
        { "Precedence", "pfcp.precedence",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_source_interface,
        { "Source Interface", "pfcp.source_interface",
            FT_UINT8, BASE_DEC, VALS(pfcp_source_interface_vals), 0x0f,
            NULL, HFILL }
        },
        { &hf_pfcp_f_teid_flags,
        { "Flags", "pfcp.f_teid_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_fteid_flg_spare,
        { "Spare", "pfcp.fteid_flg.spare",
            FT_UINT8, BASE_DEC, NULL, 0xf0,
            NULL, HFILL }
        },
        { &hf_pfcp_fteid_flg_b3_ch_id,
        { "CHID (CHOOSE_ID)", "pfcp.f_teid_flags.ch_id",
            FT_BOOLEAN, 8, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_pfcp_fteid_flg_b2_ch,
        { "CH (CHOOSE)", "pfcp.f_teid_flags.ch",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_fteid_flg_b1_v6,
        { "V6 (IPv6)", "pfcp.f_teid_flags.v6",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_fteid_flg_b0_v4,
        { "V4 (IPv4)", "pfcp.f_teid_flags.v4",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_f_teid_ch_id,
        { "Choose Id", "pfcp.f_teid.choose_id",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_f_teid_teid,
        { "TEID", "pfcp.f_teid.teid",
            FT_UINT32, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_f_teid_ipv4,
        { "IPv4 address", "pfcp.f_teid.ipv4_addr",
            FT_IPv4, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_f_teid_ipv6,
        { "IPv6 address", "pfcp.f_teid.ipv6_addr",
            FT_IPv6, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_network_instance,
        { "Network Instance", "pfcp.network_instance",
            FT_STRING, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_pdn_type,
        { "PDN Type", "pfcp.pdn_type",
            FT_UINT8, BASE_DEC, VALS(pfcp_pdn_type_vals), 0x7,
            NULL, HFILL }
        },
        { &hf_pfcp_multiplier_value_digits,
        { "Value Digits", "pfcp.multiplier.value_digits",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_multiplier_exponent,
        { "Exponent", "pfcp.multiplier.exponent",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_aggregated_urr_id_ie_urr_id,
        { "URR ID", "pfcp.aggregated_urr_id_ie.urr_id",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_failed_rule_id_type,
        { "Failed Rule ID Type", "pfcp.failed_rule_id_type",
            FT_UINT8, BASE_DEC, VALS(pfcp_failed_rule_id_type_vals), 0x7,
            NULL, HFILL }
        },
        { &hf_pfcp_time_qouta_mechanism_bti_type,
        { "Base Time Interval Type", "pfcp.time_qouta_mechanism_bti_type",
            FT_UINT8, BASE_DEC, VALS(pfcp_time_qouta_mechanism_bti_type_vals), 0x3,
            NULL, HFILL }
        },
        { &hf_pfcp_time_qouta_mechanism_bti,
        { "Base Time Interval", "pfcp.time_qouta_mechanism_bti",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_ue_ip_address_flags,
        { "Flags", "pfcp.ue_ip_address_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_ue_ip_address_flag_b0_v6,
        { "V6 (IPv6)", "pfcp.ue_ip_address_flag.v6",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_ue_ip_address_flag_b1_v4,
        { "V4 (IPv4)", "pfcp.ue_ip_address_flag.v4",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_ue_ip_address_flag_b2_sd,
        { "S/D", "pfcp.ue_ip_address_flag.sd",
            FT_BOOLEAN, 8, TFS(&pfcp_ue_ip_add_sd_flag_vals), 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_ue_ip_address_flag_b3_v6d,
        { "IPv6D", "pfcp.ue_ip_address_flag.v6d",
            FT_BOOLEAN, 8, TFS(&pfcp_ue_ip_add_sd_flag_vals), 0x08,
            NULL, HFILL }
        },
        { &hf_pfcp_ue_ip_addr_ipv4,
        { "IPv4 address", "pfcp.ue_ip_addr_ipv4",
            FT_IPv4, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_ue_ip_add_ipv6,
        { "IPv6 address", "pfcp.ue_ip_addr_ipv6",
            FT_IPv6, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_ue_ip_add_ipv6_prefix,
        { "IPv6 Prefix", "pfcp.ue_ip_addr_ipv6_prefix",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_application_id,
        { "Application Identifier", "pfcp.application_id",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_application_id_str,
        { "Application Identifier", "pfcp.application_id_str",
            FT_STRING, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_sdf_filter_flags,
        { "Flags", "pfcp.sdf_filter_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_sdf_filter_flags_b0_fd,
        { "FD (Flow Description)", "pfcp.sdf_filter.fd",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_sdf_filter_flags_b1_ttc,
        { "TTC (ToS Traffic Class)", "pfcp.sdf_filter.ttc",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_sdf_filter_flags_b2_spi,
        { "SPI (Security Parameter Index)", "pfcp.sdf_filter.spi",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_sdf_filter_flags_b3_fl,
        { "FL (Flow Label)", "pfcp.sdf_filter.fl",
            FT_BOOLEAN, 8, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_pfcp_sdf_filter_flags_b4_bid,
        { "BID (Bidirectional SDF Filter)", "pfcp.sdf_filter.bid",
            FT_BOOLEAN, 8, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_pfcp_flow_desc_len,
        { "Length of Flow Description", "pfcp.flow_desc_len",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_flow_desc,
        { "Flow Description", "pfcp.flow_desc",
            FT_STRING, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_traffic_class,
        { "ToS Traffic Class", "pfcp.traffic_class",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_traffic_mask,
        { "Mask field", "pfcp.traffic_mask",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_spi,
        { "Security Parameter Index", "pfcp.spi",
            FT_UINT32, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_flow_label_spare_bit,
        { "Spare bit", "pfcp.flow_label_spare_bit",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_flow_label,
        { "Flow Label", "pfcp.flow_label",
            FT_UINT24, BASE_HEX, NULL, 0x0FFFFF,
            NULL, HFILL }
        },
        { &hf_pfcp_sdf_filter_id,
        { "SDF Filter ID", "pfcp.sdf_filter_id",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_pfcp_out_hdr_desc,
        { "Outer Header Removal Description", "pfcp.out_hdr_desc",
            FT_UINT8, BASE_DEC, VALS(pfcp_out_hdr_desc_vals), 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_far_id_flg,
        { "Allocation type", "pfcp.far_id_flg",
            FT_BOOLEAN, 32, TFS(&pfcp_id_predef_dynamic_tfs), 0x80000000,
            NULL, HFILL }
        },
        { &hf_pfcp_far_id,
        { "FAR ID", "pfcp.far_id",
            FT_UINT32, BASE_DEC, NULL, 0x7fffffff,
            NULL, HFILL }
        },
        { &hf_pfcp_urr_id_flg,
        { "Allocation type", "pfcp.urr_id_flg",
            FT_BOOLEAN, 32, TFS(&pfcp_id_predef_dynamic_tfs), 0x80000000,
            NULL, HFILL }
        },
        { &hf_pfcp_urr_id,
        { "URR ID", "pfcp.urr_id",
            FT_UINT32, BASE_DEC, NULL, 0x7fffffff,
            NULL, HFILL }
        },
        { &hf_pfcp_qer_id_flg,
        { "Allocation type", "pfcp.qer_id_flg",
            FT_BOOLEAN, 32, TFS(&pfcp_id_predef_dynamic_tfs), 0x80000000,
            NULL, HFILL }
        },
        { &hf_pfcp_qer_id,
        { "QER ID", "pfcp.qer_id",
            FT_UINT32, BASE_DEC, NULL, 0x7fffffff,
            NULL, HFILL }
        },
        { &hf_pfcp_predef_rules_name,
        { "Predefined Rules Name", "pfcp.predef_rules_name",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_apply_action_flags,
        { "Flags", "pfcp.apply_action_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_apply_action_flags_b0_drop,
        { "DROP (Drop)", "pfcp.apply_action.drop",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_apply_action_flags_b1_forw,
        { "FORW (Forward)", "pfcp.apply_action.forw",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_apply_action_flags_b2_buff,
        { "BUFF (Buffer)", "pfcp.apply_action.buff",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_apply_action_flags_b3_nocp,
        { "NOCP (Notify the CP function)", "pfcp.apply_action.nocp",
            FT_BOOLEAN, 8, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_pfcp_apply_action_flags_b4_dupl,
        { "DUPL (Duplicate)", "pfcp.apply_action.dupl",
            FT_BOOLEAN, 8, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_pfcp_bar_id,
        { "BAR ID", "pfcp.bar_id",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_fq_csid_node_id_type,
        { "FQ-CSID Node-ID Type", "pfcp.fq_csid_node_id_type",
            FT_UINT8, BASE_DEC, VALS(pfcp_fq_csid_node_id_type_vals), 0xf0,
            NULL, HFILL }
        },
        { &hf_pfcp_num_csid,
        { "Number of CSID", "pfcp.num_csid",
            FT_UINT8, BASE_DEC, NULL, 0x0f,
            NULL, HFILL }
        },
        { &hf_pfcp_fq_csid_node_id_ipv4,
        { "Node-Address", "pfcp.q_csid_node_id.ipv4",
            FT_IPv4, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_fq_csid_node_id_ipv6,
        { "Node-Address", "pfcp.q_csid_node_id.ipv6",
            FT_IPv6, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_fq_csid_node_id_mcc_mnc,
        { "Node-Address MCC MNC", "pfcp.q_csid_node_id.mcc_mnc",
            FT_UINT32, BASE_DEC, NULL, 0xfffff000,
            NULL, HFILL }
        },
        { &hf_pfcp_fq_csid_node_id_int,
        { "Node-Address Number", "pfcp.q_csid_node_id.int",
            FT_UINT32, BASE_DEC, NULL, 0x00000fff,
            NULL, HFILL }
        },
        { &hf_pfcp_fq_csid,
        { "PDN Connection Set Identifier (CSID)", "pfcp.csid",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_measurement_period,
        { "Measurement Period", "pfcp.measurement_period",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_duration_measurement,
        { "Duration", "pfcp.duration_measurement",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_time_of_first_packet,
        { "Time of First Packet", "pfcp.time_of_first_packet",
            FT_STRING, BASE_NONE, NULL, 0,
            NULL, HFILL }
        },
        { &hf_pfcp_time_of_last_packet,
        { "Time of Last Packet", "pfcp.time_of_last_packet",
            FT_STRING, BASE_NONE, NULL, 0,
            NULL, HFILL }
        },
        { &hf_pfcp_dst_interface,
        { "Interface", "pfcp.dst_interface",
            FT_UINT8, BASE_DEC, VALS(pfcp_dst_interface_vals), 0x0f,
            NULL, HFILL }
        },
        { &hf_pfcp_redirect_address_type,
        { "Redirect Address Type", "pfcp.redirect_address_type",
            FT_UINT8, BASE_DEC, VALS(pfcp_redirect_address_type_vals), 0x0f,
            NULL, HFILL }
        },
        { &hf_pfcp_redirect_server_addr_len,
        { "Redirect Server Address Length", "pfcp.redirect_server_addr_len",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_redirect_server_address,
        { "Redirect Server Address", "pfcp.redirect_server_address",
            FT_STRING, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_linked_urr_id,
        { "Linked URR ID", "pfcp.linked_urr_id",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_outer_hdr_desc,
        { "Outer Header Creation Description", "pfcp.outer_hdr_desc",
            FT_UINT16, BASE_DEC, VALS(pfcp_outer_hdr_desc_vals), 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_outer_hdr_creation_teid,
        { "TEID", "pfcp.outer_hdr_creation.teid",
            FT_UINT32, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_outer_hdr_creation_ipv4,
        { "IPv4 Address", "pfcp.outer_hdr_creation.ipv4",
            FT_IPv4, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_outer_hdr_creation_ipv6,
        { "IPv6 Address", "pfcp.outer_hdr_creation.ipv6",
            FT_IPv6, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_outer_hdr_creation_port,
        { "Port Number", "pfcp.outer_hdr_creation.port",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_time_threshold,
        { "Time Threshold", "pfcp.time_threshold",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_forwarding_policy_id_len,
        { "Forwarding Policy Identifier Length", "pfcp.forwarding_policy_id_len",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_forwarding_policy_id,
        { "Forwarding Policy Identifier", "pfcp.forwarding_policy_id",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_measurement_method_flags,
        { "Flags", "pfcp.measurement_method_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_measurement_method_flags_b0_durat,
        { "DURAT (Duration)", "pfcp.measurement_method_flags.durat",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_measurement_method_flags_b1_volume,
        { "VOLUM (Volume)", "pfcp.measurement_method_flags.volume",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_measurement_method_flags_b2_event,
        { "EVENT (Event)", "pfcp.measurement_method_flags.event",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_subsequent_time_threshold,
        { "Subsequent Time Threshold", "pfcp.subsequent_time_threshold",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_inactivity_detection_time,
        { "Inactivity Detection Time", "pfcp.inactivity_detection_time",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_monitoring_time,
        { "Monitoring Time", "pfcp.monitoring_time",
            FT_STRING, BASE_NONE, NULL, 0,
            NULL, HFILL }
        },
        { &hf_pfcp_reporting_triggers_o5_b0_perio,
        { "PERIO (Periodic Reporting)", "pfcp.reporting_triggers_flags.perio",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_reporting_triggers_o5_b1_volth,
        { "VOLTH (Volume Threshold)", "pfcp.reporting_triggers_flags.volth",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_reporting_triggers_o5_b2_timth,
        { "TIMTH (Time Threshold)", "pfcp.reporting_triggers_flags.timth",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_reporting_triggers_o5_b3_quhti,
        { "QUHTI (Quota Holding Time)", "pfcp.reporting_triggers_flags.quhti",
            FT_BOOLEAN, 8, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_pfcp_reporting_triggers_o5_b4_start,
        { "START (Start of Traffic)", "pfcp.reporting_triggers_flags.start",
            FT_BOOLEAN, 8, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_pfcp_reporting_triggers_o5_b5_stopt,
        { "STOPT (Stop of Traffic)", "pfcp.reporting_triggers_flags.stopt",
            FT_BOOLEAN, 8, NULL, 0x20,
            NULL, HFILL }
        },
        { &hf_pfcp_reporting_triggers_o5_b6_droth,
        { "DROTH (Dropped DL Traffic Threshold)", "pfcp.reporting_triggers_flags.droth",
            FT_BOOLEAN, 8, NULL, 0x40,
            NULL, HFILL }
        },
        { &hf_pfcp_reporting_triggers_o5_b7_liusa,
        { "LIUSA (Linked Usage Reporting)", "pfcp.reporting_triggers_flags.liusa",
            FT_BOOLEAN, 8, NULL, 0x80,
            NULL, HFILL }
        },
        { &hf_pfcp_reporting_triggers_o6_b0_volqu,
        { "VOLQU (Volume Quota)", "pfcp.reporting_triggers_flags.volqu",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_reporting_triggers_o6_b1_timqu,
        { "TIMQU (Time Quota)", "pfcp.reporting_triggers_flags.timqu",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_reporting_triggers_o6_b2_envcl,
        { "ENVCL (Envelope Closure)", "pfcp.reporting_triggers_flags.envcl",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_reporting_triggers_o6_b3_macar,
        { "MACAR (MAC Addresses Reporting)", "pfcp.reporting_triggers_flags.macar",
            FT_BOOLEAN, 8, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_pfcp_reporting_triggers_o6_b4_eveth,
        { "EVETH (Event Threshold)", "pfcp.reporting_triggers_flags.eveth",
            FT_BOOLEAN, 8, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_pfcp_reporting_triggers_o6_b5_evequ,
        { "EVEQU (Event Quota)", "pfcp.reporting_triggers_flags.evequ",
            FT_BOOLEAN, 8, NULL, 0x20,
            NULL, HFILL }
        },

        { &hf_pfcp_usage_report_trigger_o7_b0_evequ,
        { "EVEQU (Event Quota)", "pfcp.usage_report_trigger_flags.evequ",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_report_trigger_o6_b0_volqu,
        { "VOLQU (Volume Quota)", "pfcp.usage_report_trigger_flags.volqu",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_report_trigger_o6_b1_timqu,
        { "TIMQU (Time Quota)", "pfcp.usage_report_trigger_flags.timqu",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_report_trigger_o6_b2_liusa,
        { "LIUSA (Linked Usage Reporting)", "pfcp.usage_report_trigger_flags.liusa",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_report_trigger_o6_b3_termr,
        { "TERMR (Termination Report)", "pfcp.usage_report_trigger.term",
            FT_BOOLEAN, 8, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_report_trigger_o6_b4_monit,
        { "MONIT (Monitoring Time)", "pfcp.usage_report_trigger.monit",
            FT_BOOLEAN, 8, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_report_trigger_o6_b5_envcl,
        { "ENVCL (Envelope Closure)", "pfcp.usage_report_trigger_flags.envcl",
            FT_BOOLEAN, 8, NULL, 0x20,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_report_trigger_o6_b7_eveth,
        { "EVETH (Event Threshold)", "pfcp.usage_report_trigger_flags.eveth",
            FT_BOOLEAN, 8, NULL, 0x80,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_report_trigger_o6_b6_macar,
        { "MACAR (MAC Addresses Reporting)", "pfcp.usage_report_trigger_flags.macar",
            FT_BOOLEAN, 8, NULL, 0x40,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_report_trigger_o5_b0_perio,
        { "PERIO (Periodic Reporting)", "pfcp.usage_report_trigger_flags.perio",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_report_trigger_o5_b1_volth,
        { "VOLTH (Volume Threshold)", "pfcp.usage_report_trigger_flags.volth",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_report_trigger_o5_b2_timth,
        { "TIMTH (Time Threshold)", "pfcp.usage_report_trigger_flags.timth",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_report_trigger_o5_b3_quhti,
        { "QUHTI (Quota Holding Time)", "pfcp.usage_report_trigger_flags.quhti",
            FT_BOOLEAN, 8, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_report_trigger_o5_b4_start,
        { "START (Start of Traffic)", "pfcp.usage_report_trigger_flags.start",
            FT_BOOLEAN, 8, NULL, 0x10,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_report_trigger_o5_b5_stopt,
        { "STOPT (Stop of Traffic)", "pfcp.usage_report_trigger_flags.stopt",
            FT_BOOLEAN, 8, NULL, 0x20,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_report_trigger_o5_b6_droth,
        { "DROTH (Dropped DL Traffic Threshold)", "pfcp.usage_report_trigger_flags.droth",
            FT_BOOLEAN, 8, NULL, 0x40,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_report_trigger_o5_b7_immer,
        { "IMMER (Immediate Report)", "pfcp.usage_report_trigger.immer",
            FT_BOOLEAN, 8, NULL, 0x80,
            NULL, HFILL }
        },

        { &hf_pfcp_volume_threshold,
        { "Flags", "pfcp.volume_threshold",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_volume_threshold_b0_tovol,
        { "TOVOL", "pfcp.volume_threshold_flags.tovol",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_volume_threshold_b1_ulvol,
        { "ULVOL", "pfcp.volume_threshold_flags.ulvol",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_volume_threshold_b2_dlvol,
        { "DLVOL", "pfcp.volume_threshold_flags.dlvol",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_volume_threshold_tovol,
        { "Total Volume", "pfcp.volume_threshold.tovol",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_volume_threshold_ulvol,
        { "Uplink Volume", "pfcp.volume_threshold.ulvol",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_volume_threshold_dlvol,
        { "Downlink Volume", "pfcp.volume_threshold.dlvol",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_volume_quota,
        { "Flags", "pfcp.volume_quota",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_volume_quota_b0_tovol,
        { "TOVOL", "pfcp.volume_quota_flags.tovol",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_volume_quota_b1_ulvol,
        { "ULVOL", "pfcp.volume_quota_flags.ulvol",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_volume_quota_b2_dlvol,
        { "DLVOL", "pfcp.volume_quota_flags.dlvol",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_volume_quota_tovol,
        { "Total Volume", "pfcp.volume_quota.tovol",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_volume_quota_ulvol,
        { "Uplink Volume", "pfcp.volume_quota.ulvol",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_volume_quota_dlvol,
        { "Downlink Volume", "pfcp.volume_quota.dlvol",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_subseq_volume_threshold,
        { "Flags", "pfcp.subseq_volume_threshold",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_subseq_volume_threshold_b0_tovol,
        { "TOVOL", "pfcp.subseq_volume_threshold.tovol_flg",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_subseq_volume_threshold_b1_ulvol,
        { "ULVOL", "pfcp.subseq_volume_threshold.ulvol_flg",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_subseq_volume_threshold_b2_dlvol,
        { "DLVOL", "pfcp.subseq_volume_threshold.dlvol_flg",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_subseq_volume_threshold_tovol,
        { "Total Volume", "pfcp.subseq_volume_threshold.tovol",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_subseq_volume_threshold_ulvol,
        { "Uplink Volume", "pfcp.subseq_volume_threshold.ulvol",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_subseq_volume_threshold_dlvol,
        { "Downlink Volume", "pfcp.subseq_volume_threshold.dlvol",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_time_quota,
        { "Time Quota", "pfcp.time_quota",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_start_time,
        { "Start Time", "pfcp.start_time",
            FT_STRING, BASE_NONE, NULL, 0,
            NULL, HFILL }
        },
        { &hf_pfcp_end_time,
        { "End Time", "pfcp.start_time",
            FT_STRING, BASE_NONE, NULL, 0,
            NULL, HFILL }
        },
        { &hf_pfcp_quota_holding_time,
        { "Quota Holding Time", "pfcp.quota_holding_time",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_dropped_dl_traffic_threshold,
        { "Flags", "pfcp.dropped_dl_traffic_threshold",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_dropped_dl_traffic_threshold_b0_dlpa,
        { "DLPA", "pfcp.dropped_dl_traffic_threshold.dlpa_flg",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_dropped_dl_traffic_threshold_b1_dlby,
        { "DLBY", "pfcp.dropped_dl_traffic_threshold.dlby_flg",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_downlink_packets,
        { "Downlink Packets", "pfcp.downlink_packets",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_bytes_downlink_data,
        { "Bytes of Downlink Data", "pfcp.bytes_downlink_data",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_qer_correlation_id,
        { "QER Correlation ID", "pfcp.qer_correlation_id",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_gate_status,
        { "Flags", "pfcp.gate_status",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_gate_status_b0b1_dlgate,
        { "DL Gate", "pfcp.gate_status.ulgate",
            FT_UINT8, BASE_DEC, VALS(pfcp_gate_status_vals), 0x03,
            NULL, HFILL }
        },
        { &hf_pfcp_gate_status_b3b2_ulgate,
        { "UL Gate", "pfcp.gate_status.ulgate",
            FT_UINT8, BASE_DEC, VALS(pfcp_gate_status_vals), 0x0c,
            NULL, HFILL }
        },
        { &hf_pfcp_ul_mbr,
        { "UL MBR", "pfcp.ul_mbr",
            FT_UINT40, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_dl_mbr,
        { "DL MBR", "pfcp.dl_mbr",
            FT_UINT40, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_ul_gbr,
        { "UL GBR", "pfcp.ul_gbr",
            FT_UINT40, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_dl_gbr,
        { "DL GBR", "pfcp.dl_gbr",
            FT_UINT40, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_report_type,
        { "Flags", "pfcp.report_type",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_report_type_b3_upir,
        { "UPIR (User Plane Inactivity Report)", "pfcp.report_type.upir",
            FT_BOOLEAN, 8, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_pfcp_report_type_b2_erir,
        { "ERIR (Error Indication Report)", "pfcp.report_type.erir",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_report_type_b1_usar,
        { "USAR (Usage Report)", "pfcp.report_type.usar",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_report_type_b0_dldr,
        { "DLDR (Downlink Data Report)", "pfcp.report_type.dldr",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_offending_ie,
        { "Type of the offending IE", "pfcp.offending_ie",
            FT_UINT16, BASE_DEC | BASE_EXT_STRING, &pfcp_ie_type_ext, 0x0,
            NULL, HFILL }
        },

        { &hf_pfcp_up_function_features_o5_b0_bucp,
        { "BUCP", "pfcp.up_function_features.bucp",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x01,
            "Downlink Data Buffering in CP function", HFILL }
        },
        { &hf_pfcp_up_function_features_o5_b1_ddnd,
        { "DDND", "pfcp.up_function_features.ddnd",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x02,
            "Buffering parameter 'Downlink Data Notification Delay", HFILL }
        },
        { &hf_pfcp_up_function_features_o5_b2_dlbd,
        { "DLBD", "pfcp.up_function_features.dlbd",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_up_function_features_o5_b3_trst,
        { "TRST", "pfcp.up_function_features.trst",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x08,
            "Traffic Steering", HFILL }
        },
        { &hf_pfcp_up_function_features_o5_b4_ftup,
        { "FTUP", "pfcp.up_function_features.ftup",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x10,
            "F-TEID allocation / release in the UP function", HFILL }
        },
        { &hf_pfcp_up_function_features_o5_b5_pfdm,
        { "PFDM", "pfcp.up_function_features.pfdm",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x20,
            "PFD Management procedure", HFILL }
        },
        { &hf_pfcp_up_function_features_o5_b6_heeu,
        { "HEEU", "pfcp.up_function_features.heeu",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x40,
            "Header Enrichment of Uplink traffic", HFILL }
        },
        { &hf_pfcp_up_function_features_o5_b7_treu,
        { "TREU", "pfcp.up_function_features.treu",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x80,
            "Traffic Redirection Enforcement in the UP function", HFILL }
        },
        { &hf_pfcp_up_function_features_o6_b0_empu,
        { "EMPU", "pfcp.up_function_features.empu",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x01,
            "Sending of End Marker packets", HFILL }
        },
        { &hf_pfcp_up_function_features_o6_b1_pdiu,
        { "PDIU", "pfcp.up_function_features.pdiu",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x02,
            "Support of PDI optimised signalling", HFILL }
        },
        { &hf_pfcp_up_function_features_o6_b2_udbc,
        { "UDBC", "pfcp.up_function_features.udbc",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x04,
            "Support of UL/DL Buffering Control", HFILL }
        },
        { &hf_pfcp_up_function_features_o6_b3_quoac,
        { "QUOAC", "pfcp.up_function_features.quoac",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x08,
            "The UP function supports being provisioned with the Quota Action to apply when reaching quotas.", HFILL }
        },
        { &hf_pfcp_up_function_features_o6_b4_trace,
        { "TRACE", "pfcp.up_function_features.trace",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x10,
            "The UP function supports Trace", HFILL }
        },
        { &hf_pfcp_up_function_features_o6_b5_frrt,
        { "FRRT", "pfcp.up_function_features.frrt",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x20,
            "The UP function supports Framed Routing", HFILL }
        },
        { &hf_pfcp_sequence_number,
        { "Sequence Number", "pfcp.sequence_number",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_metric,
        { "Metric", "pfcp.metric",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_timer_unit,
        { "Timer unit", "pfcp.timer_unit",
            FT_UINT8, BASE_DEC, VALS(pfcp_timer_unit_vals), 0xe0,
            NULL, HFILL }
        },
        { &hf_pfcp_timer_value,
        { "Timer value", "pfcp.timer_value",
            FT_UINT8, BASE_DEC, NULL, 0x1f,
            NULL, HFILL }
        },
        { &hf_pfcp_volume_measurement,
        { "Flags", "pfcp.volume_measurement",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_volume_measurement_b0_tovol,
        { "TOVOL", "pfcp.volume_measurement_flags.tovol",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_volume_measurement_b1_ulvol,
        { "ULVOL", "pfcp.volume_measurement_flags.ulvol",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_volume_measurement_b2_dlvol,
        { "DLVOL", "pfcp.volume_measurement_flags.dlvol",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_vol_meas_tovol,
        { "Total Volume", "pfcp.volume_measurement.tovol",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_vol_meas_ulvol,
        { "Uplink Volume", "pfcp.volume_measurement.ulvol",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_vol_meas_dlvol,
        { "Downlink Volume", "pfcp.volume_measurement.dlvol",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_cp_function_features,
        { "Flags", "pfcp.cp_function_features",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_cp_function_features_b0_load,
        { "LOAD", "pfcp.cp_function_features.load",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x01,
            "Load Control", HFILL }
        },
        { &hf_pfcp_cp_function_features_b1_ovrl,
        { "OVRL", "pfcp.cp_function_features.ovrl",
            FT_BOOLEAN, 8, TFS(&tfs_supported_not_supported), 0x02,
            "Overload Control", HFILL }
        },
        { &hf_pfcp_usage_information,
        { "Flags", "pfcp.usage_information",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_information_b0_bef,
        { "BEF (Before)", "pfcp.usage_information.bef",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_information_b1_aft,
        { "AFT (After)", "pfcp.usage_information.aft",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_information_b2_uae,
        { "UAE (Usage After Enforcement)", "pfcp.usage_information.uae",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_usage_information_b3_ube,
        { "UBE (Usage Before Enforcement)", "pfcp.usage_information.ube",
            FT_BOOLEAN, 8, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_pfcp_application_instance_id,
        { "Application Instance Identifier", "pfcp.application_instance_id",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_application_instance_id_str,
        { "Application Instance Identifier", "pfcp.application_instance_id_str",
            FT_STRING, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_flow_dir,
        { "Flow Direction", "pfcp.flow_dir",
            FT_UINT8, BASE_DEC, VALS(pfcp_flow_dir_vals), 0x07,
            NULL, HFILL }
        },
        { &hf_pfcp_packet_rate,
        { "Flags", "pfcp.packet_rate",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_packet_rate_b0_ulpr,
        { "ULPR (Uplink Packet Rate)", "pfcp.packet_rate.ulpr",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_packet_rate_b1_dlpr,
        { "DLPR (Downlink Packet Rate)", "pfcp.packet_rate.dlpr",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_ul_time_unit,
        { "Uplink Time Unit", "pfcp.ul_time_unit",
            FT_UINT8, BASE_DEC, VALS(pfcp_pr_time_unit_vals), 0x07,
            NULL, HFILL }
        },
        { &hf_pfcp_max_ul_pr,
        { "Maximum Uplink Packet Rate", "pfcp.max_ul_pr",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_dl_time_unit,
        { "Downlink Time Unit", "pfcp.dl_time_unit",
            FT_UINT8, BASE_DEC, VALS(pfcp_pr_time_unit_vals), 0x07,
            NULL, HFILL }
        },
        { &hf_pfcp_max_dl_pr,
        { "Maximum Downlink Packet Rate", "pfcp.max_dl_pr",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_dl_flow_level_marking,
        { "Flags", "pfcp.dl_flow_level_marking",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_dl_flow_level_marking_b0_ttc,
        { "TTC (ToS/Traffic Class)", "pfcp.dl_flow_level_marking.ttc",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_dl_flow_level_marking_b1_sci,
        { "SCI(Service Class Indicator)", "pfcp.dl_flow_level_marking.sci",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_sci,
        { "Service Class Indicator", "pfcp.sci",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_dl_data_notification_delay,
        { "Delay Value", "pfcp.dl_data_notification_delay",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            "Delay Value in integer multiples of 50 millisecs, or zero", HFILL }
        },
        { &hf_pfcp_packet_count,
        { "Packet Count", "pfcp.packet_count",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_dl_data_service_inf_flags,
        { "Flags", "pfcp.dl_data_service_inf_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_dl_data_service_inf_b0_ppi,
        { "PPI(Paging Policy Indication)", "pfcp.dl_data_service_inf.ppi",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_dl_data_service_inf_b1_qfii,
        { "QFII(QoS Flow Identifier)", "pfcp.dl_data_service_inf.qfii",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_ppi,
        { "Paging Policy Indication", "pfcp.dl_data_service_inf.ppi",
            FT_UINT16, BASE_DEC, NULL, 0x7f,
            NULL, HFILL }
        },
        { &hf_pfcp_pfcpsmreq_flags,
        { "Flags", "pfcp.smreq_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_pfcpsmreq_flags_b0_drobu,
        { "DROBU (Drop Buffered Packets)", "pfcp.smreq_flags.drobu",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_pfcpsmreq_flags_b1_sndem,
        { "SNDEM (Send End Marker Packets)", "pfcp.smreq_flags.sndem",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_pfcpsmreq_flags_b2_qaurr,
        { "QAURR (Query All URRs)", "pfcp.smreq_flags.qaurr",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_pfcpsrrsp_flags,
        { "Flags", "pfcp.srrsp_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_pfcpsrrsp_flags_b0_drobu,
        { "DROBU (Drop Buffered Packets)", "pfcp.srrsp_flags.drobu",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_pfd_contents_flags,
        { "Flags", "pfcp.pfd_contents_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_pfd_contents_flags_b0_fd,
        { "FD (Flow Description)", "pfcp.pfd_contents_flags.fd",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_pfd_contents_flags_b1_url,
        { "URL (URL)", "pfcp.pfd_contents_flags.url",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_pfd_contents_flags_b2_dn,
        { "DN (Domain Name)", "pfcp.pfd_contents_flags.dn",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_pfd_contents_flags_b3_cp,
        { "CP (Custom PFD Content)", "pfcp.pfd_contents_flags.cp",
            FT_BOOLEAN, 8, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_pfcp_url_len,
        { "Length of URL", "pfcp.url_len",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_url,
        { "URL", "pfcp.url",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_dn_len,
        { "Length of Domain Name", "pfcp.dn_len",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_dn,
        { "Domain Name", "pfcp.dn",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_cp_len,
        { "Length of Custom PFD Content", "pfcp.cp_len",
            FT_UINT16, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_cp,
        { "Custom PFD Content", "pfcp.cp",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_header_type,
        { "Header Type", "pfcp.header_type",
            FT_UINT8, BASE_DEC, VALS(pfcp_header_type_vals), 0x1f,
            NULL, HFILL }
        },
        { &hf_pfcp_hf_len,
        { "Length of Header Field Name", "pfcp.hf_len",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_hf_name,
        { "Header Field Name", "pfcp.hf_name",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_hf_name_str,
        { "Header Field Name", "pfcp.hf_name_str",
            FT_STRING, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_hf_val_len,
        { "Length of Header Field Value", "pfcp.hf_val_len",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_hf_val,
        { "Header Field Value", "pfcp.hf_val",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_hf_val_str,
        { "Header Field Value", "pfcp.hf_val_str",
            FT_STRING, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_measurement_info,
        { "Flags", "pfcp.measurement_info",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_measurement_info_b0_mbqe,
        { "MBQE (Measurement Before QoS Enforcement)", "pfcp.measurement_info.fd",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_measurement_info_b1_inam,
        { "INAM (Inactive Measurement)", "pfcp.measurement_info.inam",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_measurement_info_b2_radi,
        { "RADI (Reduced Application Detection Information)", "pfcp.measurement_info.radi",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_measurement_info_b3_istm,
        { "ISTM (Immediate Start Time Metering)", "pfcp.measurement_info.istm",
            FT_BOOLEAN, 8, NULL, 0x08,
            NULL, HFILL }
        },
        { &hf_pfcp_node_report_type,
        { "Flags", "pfcp.node_report_type",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_node_report_type_b0_upfr,
        { "UPFR (User Plane Path Failure Report)", "pfcp.node_report_type.upfr",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_remote_gtp_u_peer_flags,
        { "Flags", "pfcp.remote_gtp_u_peer_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_remote_gtp_u_peer_flags_b0_v6,
        { "V6 (IPv6)", "pfcp.remote_gtp_u_peer_flags.v6",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_remote_gtp_u_peer_flags_b1_v4,
        { "V4 (IPv4)", "pfcp.remote_gtp_u_peer_flags.v4",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_remote_gtp_u_peer_ipv4,
        { "IPv4 address", "pfcp.node_id_ipv4",
            FT_IPv4, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_remote_gtp_u_peer_ipv6,
        { "IPv6 address", "pfcp.node_id_ipv6",
            FT_IPv6, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_ur_seqn,
        { "UR-SEQN", "pfcp.ur_seqn",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_oci_flags,
        { "Flags", "pfcp.oci_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_oci_flags_b0_aoci,
        { "AOCI: Associate OCI with Node ID", "pfcp.oci_flags.aoci",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_pfcp_assoc_rel_req_flags,
        { "Flags", "pfcp.assoc_rel_req",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_pfcp_assoc_rel_req_b0_sarr,
        { "SARR (PFCP Association Release Request)", "pfcp.assoc_rel_req.sarr",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_upiri_flags,
        { "Flags", "pfcp.upiri_flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_upiri_flg_b6_assosi,
        { "ASSOSI (Associated Source Instance)", "pfcp.upiri_flags.assosi",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x40,
            NULL, HFILL }
        },
        { &hf_pfcp_upiri_flg_b5_assoni,
        { "ASSONI (Associated Network Instance)", "pfcp.upiri_flags.assoni",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x20,
            NULL, HFILL }
        },
        { &hf_pfcp_upiri_flg_b2b4_teidri,
        { "TEIDRI (TEID Range Indication)", "pfcp.upiri_flags.teidri",
            FT_UINT8, BASE_HEX, NULL, 0x1c,
            NULL, HFILL }
        },
        { &hf_pfcp_upiri_flags_b1_v6,
        { "V6 (IPv6)", "pfcp.upiri_flags.v6",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_upiri_flags_b0_v4,
        { "V4 (IPv4)", "pfcp.upiri_flags.v4",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_upiri_teidri,
        { "TEID Range Indication", "pfcp.upiri.teidri",
            FT_UINT8, BASE_DEC, NULL, 0x1C,
            NULL, HFILL }
        },
        { &hf_pfcp_upiri_teid_range,
        { "TEID", "pfcp.upiri.teid_range",
            FT_UINT32, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_upiri_ipv4,
        { "IPv4 address", "pfcp.upiri.ipv4_addr",
            FT_IPv4, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_upiri_ipv6,
        { "IPv6 address", "pfcp.upiri.ipv6_addr",
            FT_IPv6, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_user_plane_inactivity_timer,
        { "User Plane Inactivity Timer", "pfcp.user_plane_inactivity_time",
            FT_UINT32, BASE_DEC|BASE_UNIT_STRING, &units_seconds, 0,
            NULL, HFILL }
        },

        { &hf_pfcp_subsequent_volume_quota,
        { "Flags", "pfcp.subsequent_volume_quota",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_subsequent_volume_quota_b0_tovol,
        { "TOVOL", "pfcp.subsequent_volume_quota_flags.tovol",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_subsequent_volume_quota_b1_ulvol,
        { "ULVOL", "pfcp.subsequent_volume_quota_flags.ulvol",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_subsequent_volume_quota_b2_dlvol,
        { "DLVOL", "pfcp.subsequent_volume_quota_flags.dlvol",
            FT_BOOLEAN, 8, NULL, 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_subsequent_volume_quota_tovol,
        { "Total Volume", "pfcp.subsequent_volume_quota.tovol",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_subsequent_volume_quota_ulvol,
        { "Uplink Volume", "pfcp.subsequent_volume_quota.ulvol",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_subsequent_volume_quota_dlvol,
        { "Downlink Volume", "pfcp.subsequent_volume_quota.dlvol",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_pfcp_subsequent_time_quota,
        { "Subsequent Time Quota", "pfcp.subsequent_time_quota",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_rqi_flag,
        { "RQI", "pfcp.rqi_flag",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_qfi,
        { "QFI", "pfcp.qfi_value",
            FT_UINT8, BASE_HEX, NULL, 0x7f,
            NULL, HFILL }
        },
        { &hf_pfcp_query_urr_reference,
        { "Query URR Reference", "pfcp.query_urr_reference",
            FT_UINT32, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_pfcp_additional_usage_reports_information,
        { "Additional Usage Reports Information", "pfcp.additional_usage_reports_information",
            FT_UINT16, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_additional_usage_reports_information_b15_auri,
        { "AURI (Additional Usage Reports Indication)", "pfcp.additional_usage_reports_information_auri",
            FT_BOOLEAN, 16, NULL, 0x8000,
            NULL, HFILL }
        },
        { &hf_pfcp_additional_usage_reports_information_b14_b0_number_value,
        { "Number of Additional Usage Reports value", "pfcp.additional_usage_reports_information_value",
            FT_UINT16, BASE_DEC, NULL, 0x7FFF,
            NULL, HFILL }
        },
        { &hf_pfcp_traffic_endpoint_id,
        { "Traffic Endpoint ID", "pfcp.traffic_endpoint_id",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_pfcp_mac_address_flags,
        { "Flags", "pfcp.mac_address.flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_mac_address_flags_b0_sour,
        { "SOUR", "pfcp.mac_address.flags.sour",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_mac_address_flags_b1_dest,
        { "DEST", "pfcp.mac_address.flags.dest",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_mac_address_flags_b2_usou,
        { "USUO", "pfcp.mac_address.flags.usuo",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_mac_address_flags_b3_udes,
        { "UDES", "pfcp.mac_address.flags.udes",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x08,
            NULL, HFILL }
        },
        { &hf_pfcp_mac_address_source_mac_address,
        { "Source MAC Address", "pfcp.mac_address.sour",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_mac_address_dest_mac_address,
        { "Destination MAC Address", "pfcp.mac_address.dest",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_mac_address_upper_source_mac_address,
        { "Upper Source MAC Address", "pfcp.mac_address.usou",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_mac_address_upper_dest_mac_address,
        { "Upper Destination MAC Address", "pfcp.mac_address.udes",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_pfcp_c_tag_flags,
        { "Flags", "pfcp.c_tag.flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_c_tag_flags_b0_pcp,
        { "PCP", "pfcp.c_tag.flags.pcp",
            FT_BOOLEAN, 8, NULL, 0x01,
            "Priority code point", HFILL }
        },
        { &hf_pfcp_c_tag_flags_b1_dei,
        { "DEI", "pfcp.c_tag.flags.dei",
            FT_BOOLEAN, 8, NULL, 0x02,
            "Drop eligible indicator", HFILL }
        },
        { &hf_pfcp_c_tag_flags_b2_vid,
        { "VID", "pfcp.c_tag.flags.vid",
            FT_BOOLEAN, 8, NULL, 0x04,
            "VLAN identifier", HFILL }
        },
        { &hf_pfcp_c_tag_cvid,
        { "C-VLAN", "pfcp.c_tag.cvid",
            FT_UINT8, BASE_HEX, NULL, 0xF0,
            NULL, HFILL }
        },
        { &hf_pfcp_c_tag_dei_flag,
        { "Drop eligible indicator (DEI)", "pfcp.c_tag.dei_flag",
            FT_BOOLEAN, 8, TFS(&tfs_eligible_ineligible), 0x08,
            NULL, HFILL }
        },
        { &hf_pfcp_c_tag_pcp_value,
        { "Priority code point (PCP)", "pfcp.c_tag.pcp",
            FT_UINT8, BASE_DEC, VALS(pfcp_vlan_tag_pcp_vals), 0x07,
            NULL, HFILL }
        },
        { &hf_pfcp_c_tag_cvid_value,
        { "C-VLAN value", "pfcp.c_tag.cvid_value",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_pfcp_s_tag_flags,
        { "Flags", "pfcp.s_tag.flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_s_tag_flags_b0_pcp,
        { "PCP", "pfcp.s_tag.flags.pcp",
            FT_BOOLEAN, 8, NULL, 0x01,
            "Priority code point", HFILL }
        },
        { &hf_pfcp_s_tag_flags_b1_dei,
        { "DEI", "pfcp.s_tag.flags.dei",
            FT_BOOLEAN, 8, NULL, 0x02,
            "Drop eligible indicator", HFILL }
        },
        { &hf_pfcp_s_tag_flags_b2_vid,
        { "VID", "pfcp.s_tag.flags.vid",
            FT_BOOLEAN, 8, NULL, 0x04,
            "VLAN identifier", HFILL }
        },
        { &hf_pfcp_s_tag_svid,
        { "S-VLAN", "pfcp.s_tag.svid",
            FT_UINT8, BASE_HEX, NULL, 0xF0,
            NULL, HFILL }
        },
        { &hf_pfcp_s_tag_dei_flag,
        { "Drop eligible indicator (DEI)", "pfcp.s_tag.dei_flag",
            FT_BOOLEAN, 8, TFS(&tfs_eligible_ineligible), 0x08,
            NULL, HFILL }
        },
        { &hf_pfcp_s_tag_pcp_value,
        { "Priority code point (PCP)", "pfcp.s_tag.pcp",
            FT_UINT8, BASE_DEC, VALS(pfcp_vlan_tag_pcp_vals), 0x07,
            NULL, HFILL }
        },
        { &hf_pfcp_s_tag_svid_value,
        { "S-VLAN value", "pfcp.s_tag.svid_value",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_pfcp_ethertype,
        { "Ethertype", "pfcp.ethertype",
            FT_UINT16, BASE_HEX, VALS(etype_vals), 0x0,
            NULL, HFILL }
        },

        { &hf_pfcp_proxying_flags,
        { "Flags", "pfcp.proxying.flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_proxying_flags_b0_arp,
        { "ARP", "pfcp.proxying.flags.arp",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_proxying_flags_b1_ins,
        { "INS", "pfcp.proxying.flags.ins",
            FT_BOOLEAN, 8, NULL, 0x02,
            NULL, HFILL }
        },

        { &hf_pfcp_ethertype_filter_id,
        { "Ethertype Filter ID", "pfcp.ethertype_filter_id",
            FT_UINT64, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_pfcp_ethertype_filter_properties_flags,
        { "Flags", "pfcp.ethertype_filter_properties.flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_ethertype_filter_properties_flags_b0_bide,
        { "BIDE", "pfcp.ethertype_filter_properties.flags.bide",
            FT_BOOLEAN, 8, NULL, 0x01,
            NULL, HFILL }
        },

        { &hf_pfcp_suggested_buffering_packets_count_packet_count,
        { "Packet count", "pfcp.suggested_buffering_packets_count.packet_count",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_pfcp_user_id_flags,
        { "Flags", "pfcp.user_id.flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_user_id_flags_b0_imsif,
        { "IMSIF", "pfcp.user_id.flags.imsif",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x01,
            NULL, HFILL }
        },
        { &hf_pfcp_user_id_flags_b1_imeif,
        { "IMEIF", "pfcp.user_id.flags.imeif",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x02,
            NULL, HFILL }
        },
        { &hf_pfcp_user_id_flags_b2_msisdnf,
        { "MSISDNF", "pfcp.user_id.flags.msisdnf",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x04,
            NULL, HFILL }
        },
        { &hf_pfcp_user_id_flags_b3_naif,
        { "NAIF", "pfcp.user_id.flags.naif",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x08,
            NULL, HFILL }
        },
        { &hf_pfcp_user_id_length_of_imsi,
        { "Length of IMSI", "pfcp.user_id.length_of_imsi",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_user_id_length_of_imei,
        { "Length of IMEI", "pfcp.user_id.length_of_imei",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_user_id_imei,
        { "IMEI", "pfcp.user_id.imei",
            FT_STRING, BASE_NONE, NULL, 0,
            NULL, HFILL }
        },
        { &hf_pfcp_user_id_length_of_msisdn,
        { "Length of MSISDN", "pfcp.user_id.length_of_msisdn",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_user_id_length_of_nai,
        { "Length of NAI", "pfcp.user_id.length_of_nai",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_user_id_nai,
        { "NAI", "pfcp.user_id.nai",
            FT_STRING, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },


        { &hf_pfcp_ethernet_pdu_session_information_flags,
        { "Flags", "pfcp.ethernet_pdu_session_information.flags",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_ethernet_pdu_session_information_flags_b0_ethi,
        { "IMSIF", "pfcp.ethernet_pdu_session_information.flags.ethi",
            FT_BOOLEAN, 8, TFS(&tfs_present_not_present), 0x01,
            NULL, HFILL }
        },


        { &hf_pfcp_mac_addresses_detected_number_of_mac_addresses,
        { "Number of MAC addresses", "pfcp.mac_addresses_detected.number_of_mac_addresses",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_mac_addresses_detected_mac_address,
        { "MAC Address", "pfcp.mac_addresses_detected.mac_address",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_pfcp_mac_addresses_removed_number_of_mac_addresses,
        { "Number of MAC addresses", "pfcp.mac_addresses_removed.number_of_mac_address",
            FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_mac_addresses_removed_mac_address,
        { "MAC Address", "pfcp.mac_addresses_removed.mac_addresses",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_pfcp_ethernet_inactivity_timer,
        { "Ethernet Inactivity Timer", "pfcp.ethernet",
            FT_UINT32, BASE_DEC|BASE_UNIT_STRING, &units_seconds, 0,
            NULL, HFILL }
        },

        { &hf_pfcp_subsequent_event_quota,
        { "Subsequent Event Quota", "pfcp.subsequent_event_quota",
            FT_UINT32, BASE_DEC, NULL, 0,
            NULL, HFILL }
        },

        { &hf_pfcp_subsequent_event_threshold,
        { "Subsequent Event Threshold", "pfcp.subsequent_event_threshold",
            FT_UINT32, BASE_DEC, NULL, 0,
            NULL, HFILL }
        },

        { &hf_pfcp_trace_information_trace_id,
        { "Trace ID", "pfcp.trace_information.traceid",
            FT_UINT24, BASE_DEC, NULL, 0,
            NULL, HFILL }
        },
        { &hf_pfcp_trace_information_length_trigger_events,
        { "Length of Trigger Events", "pfcp.trace_information.length_trigger_events",
            FT_UINT8, BASE_DEC, NULL, 0,
            NULL, HFILL }
        },
        { &hf_pfcp_trace_information_trigger_events,
        { "Trigger Events", "pfcp.trace_information.trigger_events",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_trace_information_session_trace_depth,
        { "Session Trace Depth", "pfcp.trace_information.session_trace_depth",
            FT_UINT8, BASE_DEC, NULL, 0,
            NULL, HFILL }
        },
        { &hf_pfcp_trace_information_length_list_interfaces,
        { "Length of List of Interfaces", "pfcp.trace_information.length_list_interfaces",
            FT_UINT8, BASE_DEC, NULL, 0,
            NULL, HFILL }
        },
        { &hf_pfcp_trace_information_list_interfaces,
        { "List of Interfaces", "pfcp.trace_information.list_interfaces",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_trace_information_length_ipaddress,
        { "Length of IP Address", "pfcp.trace_information.length_ipaddress",
            FT_UINT8, BASE_DEC, NULL, 0,
            NULL, HFILL }
        },
        { &hf_pfcp_trace_information_ipaddress,
        { "IP Address", "pfcp.trace_information.ipaddress",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_pfcp_frame_route,
        { "Frame-Route", "pfcp.frame_route",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_frame_routing,
        { "Frame-Routing", "pfcp.frame_routing",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_pfcp_frame_ipv6_route,
        { "Frame-IPv6-Route", "pfcp.frame_ipv6_route",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },

        { &hf_pfcp_event_quota,
        { "Event Quota", "pfcp.event_quota",
            FT_UINT32, BASE_DEC, NULL, 0,
            NULL, HFILL }
        },

        { &hf_pfcp_event_threshold,
        { "Event Threshold", "pfcp.event_threshold",
            FT_UINT32, BASE_DEC, NULL, 0,
            NULL, HFILL }
        },

        { &hf_pfcp_event_time_stamp,
        { "Event Time Stamp", "pfcp.event_time_stamp",
            FT_STRING, BASE_NONE, NULL, 0,
            NULL, HFILL }
        },

        { &hf_pfcp_averaging_window,
        { "Averaging Window", "pfcp.averaging_window",
            FT_UINT32, BASE_DEC, NULL, 0,
            NULL, HFILL }
        },

        { &hf_pfcp_paging_policy_indicator,
        { "Paging Policy Indicator (PPI)", "pfcp.ppi",
            FT_UINT8, BASE_DEC, NULL, 0x7,
            NULL, HFILL }
        },
    };

    /* Setup protocol subtree array */
#define NUM_INDIVIDUAL_ELEMS_PFCP    49
    gint *ett[NUM_INDIVIDUAL_ELEMS_PFCP +
        (NUM_PFCP_IES - 1)];

    ett[0] = &ett_pfcp;
    ett[1] = &ett_pfcp_flags;
    ett[2] = &ett_pfcp_ie;
    ett[3] = &ett_pfcp_grouped_ie;
    ett[4] = &ett_pfcp_f_seid_flags;
    ett[5] = &ett_f_teid_flags;
    ett[6] = &ett_pfcp_ue_ip_address_flags;
    ett[7] = &ett_pfcp_sdf_filter_flags;
    ett[8] = &ett_pfcp_apply_action_flags;
    ett[9] = &ett_pfcp_measurement_method_flags;
    ett[10] = &ett_pfcp_reporting_triggers;
    ett[11] = &ett_pfcp_volume_threshold;
    ett[12] = &ett_pfcp_volume_quota;
    ett[13] = &ett_pfcp_subseq_volume_threshold;
    ett[14] = &ett_pfcp_dropped_dl_traffic_threshold;
    ett[15] = &ett_pfcp_gate_status;
    ett[16] = &ett_pfcp_report_type;
    ett[17] = &ett_pfcp_up_function_features;
    ett[18] = &ett_pfcp_report_trigger;
    ett[19] = &ett_pfcp_volume_measurement;
    ett[20] = &ett_pfcp_cp_function_features;
    ett[21] = &ett_pfcp_usage_information;
    ett[22] = &ett_pfcp_packet_rate;
    ett[23] = &ett_pfcp_pfcp_dl_flow_level_marking;
    ett[24] = &ett_pfcp_dl_data_service_inf;
    ett[25] = &ett_pfcp_pfcpsmreq;
    ett[26] = &ett_pfcp_pfcpsrrsp;
    ett[27] = &ett_pfcp_measurement_info;
    ett[28] = &ett_pfcp_node_report_type;
    ett[29] = &ett_pfcp_remote_gtp_u_peer;
    ett[30] = &ett_pfcp_oci_flags;
    ett[31] = &ett_pfcp_assoc_rel_req_flags;
    ett[32] = &ett_pfcp_upiri_flags;
    ett[33] = &ett_pfcp_flow_desc;
    ett[34] = &ett_pfcp_tos;
    ett[35] = &ett_pfcp_spi;
    ett[36] = &ett_pfcp_flow_label;
    ett[37] = &ett_pfcp_subsequent_volume_quota;
    ett[38] = &ett_pfcp_additional_usage_reports_information;
    ett[39] = &ett_pfcp_mac_address;
    ett[40] = &ett_pfcp_c_tag;
    ett[41] = &ett_pfcp_c_tag_dei;
    ett[42] = &ett_pfcp_s_tag;
    ett[43] = &ett_pfcp_s_tag_dei;
    ett[44] = &ett_pfcp_proxying;
    ett[45] = &ett_pfcp_ethernet_filter_properties;
    ett[46] = &ett_pfcp_user_id;
    ett[47] = &ett_pfcp_ethernet_pdu_session_information;
    ett[48] = &ett_pfcp_sdf_filter_id;


    static ei_register_info ei[] = {
        { &ei_pfcp_ie_reserved,{ "pfcp.ie_id_reserved", PI_PROTOCOL, PI_ERROR, "Reserved IE value used", EXPFILL } },
        { &ei_pfcp_ie_data_not_decoded,{ "pfcp.ie_data_not_decoded", PI_UNDECODED, PI_NOTE, "IE data not decoded by WS yet", EXPFILL } },
        { &ei_pfcp_ie_not_decoded_null,{ "pfcp.ie_not_decoded_null", PI_UNDECODED, PI_NOTE, "IE not decoded yet(WS:no decoding function(NULL))", EXPFILL } },
        { &ei_pfcp_ie_not_decoded_to_large,{ "pfcp.ie_not_decoded", PI_UNDECODED, PI_NOTE, "IE not decoded yet(WS:IE id to large)", EXPFILL } },
        { &ei_pfcp_enterprise_ie_3gpp,{ "pfcp.ie_enterprise_3gpp", PI_PROTOCOL, PI_ERROR, "IE not decoded yet(WS:No vendor dissector)", EXPFILL } },
        { &ei_pfcp_ie_encoding_error,{ "pfcp.ie_encoding_error", PI_PROTOCOL, PI_ERROR, "IE wrongly encoded", EXPFILL } },
    };

    module_t *module_pfcp;
    expert_module_t* expert_pfcp;

    guint last_index = NUM_INDIVIDUAL_ELEMS_PFCP, i;

    for (i = 0; i < (NUM_PFCP_IES-1); i++, last_index++)
    {
        ett_pfcp_elem[i] = -1;
        ett[last_index] = &ett_pfcp_elem[i];
    }
    proto_pfcp = proto_register_protocol("Packet Forwarding Control Protocol", "PFCP", "pfcp");
    pfcp_handle = register_dissector("pfcp", dissect_pfcp, proto_pfcp);
    module_pfcp = prefs_register_protocol(proto_pfcp, proto_reg_handoff_pfcp);

    proto_register_field_array(proto_pfcp, hf_pfcp, array_length(hf_pfcp));
    proto_register_subtree_array(ett, array_length(ett));
    expert_pfcp = expert_register_protocol(proto_pfcp);
    expert_register_field_array(expert_pfcp, ei, array_length(ei));

    /* Register dissector table for enterprise IE dissectors */
    pfcp_enterprise_ies_dissector_table = register_dissector_table("pfcp.enterprise_ies", "PFCP Enterprice IEs",
        proto_pfcp, FT_UINT32, BASE_DEC);

    pfcp_3gpp_ies_handle = register_dissector("pfcp_3gpp_ies", dissect_pfcp_3gpp_enterprise_ies, proto_pfcp);

    prefs_register_uint_preference(module_pfcp, "port_pfcp", "PFCP port", "PFCP port (default 8805)", 10, &g_pfcp_port);
    prefs_register_bool_preference(module_pfcp, "track_pfcp_session", "Track PFCP session", "Track PFCP session", &g_pfcp_session);
    register_init_routine(pfcp_init);
    register_cleanup_routine(pfcp_cleanup);

}

void
proto_reg_handoff_pfcp(void)
{
    dissector_add_uint("udp.port", g_pfcp_port, pfcp_handle);
    /* Register 3GPP in the table to give expert info and serve as an example how to add decoding of enterprise IEs*/
    dissector_add_uint("pfcp.enterprise_ies", VENDOR_THE3GPP, pfcp_3gpp_ies_handle);


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
