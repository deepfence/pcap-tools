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

/*
 * stream_tcp.c authors:
 *     Martin Roesch <roesch@sourcefire.com>
 *     Steven Sturges <ssturges@sourcefire.com>
 *     Russ Combs <rcombs@sourcefire.com>
 */

/*
 * FIXITs:
 * - midstream ssn pickup (done, SAS 10/14/2005)
 * - syn flood protection (done, SAS 9/27/2005)
 *
 * - review policy anomaly detection
 *   + URG pointer (TODO)
 *   + data on SYN (done, SAS 10/12/2005)
 *   + data on FIN (done, SAS 10/12/2005)
 *   + data after FIN (done, SAS 10/13/2005)
 *   + window scaling/window size max (done, SAS 10/13/2005)
 *   + PAWS, TCP Timestamps (done, SAS 10/12/2005)
 *
 * - session shutdown/Reset handling (done, SAS)
 * - flush policy for Window/Consumed
 * - limit on number of overlapping packets (done, SAS)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tcp_session.h"

#include "detection/detection_engine.h"
#include "detection/rules.h"
#include "log/log.h"
#include "memory/memory_cap.h"
#include "profiler/profiler.h"
#include "protocols/eth.h"

#include "stream_tcp.h"
#include "tcp_debug_trace.h"
#include "tcp_ha.h"
#include "tcp_module.h"
#include "tcp_normalizers.h"
#include "tcp_reassemblers.h"
#include "tcp_segment_node.h"
#include "tcp_state_machine.h"

using namespace snort;

void TcpSession::sinit()
{
    TcpSegmentDescriptor::setup();
    TcpSegmentNode::setup();
}

void TcpSession::sterm()
{
    TcpSegmentDescriptor::clear();
    TcpSegmentNode::clear();
}

TcpSession::TcpSession(Flow* f) : TcpStreamSession(f)
{
    tsm = TcpStateMachine::get_instance();
    splitter_init = false;

    client.session = this;
    server.session = this;
    tcpStats.instantiated++;

    memory::MemoryCap::update_allocations(sizeof(*this));
}

TcpSession::~TcpSession()
{
    clear_session(true, false, false);
    memory::MemoryCap::update_deallocations(sizeof(*this));
}

bool TcpSession::setup(Packet* p)
{
    TcpStreamSession::setup(p);
    splitter_init = false;

    tcp_config = get_tcp_cfg(flow->ssn_server);
    flow->set_default_session_timeout(tcp_config->session_timeout, false);
    set_os_policy();

    SESSION_STATS_ADD(tcpStats)
    tcpStats.setups++;
    return true;
}

// FIXIT-M once TcpReassembler interface is abstract class move this to base class
void TcpSession::restart(Packet* p)
{
    // sanity check since this is called externally
    assert(p);
    assert(p->ptrs.tcph);
    assert(p->flow == flow);

    DetectionEngine::onload(flow);
    TcpStreamTracker* talker;
    TcpStreamTracker* listener;

    if (p->is_from_server())
    {
        talker = &server;
        listener = &client;
    }
    else
    {
        talker = &client;
        listener = &server;
    }

    if (p->dsize > 0)
        listener->reassembler.flush_on_data_policy(p);

    if (p->ptrs.tcph->is_ack())
        talker->reassembler.flush_on_ack_policy(p);

    tcpStats.restarts++;
}

//-------------------------------------------------------------------------
// when client ports are configured, that means c2s and is stored on the
// client side; when the session starts, the server policy is obtained from
// the client side because segments are stored on the receiving side.
//
// this could be improved further by storing the c2s policy on the server
// side and then obtaining server policy from the server on session
// startup.
//
// either way, this client / server distinction must be kept in mind to
// make sense of the code in this file.
//-------------------------------------------------------------------------

void TcpSession::clear_session(bool free_flow_data, bool flush_segments, bool restart, Packet* p)
{
    assert(!p or p->flow == flow);
    if ( !tcp_init )
    {
        if ( lws_init )
            tcpStats.no_pickups++;
        return;
    }

    lws_init = false;
    tcp_init = false;
    tcpStats.released++;

    if ( flush_segments )
    {
        client.reassembler.flush_queued_segments(flow, true, p);
        server.reassembler.flush_queued_segments(flow, true, p);
    }

    if ( p )
    {
        client.finalize_held_packet(p);
        server.finalize_held_packet(p);
    }
    else
    {
        client.finalize_held_packet(flow);
        server.finalize_held_packet(flow);
    }

    client.reassembler.purge_segment_list();
    server.reassembler.purge_segment_list();

    update_perf_base_state(TcpStreamTracker::TCP_CLOSED);

    if ( restart )
    {
        flow->restart(free_flow_data);
        paf_reset(&client.paf_state);
        paf_reset(&server.paf_state);

    }
    else
    {
        flow->clear(free_flow_data);
        paf_clear(&client.paf_state);
        paf_clear(&server.paf_state);
    }

    set_splitter(true, nullptr);
    set_splitter(false, nullptr);

    tel.log_internal_event(SESSION_EVENT_CLEAR);
}

void TcpSession::update_perf_base_state(char newState)
{
    uint32_t session_flags = flow->get_session_flags();
    bool fire_event = false;

    switch ( newState )
    {
    case TcpStreamTracker::TCP_SYN_SENT:
        if ( !( session_flags & SSNFLAG_COUNTED_INITIALIZE ) )
        {
            tcpStats.sessions_initializing++;
            session_flags |= SSNFLAG_COUNTED_INITIALIZE;
        }
        break;

    case TcpStreamTracker::TCP_ESTABLISHED:
        if ( !( session_flags & SSNFLAG_COUNTED_ESTABLISH ) )
        {
            tcpStats.sessions_established++;
            session_flags |= SSNFLAG_COUNTED_ESTABLISH;
            fire_event = true;

            tel.log_internal_event(SESSION_EVENT_SETUP);
            if ( ( session_flags & SSNFLAG_COUNTED_INITIALIZE )
                && !( session_flags & SSNFLAG_COUNTED_CLOSING ) )
            {
                assert(tcpStats.sessions_initializing);
                tcpStats.sessions_initializing--;
            }
        }
        break;

    case TcpStreamTracker::TCP_CLOSING:
        if ( !( session_flags & SSNFLAG_COUNTED_CLOSING ) )
        {
            tcpStats.sessions_closing++;
            session_flags |= SSNFLAG_COUNTED_CLOSING;

            if ( session_flags & SSNFLAG_COUNTED_ESTABLISH )
            {
                assert(tcpStats.sessions_established);
                tcpStats.sessions_established--;
            }
            else if ( session_flags & SSNFLAG_COUNTED_INITIALIZE )
            {
                assert(tcpStats.sessions_initializing);
                tcpStats.sessions_initializing--;
            }
        }
        break;

    case TcpStreamTracker::TCP_CLOSED:
        if ( !( session_flags & SSNFLAG_COUNTED_CLOSED ) )
        {
            session_flags |= SSNFLAG_COUNTED_CLOSED;
            fire_event = true;

            if ( session_flags & SSNFLAG_COUNTED_CLOSING )
            {
                assert(tcpStats.sessions_closing);
                tcpStats.sessions_closing--;
            }
            else if ( session_flags & SSNFLAG_COUNTED_ESTABLISH )
            {
                assert(tcpStats.sessions_established);
                tcpStats.sessions_established--;
            }
            else if ( session_flags & SSNFLAG_COUNTED_INITIALIZE )
            {
                assert(tcpStats.sessions_initializing);
                tcpStats.sessions_initializing--;
            }
        }
        break;

    default:
        break;
    }

    flow->update_session_flags(session_flags);

    if ( fire_event )
        DataBus::publish(FLOW_STATE_EVENT, nullptr, flow);
}

bool TcpSession::flow_exceeds_config_thresholds(const TcpSegmentDescriptor& tsd)
{
    TcpStreamTracker* listener = tsd.get_listener();

    if ( listener->flush_policy == STREAM_FLPOLICY_IGNORE )
        return true;

    // FIXIT-M any discards must be counted and in many cases alerted as well
    // (count all but alert at most once per flow)
    // three cases in this function; look for others
    if ( ( tcp_config->flags & STREAM_CONFIG_NO_ASYNC_REASSEMBLY ) && !flow->two_way_traffic() )
        return true;

    if ( tcp_config->max_consec_small_segs )
    {
        if ( tsd.get_len() >= tcp_config->max_consec_small_seg_size )
            listener->small_seg_count = 0;

        else if ( ++listener->small_seg_count == tcp_config->max_consec_small_segs )
            tel.set_tcp_event(EVENT_MAX_SMALL_SEGS_EXCEEDED);
    }

    if ( tcp_config->max_queued_bytes
        && ( listener->reassembler.get_seg_bytes_total() > tcp_config->max_queued_bytes ) )
    {
        tcpStats.exceeded_max_bytes++;
        // FIXIT-M add one alert per flow per above
        return true;
    }

    if ( tcp_config->max_queued_segs
        && ( listener->reassembler.get_seg_count() + 1 > tcp_config->max_queued_segs ) )
    {
        tcpStats.exceeded_max_segs++;
        // FIXIT-M add one alert per flow per above
        return true;
    }

    return false;
}

void TcpSession::process_tcp_stream(TcpSegmentDescriptor& tsd)
{
    if ( tsd.are_packet_flags_set(PKT_IGNORE) )
        return;

    set_packet_header_foo(tsd);

    if ( flow_exceeds_config_thresholds(tsd) )
        return;

    TcpStreamTracker* listener = tsd.get_listener();

    listener->reassembler.queue_packet_for_reassembly(tsd);

    // Alert if overlap limit exceeded
    if ( (tcp_config->overlap_limit)
        && (listener->reassembler.get_overlap_count() > tcp_config->overlap_limit) )
    {
        tel.set_tcp_event(EVENT_EXCESSIVE_OVERLAP);
        listener->reassembler.set_overlap_count(0);
    }
}

void TcpSession::update_stream_order(const TcpSegmentDescriptor& tsd, bool aligned)
{
    TcpStreamTracker* listener = tsd.get_listener();

    switch ( listener->order )
    {
    case 0:
        if ( aligned )
            tsd.set_packet_flags(PKT_STREAM_ORDER_OK);
        else
            listener->order = 1;
        break;
    case 1:
        if ( aligned )
        {
            tsd.set_packet_flags(PKT_STREAM_ORDER_OK);
            listener->order = 2;
        }
        break;
    default:
        if ( aligned )
            tsd.set_packet_flags(PKT_STREAM_ORDER_OK);

        else
        {
            if ( !(flow->get_session_flags() & SSNFLAG_STREAM_ORDER_BAD) )
                flow->set_session_flags(SSNFLAG_STREAM_ORDER_BAD);
            tsd.set_packet_flags(PKT_STREAM_ORDER_BAD);
         }
    }
}

int TcpSession::process_tcp_data(TcpSegmentDescriptor& tsd)
{
    TcpStreamTracker* listener = tsd.get_listener();
    const tcp::TCPHdr* tcph = tsd.get_tcph();
    uint32_t seq = tsd.get_seq();

    if ( tcph->is_syn() )
    {
        if (listener->normalizer.get_os_policy() == StreamPolicy::OS_MACOS)
            seq++;
        else
        {
            listener->normalizer.trim_syn_payload(tsd);
            return STREAM_UNALIGNED;
        }
    }

    /* we're aligned, so that's nice anyway */
    if (seq == listener->rcv_nxt)
    {
        /* check if we're in the window */
        if (tcp_config->policy != StreamPolicy::OS_PROXY
            and listener->normalizer.get_stream_window(tsd) == 0)
        {
            listener->normalizer.trim_win_payload(tsd);
            return STREAM_UNALIGNED;
        }

        /* move the ack boundary up, this is the only way we'll accept data */
        // FIXIT-L for ips, must move all the way to first hole or right end
        listener->rcv_nxt = tsd.get_end_seq();

        if (tsd.get_len() != 0)
        {
            update_stream_order(tsd, true);
            process_tcp_stream(tsd);
            return STREAM_ALIGNED;
        }
    }
    else
    {
        // pkt is out of order, do some target-based shizzle here...
        // NO, we don't want to simply bail.  Some platforms favor unack'd dup data over the
        // original data.  Let the reassembly policy decide how to handle the overlapping data.
        // See HP, Solaris, et al. for those that favor duplicate data over the original in
        // some cases.

        /* check if we're in the window */
        if (tcp_config->policy != StreamPolicy::OS_PROXY
            and listener->normalizer.get_stream_window(tsd) == 0)
        {
            listener->normalizer.trim_win_payload(tsd);
            return STREAM_UNALIGNED;
        }
        if (tsd.get_len() != 0)
        {
            update_stream_order(tsd, false);
            process_tcp_stream(tsd);
        }
    }

    return STREAM_UNALIGNED;
}

