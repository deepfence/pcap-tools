//--------------------------------------------------------------------------
// Copyright (C) 2015-2020 Cisco and/or its affiliates. All rights reserved.
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

// tcp_stream_tracker.cpp author davis mcpherson <davmcphe@cisco.com>
// Created on: Jun 24, 2015

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tcp_stream_tracker.h"

#include "log/messages.h"
#include "main/analyzer.h"
#include "main/snort.h"
#include "packet_io/active.h"
#include "profiler/profiler_defs.h"
#include "protocols/eth.h"

#include "held_packet_queue.h"
#include "segment_overlap_editor.h"
#include "tcp_module.h"
#include "tcp_normalizers.h"
#include "tcp_reassemblers.h"
#include "tcp_session.h"

using namespace snort;

THREAD_LOCAL HeldPacketQueue* hpq = nullptr;

const std::list<HeldPacket>::iterator TcpStreamTracker::null_iterator { };

const char* tcp_state_names[] =
{
    "TCP_LISTEN", "TCP_SYN_SENT", "TCP_SYN_RECV",
    "TCP_ESTABLISHED",
    "TCP_FIN_WAIT1", "TCP_FIN_WAIT2", "TCP_CLOSE_WAIT", "TCP_CLOSING",
    "TCP_LAST_ACK", "TCP_TIME_WAIT", "TCP_CLOSED",
    "TCP_STATE_NONE"
};

const char* tcp_event_names[] = {
    "TCP_SYN_SENT_EVENT", "TCP_SYN_RECV_EVENT",
    "TCP_SYN_ACK_SENT_EVENT", "TCP_SYN_ACK_RECV_EVENT",
    "TCP_ACK_SENT_EVENT", "TCP_ACK_RECV_EVENT",
    "TCP_DATA_SEG_SENT_EVENT", "TCP_DATA_SEG_RECV_EVENT",
    "TCP_FIN_SENT_EVENT", "TCP_FIN_RECV_EVENT",
    "TCP_RST_SENT_EVENT", "TCP_RST_RECV_EVENT"
};

TcpStreamTracker::TcpStreamTracker(bool client) :
    tcp_state(client ? TCP_STATE_NONE : TCP_LISTEN), client_tracker(client),
    held_packet(null_iterator)
{ }

TcpStreamTracker::~TcpStreamTracker()
{ delete splitter; }

