/*
Copyright (c) <2012>, <Georgia Institute of Technology> All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted
provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions
and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or other materials provided
with the distribution.

Neither the name of the <Georgia Institue of Technology> nor the names of its contributors
may be used to endorse or promote products derived from this software without specific prior
written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

/**********************************************************************************************
 * File         : pcie_endpoint.cc
 * Author       : Joonho
 * Date         : 8/10/2021
 * SVN          : $Id: cxl_t3.h 867 2009-11-05 02:28:12Z kacear $:
 * Description  : PCIe endpoint device
 *********************************************************************************************/
#ifdef CXL
#include <cassert>
#include <iostream>
#include <algorithm>

#include "pcie_endpoint.h"

#include "memreq_info.h"
#include "utils.h"
#include "all_knobs.h"
#include "all_stats.h"
#include "assert_macros.h"

int pcie_ep_c::m_unique_id = 0;

pcie_ep_c::pcie_ep_c(macsim_c* simBase) {
  // simulation related
  m_simBase = simBase;
  m_cycle = 0;

  // set memory request size
  m_lanes = *KNOB(KNOB_PCIE_LANES);
  m_perlane_bw = *KNOB(KNOB_PCIE_PER_LANE_BW);
  m_prev_txphys_cycle = 0;
  m_peer_ep = NULL;

  m_txvc_rr_idx = 0;

  // initialize VC buffers & credit
  m_vc_cnt = *KNOB(KNOB_PCIE_VC_CNT);
  m_txvc_cap = *KNOB(KNOB_PCIE_TXVC_CAPACITY);
  m_rxvc_cap = *KNOB(KNOB_PCIE_RXVC_CAPACITY);
  m_txvc_buff = new list<message_s*>[m_vc_cnt];
  m_rxvc_buff = new list<message_s*>[m_vc_cnt];

  ASSERTM(m_vc_cnt == 2, "currently only 2 virtual channels exist\n");

  // initialize physical layers
  m_phys_cap = *KNOB(KNOB_PCIE_PHYS_CAPACITY);
}

pcie_ep_c::~pcie_ep_c() {
  delete[] m_txvc_buff;
  delete[] m_rxvc_buff;
}

void pcie_ep_c::init(int id, bool master, pool_c<message_s>* msg_pool, pcie_ep_c* peer) {
  m_id = id;
  m_master = master;
  m_msg_pool = msg_pool;
  m_peer_ep = peer;
}

void pcie_ep_c::run_a_cycle(bool pll_locked) {
  // receive requests
  end_transaction();
  process_rxtrans();
  process_rxdll();
  process_rxphys();

  // send requests
  process_txphys();
  process_txdll();
  process_txtrans();
  start_transaction();

  m_cycle++;
}

bool pcie_ep_c::insert_rxphys(message_s* pkt) {
  if ((int)m_rxphys_q.size() >= m_phys_cap) {
    return false;
  } else {
    m_rxphys_q.push_back(pkt);
    return true;
  }
}

bool pcie_ep_c::check_peer_credit(message_s* pkt) {
  int vc_id = pkt->m_vc_id;
  return (m_peer_ep->m_rxvc_buff[vc_id].size() < m_peer_ep->m_rxvc_cap);
}

//////////////////////////////////////////////////////////////////////////////
// private

Counter pcie_ep_c::get_phys_latency(message_s* pkt) {
  float freq = *KNOB(KNOB_CLOCK_IO);
  return static_cast<Counter>(pkt->m_bits / m_perlane_bw * freq);
}

bool pcie_ep_c::phys_layer_full(bool tx) {
  if (tx) {
    return (m_phys_cap == (int)m_txphys_q.size());
  } else { // rx
    return (m_phys_cap == (int)m_rxphys_q.size());
  }
}

void pcie_ep_c::init_new_msg(message_s* msg, int bits, int vc_id, mem_req_s* req) {
  msg->m_id = ++m_unique_id;
  msg->m_bits = bits;
  msg->m_vc_id = vc_id;
  msg->m_req = req;
}

bool pcie_ep_c::txvc_not_full(int channel) {
  return (m_txvc_cap - (int)m_txvc_buff[channel].size() > 0);
}

//////////////////////////////////////////////////////////////////////////////
// protected

// virtual class
// should call push_txvc internally
// look at pcie_rc_c, cxl_t3_c for examples
void pcie_ep_c::start_transaction() {
  return;
}