void TcpSession::set_os_policy()
{
    StreamPolicy client_os_policy = flow->ssn_policy ?
        static_cast<StreamPolicy>( flow->ssn_policy ) : tcp_config->policy;

    StreamPolicy server_os_policy = flow->ssn_policy ?
        static_cast<StreamPolicy>( flow->ssn_policy ) : tcp_config->policy;

    client.normalizer.init(client_os_policy, this, &client, &server);
    server.normalizer.init(server_os_policy, this, &server, &client);

    client.reassembler.init(this, &client, client_os_policy, false);
    server.reassembler.init(this, &server, server_os_policy, true);
}

// FIXIT-M this is no longer called (but should be)
#if 0
void TcpSession::swap_trackers()
{
    uint32_t session_flags = flow->get_session_flags( );
    if ( ( session_flags & SSNFLAG_CLIENT_SWAP ) && !( session_flags & SSNFLAG_CLIENT_SWAPPED ) )
    {
        TcpStreamTracker& trk = client;
        client = server;
        server = trk;

        SfIp ip = flow->client_ip;
        flow->client_ip = flow->server_ip;
        flow->server_ip = ip;

        uint16_t port = flow->client_port;
        flow->client_port = flow->server_port;
        flow->server_port = port;

        if ( !flow->two_way_traffic() )
        {
            if ( session_flags & SSNFLAG_SEEN_CLIENT )
            {
                session_flags ^= SSNFLAG_SEEN_CLIENT;
                session_flags |= SSNFLAG_SEEN_SERVER;
            }
            else if ( session_flags & SSNFLAG_SEEN_SERVER )
            {
                session_flags ^= SSNFLAG_SEEN_SERVER;
                session_flags |= SSNFLAG_SEEN_CLIENT;
            }
        }

        session_flags |= SSNFLAG_CLIENT_SWAPPED;
        flow->update_session_flags(session_flags);
    }
}
#endif