TcpStreamTracker::TcpEvent TcpStreamTracker::set_tcp_event(const TcpSegmentDescriptor& tsd)
{
    bool talker;
    const tcp::TCPHdr* tcph = tsd.get_tcph();

    if ( tsd.is_packet_from_client() )
        talker = ( client_tracker ) ? true : false;
    else
        talker = ( client_tracker ) ? false : true;

    // FIXIT-P would a lookup table help perf?  the code would be a little cleaner too.
    if ( talker )
    {
        // talker events
        if ( tcph->is_syn_only() )
            tcp_event = TCP_SYN_SENT_EVENT;
        else if ( tcph->is_syn_ack() )
            tcp_event = TCP_SYN_ACK_SENT_EVENT;
        else if ( tcph->is_rst() )
            tcp_event = TCP_RST_SENT_EVENT;
        else if ( tcph->is_fin( ) )
            tcp_event = TCP_FIN_SENT_EVENT;
        else if ( tcph->is_ack() || tcph->is_psh() )
        {
            if ( tsd.get_len() > 0 )
                tcp_event = TCP_DATA_SEG_SENT_EVENT;
            else
                tcp_event = TCP_ACK_SENT_EVENT;
        }
        else if ( tsd.get_len() > 0 )   // FIXIT-H no flags set, how do we handle this?
                                            // discard; drop if normalizing
            tcp_event = TCP_DATA_SEG_SENT_EVENT;
        else
            tcp_event = TCP_ACK_SENT_EVENT;
    }
    else
    {
        // listener events
        if ( tcph->is_syn_only() )
        {
            tcp_event = TCP_SYN_RECV_EVENT;
            tcpStats.syns++;
            if ( tcp_state == TcpStreamTracker::TCP_LISTEN )
                DataBus::publish(STREAM_TCP_SYN_EVENT, tsd.get_pkt());
        }
        else if ( tcph->is_syn_ack() )
        {
            tcp_event = TCP_SYN_ACK_RECV_EVENT;
            tcpStats.syn_acks++;
            if ( tcp_state == TcpStreamTracker::TCP_SYN_SENT or
                (!Stream::is_midstream(tsd.get_flow()) and
                (tcp_state == TcpStreamTracker::TCP_LISTEN or
                tcp_state == TcpStreamTracker::TCP_STATE_NONE)) )
                DataBus::publish(STREAM_TCP_SYN_ACK_EVENT, tsd.get_pkt());
        }
        else if ( tcph->is_rst() )
        {
            tcp_event = TCP_RST_RECV_EVENT;
            tcpStats.resets++;
        }
        else if ( tcph->is_fin( ) )
        {
            tcp_event = TCP_FIN_RECV_EVENT;
            tcpStats.fins++;
        }
        else if ( tcph->is_ack() || tcph->is_psh() )
        {
            if ( tsd.get_len() > 0 )
                tcp_event = TCP_DATA_SEG_RECV_EVENT;
            else
                tcp_event = TCP_ACK_RECV_EVENT;
        }
        else if ( tsd.get_len() > 0 )    // FIXIT-H no flags set, how do we handle this?
                                             // discard; drop if normalizing
            tcp_event = TCP_DATA_SEG_RECV_EVENT;
        else
            tcp_event = TCP_ACK_RECV_EVENT;
    }

    return tcp_event;
}

// Use a for loop and byte comparison, which has proven to be faster on pipelined architectures
// compared to a memcmp (setup for memcmp is slow).  Not using a 4 byte and 2 byte long because
// there is no guarantee of memory alignment (and thus performance issues similar to memcmp).
bool TcpStreamTracker::compare_mac_addresses(const uint8_t eth_addr[])
{
    if ( !mac_addr_valid )
        return true;

    for ( int i = 0; i < 6; ++i )
        if ( mac_addr[i] != eth_addr[i] )
            return false;

    return true;
}

void TcpStreamTracker::cache_mac_address(const TcpSegmentDescriptor& tsd, uint8_t direction)
{
    /* Not Ethernet based, nothing to do */
    if ( tsd.get_pkt()->is_eth() )
    {
        // if flag is set, guaranteed to have an eth layer
        const eth::EtherHdr* eh = layer::get_eth_layer(tsd.get_pkt() );

        if ( direction == FROM_CLIENT )
        {
            if ( client_tracker )
                for ( int i = 0; i < 6; i++ )
                    mac_addr[i] = eh->ether_src[i];
            else
                for ( int i = 0; i < 6; i++ )
                    mac_addr[i] = eh->ether_dst[i];
        }
        else
        {
            if ( client_tracker )
                for ( int i = 0; i < 6; i++ )
                    mac_addr[i] = eh->ether_dst[i];
            else
                for ( int i = 0; i < 6; i++ )
                    mac_addr[i] = eh->ether_src[i];
        }

        mac_addr_valid = true;
    }
}

void TcpStreamTracker::init_tcp_state()
{
    tcp_state = ( client_tracker ) ?
        TcpStreamTracker::TCP_STATE_NONE : TcpStreamTracker::TCP_LISTEN;
    flush_policy = STREAM_FLPOLICY_IGNORE;
    paf_setup(&paf_state);
    snd_una = snd_nxt = snd_wnd = 0;
    rcv_nxt = r_win_base = iss = ts_last = ts_last_packet = 0;
    small_seg_count = wscale = mss = 0;
    tf_flags = 0;
    alert_count = 0;
    mac_addr_valid = false;
    fin_final_seq = 0;
    fin_seq_status = TcpStreamTracker::FIN_NOT_SEEN;
    fin_seq_set = false;
    rst_pkt_sent = false;
    order = 0;
    held_packet = null_iterator;
}