// used for start_transaction
bool pcie_ep_c::push_txvc(mem_req_s* mem_req) {
  message_s* new_msg;
  int channel;

  if (m_master) {
    channel = (mem_req && mem_req->m_type == MRT_WB) ? WD_CHANNEL : WOD_CHANNEL;
  } else {
    channel = (mem_req != NULL) ? WD_CHANNEL : WOD_CHANNEL;
  }

  if (!txvc_not_full(channel)) { // txvc full
    return false;
  } else {
    new_msg = m_msg_pool->acquire_entry(m_simBase);
    init_new_msg(new_msg, 544, channel, mem_req);
    m_txvc_buff[channel].push_back(new_msg);
    return true;
  }
}

// virtual class
// should call pull_rxvc internally
// look at pcie_rc_c, cxl_t3_c for examples
void pcie_ep_c::end_transaction() {
  return;
}

// used for end_transaction
mem_req_s* pcie_ep_c::pull_rxvc() {
  vector<pair<int, int>> candidate;
  for (int ii = 0; ii < m_vc_cnt; ii++) {
    // empty
    if ((int)m_rxvc_buff[ii].size() == 0) {
      continue;
    }
    // not empty 
    else {
      int remain = m_rxvc_cap - (int)m_rxvc_buff[ii].size();
      candidate.push_back({remain, ii});
    }
  }

  sort(candidate.begin(), candidate.end());

  for (auto cand : candidate) {
    int vc_id = cand.second;

    message_s* pkt = m_rxvc_buff[vc_id].front();
    m_rxvc_buff[vc_id].pop_front();

    if (pkt->m_rxtrans_end > m_cycle) {
      continue;
    } else {
      assert(pkt->m_vc_id == vc_id);
      assert(pkt->m_req);

      mem_req_s* mem_req = pkt->m_req;
      m_msg_pool->release_entry(pkt);
      return mem_req;
    }
  }
  return NULL;
}

//////////////////////////////////////////////////////////////////////////////

void pcie_ep_c::process_txtrans() {
  // round robin policy
  for (int ii = m_txvc_rr_idx, cnt = 0; cnt < m_vc_cnt; 
      cnt++, ii = (ii + 1)  % m_vc_cnt) {
    // VC buffer emtpy
    if (m_txvc_buff[ii].empty()) {
      continue;
    }
    // VC buffer not empty
    else {
      message_s* pkt = m_txvc_buff[ii].front();
      int vc_id = pkt->m_vc_id;

      // don't consider about flow control packets now
      if (!check_peer_credit(pkt)) {
        continue;
      } else {
        if (!phys_layer_full(TX)) {
          pkt->m_txtrans_end = m_cycle + *KNOB(KNOB_PCIE_TXLOGIC_LATENCY);
          m_txvc_buff[vc_id].pop_front();
          m_txphys_q.push_back(pkt);
          break;
        }
      }
    }
  }
  m_txvc_rr_idx = (m_txvc_rr_idx + 1) % m_vc_cnt;
}

void pcie_ep_c::process_txdll() {
  return;
}

void pcie_ep_c::process_txphys() {
  if (m_txphys_q.size() == 0) {
    return;
  }

  message_s* pkt = m_txphys_q.front();

  if (pkt->m_txtrans_end > m_cycle) {
    return;
  } else {
    // insert the packet to peer's receive phys q
    if (m_peer_ep->insert_rxphys(pkt)) {
      m_txphys_q.pop_front();

      // - packets are sent serially so transmission starts only after
      //   the previsou packet finished physical layer transmission
      Counter lat = get_phys_latency(pkt);
      Counter start_cyc = max(m_prev_txphys_cycle, m_cycle);
      Counter phys_finished = start_cyc + lat;
      m_prev_txphys_cycle = phys_finished;

      pkt->m_phys_end = phys_finished;

      // - instead of modeling the rx logic layer latency separately,
      //   just add the latency here and skip process_rxlogic
      pkt->m_rxtrans_end = phys_finished + 
        *KNOB(KNOB_PCIE_RXLOGIC_LATENCY);

      m_rxphys_q.push_back(pkt);
    }
  }
}

void pcie_ep_c::process_rxphys() {
  while (m_rxphys_q.size()) {
    message_s* pkt = m_rxphys_q.front();

    // finished physical layer & rx logic layer
    if (pkt->m_rxtrans_end <= m_cycle) {
      m_rxphys_q.pop_front();
      m_rxvc_buff[pkt->m_vc_id].push_back(pkt);
    } else {
      break;
    }
  }
}

void pcie_ep_c::process_rxdll() {
  return;
}

void pcie_ep_c::process_rxtrans() {
  return;
}

void pcie_ep_c::print_ep_info() {
  return;
}
#endif // CXL