void TcpSession::init_session_on_syn(TcpSegmentDescriptor& tsd)
{
    server.init_on_syn_recv(tsd);
    client.init_on_syn_sent(tsd);
    init_new_tcp_session(tsd);
    tcpStats.sessions_on_syn++;
}

void TcpSession::init_session_on_synack(TcpSegmentDescriptor& tsd)
{
    server.init_on_synack_sent(tsd);
    client.init_on_synack_recv(tsd);
    init_new_tcp_session(tsd);
    tcpStats.sessions_on_syn_ack++;
}

void TcpSession::update_timestamp_tracking(TcpSegmentDescriptor& tsd)
{
    TcpStreamTracker* listener = tsd.get_listener();
    TcpStreamTracker* talker = tsd.get_talker();

    talker->set_tf_flags(listener->normalizer.get_timestamp_flags());
    if (listener->normalizer.handling_timestamps()
        && SEQ_EQ(listener->rcv_nxt, tsd.get_seq()))
    {
        talker->set_ts_last_packet(tsd.get_packet_timestamp());
        talker->set_ts_last(tsd.get_timestamp());
    }
}

bool TcpSession::handle_syn_on_reset_session(TcpSegmentDescriptor& tsd)
{
    TcpStreamTracker* listener = tsd.get_listener();
    TcpStreamTracker* talker = tsd.get_talker();
    const tcp::TCPHdr* tcph = tsd.get_tcph();

    if ( (listener->get_tcp_state() == TcpStreamTracker::TCP_CLOSED)
         || (talker->get_tcp_state() == TcpStreamTracker::TCP_CLOSED) )
    {
        // Listener previously issued a reset Talker is re-SYN-ing

        // FIXIT-M this leads to bogus 129:20
        clear_session( true, true, true, tsd.get_pkt() );

        if ( tcph->is_rst() )
        {
            // FIXIT-M  In inline mode, only one of the normalizations can
            // occur.  If the first normalization fires, there is nothing
            // for the second normalization to do.  However, in inline-test
            // mode, since nothing is actually normalized, both of the
            // following functions report that they 'would' normalize.
            // i.e., both functions increment their count even though only
            // one function can ever perform a normalization.

            /* Got SYN/RST.  We're done. */
            listener->normalizer.trim_syn_payload(tsd);
            listener->normalizer.trim_rst_payload(tsd);
            pkt_action_mask |= ACTION_RST;
            return false;
        }
        else if ( tcph->is_syn_only() )
        {
            flow->ssn_state.direction = FROM_CLIENT;
            flow->session_state = STREAM_STATE_SYN;
            flow->set_ttl(tsd.get_pkt(), true);
            init_session_on_syn(tsd);
            tcpStats.resyns++;
            listener->normalizer.ecn_tracker(tcph, tcp_config->require_3whs());
            flow->update_session_flags(SSNFLAG_SEEN_CLIENT);
        }
        else if ( tcph->is_syn_ack() )
        {
            if ( tcp_config->midstream_allowed(tsd.get_pkt()) )
            {
                flow->ssn_state.direction = FROM_SERVER;
                flow->session_state = STREAM_STATE_SYN_ACK;
                flow->set_ttl(tsd.get_pkt(), false);
                init_session_on_synack(tsd);
                tcpStats.resyns++;
            }

            listener->normalizer.ecn_tracker(tcph, tcp_config->require_3whs());
            flow->update_session_flags(SSNFLAG_SEEN_SERVER);
        }
    }

    return true;
}