//-------------------------------------------------------------------------
// flush policy stuff
//-------------------------------------------------------------------------

void TcpStreamTracker::init_flush_policy()
{
    if ( splitter == nullptr )
        flush_policy = STREAM_FLPOLICY_IGNORE;
    else if ( normalizer.is_tcp_ips_enabled() )
        flush_policy = STREAM_FLPOLICY_ON_DATA;
    else
        flush_policy = STREAM_FLPOLICY_ON_ACK;
}

void TcpStreamTracker::set_splitter(StreamSplitter* ss)
{
    if ( splitter )
        delete splitter;

    splitter = ss;

    if ( !splitter )
        flush_policy = STREAM_FLPOLICY_IGNORE;
    else
    {
        paf_setup(&paf_state);
        reassembler.reset_paf_segment();
    }
}

void TcpStreamTracker::set_splitter(const Flow* flow)
{
    Inspector* ins = flow->gadget;

    if ( !ins )
        ins = flow->clouseau;

    if ( ins )
        set_splitter(ins->get_splitter(!client_tracker) );
    else
        set_splitter(new AtomSplitter(!client_tracker) );
}

void TcpStreamTracker::init_on_syn_sent(TcpSegmentDescriptor& tsd)
{
    tsd.get_flow()->set_session_flags(SSNFLAG_SEEN_CLIENT);
    if ( tsd.get_tcph()->are_flags_set(TH_CWR | TH_ECE) )
        tsd.get_flow()->set_session_flags(SSNFLAG_ECN_CLIENT_QUERY);

    iss = tsd.get_seq();
    snd_una = iss;
    snd_nxt = tsd.get_end_seq();
    snd_wnd =  tsd.get_wnd();

    ts_last_packet = tsd.get_packet_timestamp();
    tf_flags |= normalizer.get_tcp_timestamp(tsd, false);
    ts_last = tsd.get_timestamp();
    if (ts_last == 0)
        tf_flags |= TF_TSTAMP_ZERO;
    tf_flags |= tsd.init_mss(&mss);
    tf_flags |= tsd.init_wscale(&wscale);

    cache_mac_address(tsd, FROM_CLIENT);
    tcp_state = TcpStreamTracker::TCP_SYN_SENT;
    tcpStats.sessions_on_syn++;
}

void TcpStreamTracker::init_on_syn_recv(TcpSegmentDescriptor& tsd)
{
    irs = tsd.get_seq();

    rcv_nxt = tsd.get_seq() + 1;
    r_win_base = tsd.get_seq() + 1;
    reassembler.set_seglist_base_seq(tsd.get_seq() + 1);

    cache_mac_address(tsd, FROM_CLIENT);
    tcp_state = TcpStreamTracker::TCP_SYN_RECV;
}

void TcpStreamTracker::init_on_synack_sent(TcpSegmentDescriptor& tsd)
{
    tsd.get_flow()->set_session_flags(SSNFLAG_SEEN_SERVER);
    if (tsd.get_tcph()->are_flags_set(TH_CWR | TH_ECE))
        tsd.get_flow()->set_session_flags(SSNFLAG_ECN_SERVER_REPLY);

    iss = tsd.get_seq();
    irs = tsd.get_ack() - 1;
    snd_una = tsd.get_seq();
    snd_nxt = tsd.get_end_seq();
    snd_wnd = tsd.get_wnd();

    r_win_base = tsd.get_ack();
    rcv_nxt = tsd.get_ack();
    reassembler.set_seglist_base_seq(tsd.get_ack() );

    ts_last_packet = tsd.get_packet_timestamp();
    tf_flags |= normalizer.get_tcp_timestamp(tsd, false);
    ts_last = tsd.get_timestamp();
    if ( ts_last == 0 )
        tf_flags |= TF_TSTAMP_ZERO;
    tf_flags |= tsd.init_mss(&mss);
    tf_flags |= tsd.init_wscale(&wscale);

    cache_mac_address(tsd, FROM_SERVER);
    tcp_state = TcpStreamTracker::TCP_SYN_RECV;
    tcpStats.sessions_on_syn_ack++;
}

void TcpStreamTracker::init_on_synack_recv(TcpSegmentDescriptor& tsd)
{
    iss = tsd.get_ack() - 1;
    irs = tsd.get_seq();
    snd_una = tsd.get_ack();
    snd_nxt = snd_una;

    rcv_nxt = tsd.get_seq() + 1;
    r_win_base = tsd.get_seq() + 1;
    reassembler.set_seglist_base_seq(tsd.get_seq() + 1);

    cache_mac_address(tsd, FROM_SERVER);
    tcp_state = TcpStreamTracker::TCP_ESTABLISHED;
}

void TcpStreamTracker::init_on_3whs_ack_sent(TcpSegmentDescriptor& tsd)
{
    tsd.get_flow()->set_session_flags(SSNFLAG_SEEN_CLIENT);

    if ( tsd.get_tcph()->are_flags_set(TH_CWR | TH_ECE) )
        tsd.get_flow()->set_session_flags(SSNFLAG_ECN_CLIENT_QUERY);

    iss = tsd.get_seq();
    snd_una = tsd.get_seq();
    snd_nxt = snd_una;
    snd_wnd = tsd.get_wnd();

    r_win_base = tsd.get_ack();
    rcv_nxt = tsd.get_ack();

    ts_last_packet = tsd.get_packet_timestamp();
    tf_flags |= normalizer.get_tcp_timestamp(tsd, false);
    ts_last = tsd.get_timestamp();
    if (ts_last == 0)
        tf_flags |= TF_TSTAMP_ZERO;
    tf_flags |= tsd.init_mss(&mss);
    tf_flags |= tsd.init_wscale(&wscale);

    cache_mac_address(tsd, FROM_CLIENT);
    tcp_state = TcpStreamTracker::TCP_ESTABLISHED;
}

void TcpStreamTracker::init_on_3whs_ack_recv(TcpSegmentDescriptor& tsd)
{
    iss = tsd.get_ack() - 1;
    irs = tsd.get_seq();
    snd_una = tsd.get_ack();
    snd_nxt = snd_una;

    rcv_nxt = tsd.get_seq();
    r_win_base = tsd.get_seq();
    reassembler.set_seglist_base_seq(tsd.get_seq() + 1);

    cache_mac_address(tsd, FROM_CLIENT);
    tcp_state = TcpStreamTracker::TCP_ESTABLISHED;
    tcpStats.sessions_on_3way++;
}

void TcpStreamTracker::init_on_data_seg_sent(TcpSegmentDescriptor& tsd)
{
    Flow* flow = tsd.get_flow();

    if ( flow->ssn_state.direction == FROM_CLIENT )
        flow->set_session_flags(SSNFLAG_SEEN_CLIENT);
    else
        flow->set_session_flags(SSNFLAG_SEEN_SERVER);

    iss = tsd.get_seq();
    irs = tsd.get_ack();
    snd_una = tsd.get_seq();
    snd_nxt = snd_una + tsd.get_len();
    snd_wnd = tsd.get_wnd();

    r_win_base = tsd.get_ack();
    rcv_nxt = tsd.get_ack();
    reassembler.set_seglist_base_seq(tsd.get_ack());

    ts_last_packet = tsd.get_packet_timestamp();
    tf_flags |= normalizer.get_tcp_timestamp(tsd, false);
    ts_last = tsd.get_timestamp();
    if (ts_last == 0)
        tf_flags |= TF_TSTAMP_ZERO;
    tf_flags |= ( tsd.init_mss(&mss) | tsd.init_wscale(&wscale) );

    cache_mac_address(tsd, tsd.get_direction() );
    tcp_state = TcpStreamTracker::TCP_ESTABLISHED;
}