void TcpSession::update_ignored_session(TcpSegmentDescriptor& tsd)
{
    if ( tsd.is_meta_ack_packet() )
        return;

    Packet* p = tsd.get_pkt();
    // FIXIT-L why flush here instead of just purge?
    // s5_ignored_session() may be disabling detection too soon if we really want to flush
    if ( Stream::ignored_flow(flow, p) )
    {
        TcpStreamTracker* listener = tsd.get_listener();
        TcpStreamTracker* talker = tsd.get_talker();

        if ( talker && ( talker->get_tf_flags() & TF_FORCE_FLUSH ) )
        {
            flush_talker(p);
            talker->clear_tf_flags(TF_FORCE_FLUSH);
        }

        if ( listener && ( listener->get_tf_flags() & TF_FORCE_FLUSH ) )
        {
            flush_listener(p);
            listener->clear_tf_flags(TF_FORCE_FLUSH);
        }

        tsd.set_packet_flags(PKT_IGNORE);
        pkt_action_mask |= ACTION_DISABLE_INSPECTION;
        tcpStats.ignored++;
    }
}

void TcpSession::handle_data_on_syn(TcpSegmentDescriptor& tsd)
{
    TcpStreamTracker* listener = tsd.get_listener();
    TcpStreamTracker* talker = tsd.get_talker();

    /* MacOS accepts data on SYN, so don't alert if policy is MACOS */
    if ( talker->normalizer.get_os_policy() == StreamPolicy::OS_MACOS )
        handle_data_segment(tsd);
    else
    {
        listener->normalizer.trim_syn_payload(tsd);
        tel.set_tcp_event(EVENT_DATA_ON_SYN);
        pkt_action_mask |= ACTION_BAD_PKT;
    }
}