void TcpStreamTracker::init_on_data_seg_recv(TcpSegmentDescriptor& tsd)
{
    iss = tsd.get_ack();
    irs = tsd.get_seq();
    snd_una = tsd.get_ack();
    snd_nxt = snd_una;
    snd_wnd = 0; /* reset later */

    rcv_nxt = tsd.get_seq();
    r_win_base = tsd.get_seq();
    reassembler.set_seglist_base_seq(tsd.get_seq());

    cache_mac_address(tsd, tsd.get_direction() );
    tcp_state = TcpStreamTracker::TCP_ESTABLISHED;
    tcpStats.sessions_on_data++;
}

void TcpStreamTracker::finish_server_init(TcpSegmentDescriptor& tsd)
{
    iss = tsd.get_seq();
    snd_una = tsd.get_seq();
    snd_nxt = tsd.get_end_seq();
    snd_wnd = tsd.get_wnd();

    // FIXIT-M move this to fin handler for syn_recv state ..
    //if ( tcph->is_fin() )
    //    server->set_snd_nxt(server->get_snd_nxt() - 1);

    tf_flags |= normalizer.get_tcp_timestamp(tsd, false);
    ts_last = tsd.get_timestamp();
    if ( ts_last != 0 )
        ts_last_packet = tsd.get_packet_timestamp();
    else
        tf_flags |= TF_TSTAMP_ZERO;

    tf_flags |= ( tsd.init_mss(&mss) | tsd.init_wscale(&wscale) );
}

void TcpStreamTracker::finish_client_init(TcpSegmentDescriptor& tsd)
{
    Flow* flow = tsd.get_flow();

    rcv_nxt = tsd.get_end_seq();

    if ( !( flow->session_state & STREAM_STATE_MIDSTREAM ) )
    {
        reassembler.set_seglist_base_seq(tsd.get_seq() + 1);
        r_win_base = tsd.get_end_seq();
    }
    else
    {
        reassembler.set_seglist_base_seq(tsd.get_seq() );
        r_win_base = tsd.get_seq();
    }
}

void TcpStreamTracker::update_tracker_ack_recv(TcpSegmentDescriptor& tsd)
{
    if ( SEQ_GT(tsd.get_ack(), snd_una) )
    {
        snd_una = tsd.get_ack();
        if ( snd_nxt < snd_una )
            snd_nxt = snd_una;
    }
}

// In no-ack policy, data is implicitly acked immediately.
void TcpStreamTracker::update_tracker_no_ack_recv(TcpSegmentDescriptor& tsd)
{
    snd_una = snd_nxt = tsd.get_end_seq();
}

void TcpStreamTracker::update_tracker_no_ack_sent(TcpSegmentDescriptor& tsd)
{
    r_win_base = tsd.get_end_seq();
    reassembler.flush_on_ack_policy(tsd.get_pkt());
}

void TcpStreamTracker::update_tracker_ack_sent(TcpSegmentDescriptor& tsd)
{
    // ** this is how we track the last seq number sent
    // as is l_unackd is the "last left" seq recvd
    //snd_una = tsd.get_seg_seq();

    // FIXIT-H add check to validate ack...
    // norm/drop + discard

    if ( SEQ_GT(tsd.get_end_seq(), snd_nxt) )
        snd_nxt = tsd.get_end_seq();

    if ( SEQ_GT(tsd.get_ack(), r_win_base) )
        r_win_base = tsd.get_ack();

    if ( ( fin_seq_status == TcpStreamTracker::FIN_WITH_SEQ_SEEN )
        && SEQ_EQ(r_win_base, fin_final_seq) )
    {
        fin_seq_status = TcpStreamTracker::FIN_WITH_SEQ_ACKED;
    }

    snd_wnd = tsd.get_wnd();
    reassembler.flush_on_ack_policy(tsd.get_pkt());
}