void TcpSession::update_session_on_rst(TcpSegmentDescriptor& tsd, bool flush)
{
    Packet* p = tsd.get_pkt();

    if ( flush )
    {
        flush_listener(p, true);
        flush_talker(p, true);
        set_splitter(true, nullptr);
        set_splitter(false, nullptr);
    }

    tsd.get_talker()->update_on_rst_sent();
}

void TcpSession::update_paws_timestamps(TcpSegmentDescriptor& tsd)
{
    TcpStreamTracker* listener = tsd.get_listener();
    TcpStreamTracker* talker = tsd.get_talker();

    if ( listener->normalizer.handling_timestamps()
        && SEQ_EQ(listener->r_win_base, tsd.get_seq()) )
    {
        if ( ((int32_t)(tsd.get_timestamp() - talker->get_ts_last()) >= 0  )
            || (tsd.get_packet_timestamp() >= talker->get_ts_last_packet() + PAWS_24DAYS) )
        {
            talker->set_ts_last(tsd.get_timestamp());
            talker->set_ts_last_packet(tsd.get_packet_timestamp());
        }
    }
}

void TcpSession::check_for_session_hijack(TcpSegmentDescriptor& tsd)
{
    TcpStreamTracker* listener = tsd.get_listener();
    TcpStreamTracker* talker = tsd.get_talker();

    Packet* p = tsd.get_pkt();
    if ( !(p->pkth->flags & DAQ_PKT_FLAG_PRE_ROUTING) )
    {
        if ( p->is_eth() )
        {
            const eth::EtherHdr* eh = layer::get_eth_layer(p);
            bool t_hijack = !talker->compare_mac_addresses(eh->ether_src);
            bool l_hijack = !listener->compare_mac_addresses(eh->ether_dst);

            // if both seem hijacked then swap src/dst check, it that matches probably a tap
            if ( (t_hijack & l_hijack) && (talker->compare_mac_addresses(eh->ether_dst) &&
                 listener->compare_mac_addresses(eh->ether_src)) )
                return;

            uint32_t event_code = 0;

            if ( t_hijack )
            {
                if ( p->is_from_client() )
                    event_code |= EVENT_SESSION_HIJACK_CLIENT;
                else
                    event_code |= EVENT_SESSION_HIJACK_SERVER;
            }

            if ( l_hijack )
            {
                if ( p->is_from_client() )
                    event_code |= EVENT_SESSION_HIJACK_SERVER;
                else
                    event_code |= EVENT_SESSION_HIJACK_CLIENT;
            }

            if ( event_code )
                tel.set_tcp_event(event_code);
        }
    }
}

bool TcpSession::check_for_window_slam(TcpSegmentDescriptor& tsd)
{
    TcpStreamTracker* listener = tsd.get_listener();

    if ( tcp_config->max_window && (tsd.get_wnd() > tcp_config->max_window) )
    {
        /* got a window too large, alert! */
        tel.set_tcp_event(EVENT_WINDOW_TOO_LARGE);
        inc_tcp_discards();
        listener->normalizer.packet_dropper(tsd, NORM_TCP_BLOCK);
        pkt_action_mask |= ACTION_BAD_PKT;
        return true;
    }
    else if ( tsd.is_packet_from_client() && (tsd.get_wnd() <= SLAM_MAX)
        && (tsd.get_ack() == listener->get_iss() + 1)
        && !(tsd.get_tcph()->is_fin() | tsd.get_tcph()->is_rst())
        && !(flow->get_session_flags() & SSNFLAG_MIDSTREAM))
    {
        /* got a window slam alert! */
        tel.set_tcp_event(EVENT_WINDOW_SLAM);
        inc_tcp_discards();

        if ( listener->normalizer.packet_dropper(tsd, NORM_TCP_BLOCK) )
        {
            pkt_action_mask |= ACTION_BAD_PKT;
            return true;
        }
    }

    return false;
}