bool TcpStreamTracker::update_on_3whs_ack(TcpSegmentDescriptor& tsd)
{
    bool good_ack = true;

    if ( is_ack_valid(tsd.get_ack()) )
    {
        Flow* flow = tsd.get_flow();

        irs = tsd.get_seq();
        finish_client_init(tsd);
        update_tracker_ack_recv(tsd);
        flow->set_session_flags(SSNFLAG_ESTABLISHED);
        flow->session_state |= ( STREAM_STATE_ACK | STREAM_STATE_ESTABLISHED );
        tcp_state = TcpStreamTracker::TCP_ESTABLISHED;
    }
    else
    {
        inc_tcp_discards();
        normalizer.trim_win_payload(tsd);
        good_ack = false;
    }

    return good_ack;
}

bool TcpStreamTracker::update_on_rst_recv(TcpSegmentDescriptor& tsd)
{
    bool good_rst = true;

    normalizer.trim_rst_payload(tsd);
    if ( normalizer.validate_rst(tsd) )
    {
        Flow* flow = tsd.get_flow();

        flow->set_session_flags(SSNFLAG_RESET);
        if ( normalizer.is_tcp_ips_enabled() )
            tcp_state = TcpStreamTracker::TCP_CLOSED;
    }
    else
    {
        inc_tcp_discards();
        normalizer.packet_dropper(tsd, NORM_TCP_BLOCK);
        good_rst = false;
    }

    return good_rst;
}

void TcpStreamTracker::update_on_rst_sent()
{
    tcp_state = TcpStreamTracker::TCP_CLOSED;
    rst_pkt_sent = true;
}

void TcpStreamTracker::flush_data_on_fin_recv(TcpSegmentDescriptor& tsd)
{
    if ( (flush_policy != STREAM_FLPOLICY_ON_ACK)
        && (flush_policy != STREAM_FLPOLICY_ON_DATA)
        && normalizer.is_tcp_ips_enabled())
    {
        tsd.set_packet_flags(PKT_PDU_TAIL);
    }

    reassembler.flush_on_data_policy(tsd.get_pkt());
}

bool TcpStreamTracker::update_on_fin_recv(TcpSegmentDescriptor& tsd)
{
    if ( SEQ_LT(tsd.get_end_seq(), r_win_base) )
        return false;

    //--------------------------------------------------
    // FIXIT-L don't bump rcv_nxt unless FIN is in seq
    // because it causes bogus 129:5 cases
    // but doing so causes extra gaps
    if ( SEQ_EQ(tsd.get_end_seq(), rcv_nxt) )
        rcv_nxt++;
    else
        fin_seq_adjust = 1;

    // set final seq # any packet rx'ed with seq > is bad
    if ( !fin_seq_set )
    {
        fin_final_seq = tsd.get_end_seq();
        fin_seq_set = true;
        if( tsd.get_len() == 0 )
            fin_seq_status = TcpStreamTracker::FIN_WITH_SEQ_SEEN;
    }

    return true;
}

bool TcpStreamTracker::update_on_fin_sent(TcpSegmentDescriptor& tsd)
{
    update_tracker_ack_sent(tsd);
    snd_nxt++;
    return true;
}

bool TcpStreamTracker::is_segment_seq_valid(TcpSegmentDescriptor& tsd)
{
    bool valid_seq = true;

    int right_ok;
    uint32_t left_seq;

    if ( SEQ_LT(rcv_nxt, r_win_base) )
        left_seq = rcv_nxt;
    else
        left_seq = r_win_base;

    if ( tsd.get_len() )
        right_ok = SEQ_GT(tsd.get_end_seq(), left_seq);
    else
        right_ok = SEQ_GEQ(tsd.get_end_seq(), left_seq);

    if ( right_ok )
    {
        uint32_t win = normalizer.get_stream_window(tsd);

        if ( SEQ_LEQ(tsd.get_seq(), r_win_base + win) )
        {
            return true;
        }
        else
        {
            valid_seq = false;
        }
    }
    else
    {
        valid_seq = false;
    }

    if ( !valid_seq )
    {
        inc_tcp_discards();
        normalizer.trim_win_payload(tsd);
    }

    return valid_seq;
}