void TcpSession::mark_packet_for_drop(TcpSegmentDescriptor& tsd)
{

    tsd.get_listener()->normalizer.packet_dropper(tsd, NORM_TCP_BLOCK);
    set_pkt_action_flag(ACTION_BAD_PKT);
}

void TcpSession::handle_data_segment(TcpSegmentDescriptor& tsd)
{
    TcpStreamTracker* listener = tsd.get_listener();
    TcpStreamTracker* talker = tsd.get_talker();

    if ( TcpStreamTracker::TCP_CLOSED != talker->get_tcp_state() )
    {
        uint8_t tcp_options_len = tsd.get_tcph()->options_len();
        if ( tsd.is_packet_from_client() )
            client.set_tcp_options_len(tcp_options_len);
        else
            server.set_tcp_options_len(tcp_options_len);

        // FIXIT-M move this to normalizer base class, handle OS_PROXY in derived class
        if ( tcp_config->policy != StreamPolicy::OS_PROXY )
        {
            /* check for valid sequence/retrans */
            if ( !listener->is_segment_seq_valid(tsd) )
                return;

            // these normalizations can't be done if we missed setup. and
            // window is zero in one direction until we've seen both sides.
            if ( !(flow->get_session_flags() & SSNFLAG_MIDSTREAM) && flow->two_way_traffic() )
            {
                // sender of syn w/mss limits payloads from peer since we store mss on
                // sender side, use listener mss same reasoning for window size
                TcpStreamTracker* st = listener;

                // trim to fit in window and mss as needed
                st->normalizer.trim_win_payload(
                    tsd, (st->r_win_base + st->get_snd_wnd() - st->rcv_nxt));

                // FIXIT-H: MSS is not set on client so packets sent to client are not trimmed
                // use case?
                if ( st->get_mss() )
                    st->normalizer.trim_mss_payload(tsd, st->get_mss());

                st->normalizer.ecn_stripper(tsd);
            }
        }

        // dunno if this is RFC but fragroute testing expects it for the record,
        // I've seen FTP data sessions that send data packets with no tcp flags set
        if ( (tsd.get_tcph()->th_flags != 0) or (tcp_config->policy == StreamPolicy::OS_LINUX)
                or (tcp_config->policy == StreamPolicy::OS_PROXY) )
        {
            process_tcp_data(tsd);
        }
        else
        {
            tel.set_tcp_event(EVENT_DATA_WITHOUT_FLAGS);
            listener->normalizer.packet_dropper(tsd, NORM_TCP_BLOCK);
        }
    }

    listener->reassembler.flush_on_data_policy(tsd.get_pkt());
}

TcpStreamTracker::TcpState TcpSession::get_talker_state(TcpSegmentDescriptor& tsd)
{
    return tsd.get_talker()->get_tcp_state();
}

TcpStreamTracker::TcpState TcpSession::get_listener_state(TcpSegmentDescriptor& tsd)
{
    return tsd.get_listener()->get_tcp_state();
}

void TcpSession::check_for_repeated_syn(TcpSegmentDescriptor& tsd)
{
    TcpStreamTracker* listener = tsd.get_listener();
    TcpStreamTracker* talker = tsd.get_talker();
    uint32_t action = ACTION_NOTHING;

    if ( !SEQ_EQ(tsd.get_seq(), talker->get_iss()) and
        listener->normalizer.packet_dropper(tsd, NORM_TCP_BLOCK) )
    {
        action = ACTION_BAD_PKT;
    }
    else if ( talker->get_tcp_state() >= TcpStreamTracker::TCP_ESTABLISHED and
        talker->get_tcp_state() < TcpStreamTracker::TCP_CLOSED )
    {
        action = listener->normalizer.handle_repeated_syn(tsd);
    }

    if (action != ACTION_NOTHING)
    {
        tel.set_tcp_event(EVENT_SYN_ON_EST);
        pkt_action_mask |= action;
    }
}

void TcpSession::flush_server(Packet* p)
{
    if ( !tcp_init )
        return;

    server.set_tf_flags(TF_FORCE_FLUSH);

    // don't flush if rebuilt packet, that would overwrite the packet being processed.
    if ( p->packet_flags & PKT_REBUILT_STREAM )
        return; // We'll check & clear the TF_FORCE_FLUSH next time through

    // Need to convert the addresses to network order
    if ( server.reassembler.flush_stream(p, PKT_FROM_SERVER) )
        server.reassembler.purge_flushed_ackd();

    server.clear_tf_flags(TF_FORCE_FLUSH);
}

void TcpSession::flush_client(Packet* p)
{
    if ( !tcp_init )
        return;

    client.set_tf_flags(TF_FORCE_FLUSH);

    // If rebuilt packet, don't flush now because we'll overwrite the packet being processed.
    if ( p->packet_flags & PKT_REBUILT_STREAM )
        return;         // TF_FORCE_FLUSH checked & cleared next time through

    if ( client.reassembler.flush_stream(p, PKT_FROM_CLIENT) )
        client.reassembler.purge_flushed_ackd();

    client.clear_tf_flags(TF_FORCE_FLUSH);
}

void TcpSession::flush_tracker(
    TcpStreamTracker& tracker, Packet* p, uint32_t dir, bool final_flush)
{
    if ( final_flush && (!tracker.splitter || !tracker.splitter->finish(flow)) )
         return;

     tracker.set_tf_flags(TF_FORCE_FLUSH);
     if ( tracker.reassembler.flush_stream(p, dir, final_flush) )
         tracker.reassembler.purge_flushed_ackd();

     tracker.clear_tf_flags(TF_FORCE_FLUSH);
}

void TcpSession::flush_listener(Packet* p, bool final_flush)
{
    // direction of flush is the data from the opposite side
    if ( p->is_from_server() )
        flush_tracker( client, p, PKT_FROM_SERVER, final_flush);
    else if ( p->is_from_client() )
        flush_tracker( server, p, PKT_FROM_CLIENT, final_flush);
}

void TcpSession::flush_talker(Packet* p, bool final_flush)
{
    // direction of flush is the data from the opposite side
    if ( p->is_from_server() )
        flush_tracker( server, p, PKT_FROM_CLIENT, final_flush);
    else if ( p->is_from_client() )
        flush_tracker( client, p, PKT_FROM_SERVER, final_flush);
}

void TcpSession::set_extra_data(Packet* p, uint32_t xid)
{
    TcpStreamTracker& st = p->ptrs.ip_api.get_src()->equals(flow->client_ip) ? server : client;
    st.reassembler.set_xtradata_mask(st.reassembler.get_xtradata_mask() | BIT(xid));
}

static inline void set_window_scale(TcpSegmentDescriptor& tsd)
{
    // scale the window.  Only if BOTH client and server specified wscale option as part
    // of 3-way handshake.  This is per RFC 1323.
    if ( (tsd.get_talker()->get_tf_flags() & TF_WSCALE)
         && (tsd.get_listener()->get_tf_flags() & TF_WSCALE) )
    {
        tsd.scale_wnd(tsd.get_talker()->get_wscale());
    }
}

void TcpSession::check_events_and_actions(const TcpSegmentDescriptor& tsd)
{
    if ( tsd.is_meta_ack_packet() )
        return;

    tel.log_tcp_events();

    Packet* p = tsd.get_pkt();
    if ( !(pkt_action_mask & ACTION_LWSSN_CLOSED) )
    {
        flow->markup_packet_flags(p);
        flow->set_expire(p, flow->default_session_timeout);
    }
    else
        TcpHAManager::process_deletion(*flow);

    if ( pkt_action_mask & ACTION_DISABLE_INSPECTION )
        DetectionEngine::disable_all(p);
}

bool TcpSession::ignore_this_packet(Packet* p)
{
    if ( no_ack_mode_enabled() and p->is_retry() )
    {
        // Don't need to process a retry packet through stream again,
        // just make sure the retransmit handler is called so that
        // we do things like update file inspection.
        flow->call_handlers(p, false);
        return true;
    }

    if ( Stream::blocked_flow(p) )
        return true;

    // FIXIT-L expected flow should be checked by Stream before we get here
    // harmonize this with that and the checks above
    if ( Stream::expected_flow(flow, p) )
    {
        server.flush_policy = STREAM_FLPOLICY_IGNORE;
        client.flush_policy = STREAM_FLPOLICY_IGNORE;
        return true;
    }

    return false;
}

void TcpSession::cleanup_session_if_expired(Packet* p)
{
    // Check if the session is expired. Should be done before we do something with
    // the packet...Insert a packet, or handle state change SYN, FIN, RST, etc.
    if ( Stream::expired_flow(flow, p) )
    {
        /* Session is timed out, if also reset then restart, otherwise clear */
        if ( flow->get_session_flags() & SSNFLAG_RESET )
            clear_session(true, true, true, p);
        else
            clear_session(true, true, false, p);

        tcpStats.timeouts++;
        TcpHAManager::process_deletion(*flow);
    }
}