bool TcpStreamTracker::set_held_packet(Packet* p)
{
    if ( held_packet != null_iterator )
        return false;

    held_packet = hpq->append(p->daq_msg, p->ptrs.tcph->seq(), *this);

    tcpStats.total_packets_held++;
    if ( ++tcpStats.current_packets_held > tcpStats.max_packets_held )
        tcpStats.max_packets_held = tcpStats.current_packets_held;

    return true;
}

uint32_t TcpStreamTracker::perform_partial_flush()
{
    uint32_t flushed = 0;
    if ( held_packet != null_iterator )
        flushed = reassembler.perform_partial_flush(session->flow);
    return flushed;
}

bool TcpStreamTracker::is_retransmit_of_held_packet(Packet* cp)
{
    if ( (held_packet == null_iterator) or ( cp->daq_msg == held_packet->get_daq_msg() ) )
        return false;

    uint32_t next_send_seq = cp->ptrs.tcph->seq() + (uint32_t)cp->dsize;
    if ( SEQ_LEQ(cp->ptrs.tcph->seq(), held_packet->get_seq_num()) and SEQ_GT(next_send_seq, held_packet->get_seq_num()) )
    {
        tcpStats.held_packet_rexmits++;
        return true;
    }

    return false;
}

void TcpStreamTracker::finalize_held_packet(Packet* cp)
{
    if ( held_packet != null_iterator )
    {
        if ( cp->active->packet_was_dropped() )
        {
            DAQ_Verdict verdict = held_packet->has_expired() ? DAQ_VERDICT_BLACKLIST : DAQ_VERDICT_BLOCK;
            Analyzer::get_local_analyzer()->finalize_daq_message(held_packet->get_daq_msg(), verdict);
            tcpStats.held_packets_dropped++;
        }
        else
        {
            Analyzer::get_local_analyzer()->finalize_daq_message(held_packet->get_daq_msg(), DAQ_VERDICT_PASS);
            tcpStats.held_packets_passed++;
        }

        hpq->erase(held_packet);
        held_packet = null_iterator;
        tcpStats.current_packets_held--;
    }

    if (cp->active->is_packet_held())
        cp->active->cancel_packet_hold();
}

void TcpStreamTracker::finalize_held_packet(Flow* flow)
{
    if ( held_packet != null_iterator )
    {
        if ( (flow->session_state & STREAM_STATE_BLOCK_PENDING) ||
             (flow->ssn_state.session_flags & SSNFLAG_BLOCK) )
        {
            DAQ_Verdict verdict = held_packet->has_expired() ? DAQ_VERDICT_BLACKLIST : DAQ_VERDICT_BLOCK;
            Analyzer::get_local_analyzer()->finalize_daq_message(held_packet->get_daq_msg(), verdict);
            tcpStats.held_packets_dropped++;
        }
        else
        {
            Analyzer::get_local_analyzer()->finalize_daq_message(held_packet->get_daq_msg(), DAQ_VERDICT_PASS);
            tcpStats.held_packets_passed++;
        }

        hpq->erase(held_packet);
        held_packet = null_iterator;
        tcpStats.current_packets_held--;
    }
}

bool TcpStreamTracker::release_held_packets(const timeval& cur_time, int max_remove)
{
    bool is_front_expired = false;
    if ( hpq )
        is_front_expired = hpq->execute(cur_time, max_remove);
    return is_front_expired;
}

void TcpStreamTracker::set_held_packet_timeout(const uint32_t ms)
{
    assert(hpq);
    hpq->set_timeout(ms);
}

bool TcpStreamTracker::adjust_expiration(uint32_t new_timeout_ms, const timeval& now)
{
    assert(hpq);
    return hpq->adjust_expiration(new_timeout_ms, now);
}

void TcpStreamTracker::thread_init()
{
    assert(!hpq);
    hpq = new HeldPacketQueue();
}

void TcpStreamTracker::thread_term()
{
    assert(hpq->empty());
    delete hpq;
    hpq = nullptr;
}