void TcpSession::precheck(Packet* p)
{
    // Check if the session is expired. Should be done before we do something with
    // the packet...Insert a packet, or handle state change SYN, FIN, RST, etc.
    cleanup_session_if_expired(p);
}

void TcpSession::init_tcp_packet_analysis(TcpSegmentDescriptor& tsd)
{
    if ( !splitter_init and tsd.get_len() > 0 )
    {
        if ( !(tcp_config->flags & STREAM_CONFIG_NO_REASSEMBLY) )
        {
            client.set_splitter(tsd.get_flow());
            server.set_splitter(tsd.get_flow());

            client.init_flush_policy();
            server.init_flush_policy();

            set_no_ack(tcp_config->no_ack);
        }

        splitter_init = true;
    }

    pkt_action_mask = ACTION_NOTHING;
    tel.clear_tcp_events();
    // process thru state machine...talker first
    // When in no-ack mode, don't trust ACK numbers. Set the ACK value
    // as if the last packet in the other direction was ACK'd.
    // FIXIT-M: The snd_nxt and snd_una checks are only needed because
    // the snd_nxt value isn't valid for SYN/ACK packet. Can remove those
    // checks if that is fixed.
    if ( tsd.is_packet_from_client() )
    {
        update_session_on_client_packet(tsd);

        if ( no_ack_mode_enabled() and (server.get_snd_nxt() or server.get_snd_una()) )
            tsd.set_ack(server.get_snd_nxt());
    }
    else
    {
        update_session_on_server_packet(tsd);

        if ( no_ack_mode_enabled() and (client.get_snd_nxt() or client.get_snd_una()) )
            tsd.set_ack(client.get_snd_nxt());
    }

    update_ignored_session(tsd);
    set_window_scale(tsd);
}

bool TcpSession::validate_packet_established_session(TcpSegmentDescriptor& tsd)
{
    TcpStreamTracker* listener = tsd.get_listener();

    pkt_action_mask |= listener->normalizer.handle_paws(tsd);

    if ( tsd.is_policy_inline() )
       if ( tsd.get_tcph()->is_ack() && !listener->is_ack_valid(tsd.get_ack()) )
           pkt_action_mask |= ACTION_BAD_PKT;

    return ( pkt_action_mask & ACTION_BAD_PKT ) ? false : true;
}

int TcpSession::process_tcp_packet(TcpSegmentDescriptor& tsd)
{
    if ( tsm->eval(tsd) )
    {
        check_events_and_actions(tsd);
        S5TraceTCP(tsd);
    }
    else
    {
        if ( pkt_action_mask & ACTION_BAD_PKT )
        {
            inc_tcp_discards();
            check_events_and_actions(tsd);
        }

        tel.log_tcp_events();
        S5TraceTCP(tsd);
    }

    return ACTION_NOTHING;
}

int TcpSession::process(Packet* p)
{
    Profile profile(s5TcpPerfStats);
    assert(flow->ssn_server && flow->pkt_type == PktType::TCP);

    if ( ignore_this_packet(p) )
        return ACTION_NOTHING;

    TcpSegmentDescriptor tsd(flow, p, tel);
    init_tcp_packet_analysis(tsd);

    // check for and process meta-ack info first if present, the current listener is the
    // talker for the meta-ack...
    DAQ_PktTcpAckData_t* tcp_mack = (DAQ_PktTcpAckData_t*)p->daq_msg->meta[DAQ_PKT_META_TCP_ACK_DATA];
    if ( tcp_mack )
    {
        TcpSegmentDescriptor ma_tsd(flow, p, tcp_mack->tcp_ack_seq_num, tcp_mack->tcp_window_size);
        init_tcp_packet_analysis(ma_tsd);
        process_tcp_packet(ma_tsd);
        tcpStats.meta_acks++;
    }

    if ( p->context->conf->is_address_anomaly_check_enabled() )
        check_for_session_hijack(tsd);

    if ( ( flow->get_session_flags() & SSNFLAG_RESET ) && tsd.get_tcph()->is_syn()
            && !handle_syn_on_reset_session(tsd) )
        return ACTION_NOTHING;

    return process_tcp_packet(tsd);
}

void TcpSession::flush()
{
    if ( ( server.reassembler.is_segment_pending_flush() ) ||
        (client.reassembler.is_segment_pending_flush() ) )
    {
       // FIXIT-L flush_queued_segments is basically a noop when the 'clear' parameter
       //         is passed in as false... review what happens in 2.9.x probably related
       //         to ssl encryption support
        server.reassembler.flush_queued_segments(flow, false);
        client.reassembler.flush_queued_segments(flow, false);
    }
}

