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
#include <cassert>
#include <iostream>
#include <algorithm>

#include "pcie_endpoint.h"

#include "memreq_info.h"
#include "utils.h"
#include "all_knobs.h"
#include "all_stats.h"

int pcie_ep_c::m_unique_id = 0;

pcie_ep_c::pcie_ep_c(macsim_c* simBase) {
  // simulation related
  m_simBase = simBase;
  m_cycle = 0;

  // set memory request size
  m_memreq_size = *KNOB(KNOB_PCIE_MEMREQ_BYTES);
  m_rr_idx = 0;
  m_lanes = *KNOB(KNOB_PCIE_LANES);
  m_perlane_bw = *KNOB(KNOB_PCIE_PER_LANE_BW);
  m_prev_txphys_cycle = 0;
  m_peer_ep = NULL;

  // initialize VC buffers & credit
  m_vc_cnt = *KNOB(KNOB_PCIE_VC_CNT);
  m_vc_cap = *KNOB(KNOB_PCIE_VC_CAPACITY);
  m_txvc_size = new int[m_vc_cnt];
  m_rxvc_size = new int[m_vc_cnt];
  for (int ii = 0; ii < m_vc_cnt; ii++) {
    m_txvc_size[ii] = m_vc_cap;
    m_rxvc_size[ii] = m_vc_cap;
  }
  m_txvc_buff = new list<packet_info_s*>[m_vc_cnt];
  m_rxvc_buff = new list<packet_info_s*>[m_vc_cnt];

  // initialize credit
  m_credit_cap = m_vc_cap;  // credit = pair's vc capacity
  m_credit = new int[m_vc_cnt];
  for (int ii = 0; ii < m_vc_cnt; ii++) {
    m_credit[ii] = m_vc_cap;
  }

  // initialize physical layers
  m_phys_cap = *KNOB(KNOB_PCIE_PHYS_CAPACITY);
  m_txphys_size = m_phys_cap;
  m_rxphys_size = m_phys_cap;
  m_txphys_q = new deque<packet_info_s*>();
  m_rxphys_q = new deque<packet_info_s*>();
}

pcie_ep_c::~pcie_ep_c() {
  delete[] m_txvc_buff;
  delete[] m_rxvc_buff;
  delete m_txphys_q;
  delete m_rxphys_q;
}

void pcie_ep_c::init(int id, pool_c<packet_info_s>* pkt_pool, 
    pcie_ep_c* peer) {
  m_id = id;
  m_pkt_pool = pkt_pool;
  m_peer_ep = peer;
}

void pcie_ep_c::run_a_cycle(bool pll_locked) {
  if (pll_locked) {
    m_cycle++;
    return;
  }

  // receive requests
  end_transaction();
  process_rxlogic();
  process_rxphys();

  // send requests
  process_txphys();
  process_txlogic();
  start_transaction();

  m_cycle++;
}

bool pcie_ep_c::insert_rxphys(packet_info_s* pkt) {
  if (m_rxphys_q->size() >= m_rxphys_size) {
    return false;
  } else {
    m_rxphys_q->push_back(pkt);
    return true;
  }
}

// virtual class
// should call push_txvc internally
// look at pcie_rc_c, cxl_t3_c for examples
void pcie_ep_c::start_transaction() {
  return;
}

// virtual class
// should call pull_rxvc internally
// look at pcie_rc_c, cxl_t3_c for examples
void pcie_ep_c::end_transaction() {
  return;
}

void pcie_ep_c::process_txlogic() {
  // round robin policy
  for (int ii = m_rr_idx, cnt = 0; cnt < m_vc_cnt; 
      cnt++, ii = (ii + 1)  % m_vc_cnt) {
    // VC buffer emtpy
    if (m_txvc_buff[ii].empty()) {
      continue;
    }
    // VC buffer not empty
    else {
      bool is_tx = true;
      packet_info_s* pkt = m_txvc_buff[ii].front();

      // don't consider about flow control packets now
      if (!check_credit(pkt)) {
        continue;
      } else {
        if (!phys_layer_full(is_tx)) {
          decrease_credit(pkt);
          pkt->m_logic_end = m_cycle + *KNOB(KNOB_PCIE_TXLOGIC_LATENCY);
          pull_vc_buffer(pkt->m_vc_id, m_txvc_size, m_txvc_buff);
          insert_tx_phys(pkt, false);  // insert to back
          break;
        }
      }
    }
  }
  m_rr_idx = (m_rr_idx + 1) % m_vc_cnt;
}

void pcie_ep_c::process_txphys() {
  if (m_txphys_q->size() == 0) {
    return;
  }

  packet_info_s* pkt = m_txphys_q->front();

  if (pkt->m_logic_end > m_cycle) {
    return;
  } else {
    // insert the packet to peer's receive phys q
    if (m_peer_ep->insert_rxphys(pkt)) {
      m_txphys_q->pop_front();

      // - packets are sent serially so transmission starts only after
      //   the previsou packet finished physical layer transmission
      Counter lat = get_phys_latency(pkt);
      pkt->m_phys_start = max(m_prev_txphys_cycle, m_cycle);
      Counter phys_finished = pkt->m_phys_start + lat;
      m_prev_txphys_cycle = phys_finished;

      // - instead of modeling the rx logic layer latency separately,
      //   just add the latency here and skip process_rxlogic
      pkt->m_rxlogic_finished = phys_finished + 
        *KNOB(KNOB_PCIE_RXLOGIC_LATENCY);
    }
  }
}

void pcie_ep_c::process_rxphys() {
  while (m_rxphys_q->size()) {
    packet_info_s* pkt = m_rxphys_q->front();

    // finished physical layer & rx logic layer
    if (pkt->m_rxlogic_finished <= m_cycle) {
      assert(m_rxvc_size[pkt->m_vc_id] >= pkt->m_bytes);
      m_rxphys_q->pop_front();
      insert_vc_buff(pkt->m_vc_id, m_rxvc_size, m_rxvc_buff, pkt);
    } else {
      break;
    }
  }
}

void pcie_ep_c::process_rxlogic() {
  // skip since the latency is considered in process_txphys
}

Counter pcie_ep_c::get_phys_latency(packet_info_s* pkt) {
  return static_cast<Counter>(pkt->m_bytes * m_perlane_bw / m_lanes);
}

void pcie_ep_c::decrease_credit(packet_info_s* flow_pkt) {
  m_credit[flow_pkt->m_vc_id] -= flow_pkt->m_bytes;
}

void pcie_ep_c::update_credit(packet_info_s* fctrl_pkt) {
  m_credit[fctrl_pkt->m_vc_id] = fctrl_pkt->m_credits;
}

void pcie_ep_c::update_credit(int vc_id, int credit) {
  m_credit[vc_id] = credit;
}

bool pcie_ep_c::check_credit(packet_info_s* pkt) {
  return pkt->m_bytes <= m_credit[pkt->m_vc_id];
}

bool pcie_ep_c::phys_layer_full(bool tx) {
  if (tx) {
    return m_txphys_q->size() >= m_txphys_size;
  } else { // rx
    return m_rxphys_q->size() >= m_rxphys_size;
  }
}

void pcie_ep_c::insert_tx_phys(packet_info_s* pkt, bool front) {
  if (front) {
    m_txphys_q->push_front(pkt);
  } else {
    m_txphys_q->push_back(pkt);
  }
}

// used for start_transaction
bool pcie_ep_c::push_txvc(mem_req_s* mem_req) {
  // insert to VC buffer with maximum remaining capacity
  int max_remain_size = -1;
  int max_remain_id = -1;
  for (int ii = 0; ii < m_vc_cnt; ii++) {
    int cur_remain = m_txvc_size[ii] - m_memreq_size;
    if (max_remain_size < cur_remain) {
      max_remain_size = cur_remain;
      max_remain_id = ii;
    }
  }

  // all buffers full
  if (max_remain_id == -1) {
    return false;
  } 
  // found buffer to insert
  else {
    int vc_id = max_remain_id;
    Pkt_Type pkt_type = (mem_req->m_type == MRT_WB) ? PKT_MWR : PKT_MRD;
    Pkt_State pkt_state = PKT_TVC;

    packet_info_s* new_pkt = m_pkt_pool->acquire_entry(m_simBase);
    init_new_pkt(new_pkt, m_memreq_size, vc_id, -1, 
      pkt_type, pkt_state, mem_req);

    insert_vc_buff(vc_id, m_txvc_size, m_txvc_buff, new_pkt);
    return true;
  }
}

// used for end_transaction
mem_req_s* pcie_ep_c::pull_rxvc() {
/* // pull from VC buffer with minimum remaining capacity */
/* int min_remain_size = m_vc_cap; */
/* int min_remain_id = -1; */
/* for (int ii = 0; ii < m_vc_cnt; ii++) { */
/* int cur_remain = m_rxvc_size[ii]; */
/* if (min_remain_size > cur_remain) { */
/* min_remain_size = cur_remain; */
/* min_remain_id = ii; */
/* } */
/* } */

/* // all buffers are empty */
/* if (min_remain_id == -1) { */
/* return NULL; */
/* } */
/* // found buffer to pull */
/* else { */
/* int vc_id = min_remain_id; */
/* packet_info_s* pkt = pull_vc_buffer(vc_id, m_rxvc_size, m_rxvc_buff); */

/* // update peer endpoint credit */
/* m_peer_ep->update_credit(vc_id, m_rxvc_size[vc_id]); */

/* assert(pkt->m_vc_id == vc_id); */
/* assert(pkt->m_req); */

/* mem_req_s* mem_req = pkt->m_req; */
/* m_pkt_pool->release_entry(pkt); */
/* return mem_req; */
/* } */

  vector<pair<int, int>> candidate;
  for (int ii = 0; ii < m_vc_cnt; ii++) {
    // empty
    if (m_rxvc_size[ii] == m_vc_cap) {
      continue;
    }
    // not empty 
    else {
      int remain = m_vc_cap - m_rxvc_size[ii];
      candidate.push_back({remain, ii});
    }
  }

  sort(candidate.begin(), candidate.end());

  for (auto cand : candidate) {
    int vc_id = cand.second;
    packet_info_s* pkt = pull_vc_buffer(vc_id, m_rxvc_size, m_rxvc_buff);

    if (pkt->m_rxlogic_finished > m_cycle) {
      continue;
    } else {
      // update peer endpoint credit
      m_peer_ep->update_credit(vc_id, m_rxvc_size[vc_id]);

      assert(pkt->m_vc_id == vc_id);
      assert(pkt->m_req);

      mem_req_s* mem_req = pkt->m_req;
      m_pkt_pool->release_entry(pkt);
      return mem_req;
    }
  }
  return NULL;
}

void pcie_ep_c::init_new_pkt(packet_info_s* pkt, int bytes, int vc_id,
  int credits, Pkt_Type pkt_type, Pkt_State pkt_state, mem_req_s* req) {
  pkt->m_id = ++m_unique_id;
  pkt->m_bytes = bytes;
  pkt->m_vc_id = vc_id;
  pkt->m_credits = credits;
  pkt->m_pkt_type = pkt_type;
  pkt->m_pkt_state = pkt_state;
  pkt->m_req = req;
}

void pcie_ep_c::insert_vc_buff(int vc_id, int* size,
    list<packet_info_s*>* buff, packet_info_s* pkt) {
  assert(size[vc_id] >= pkt->m_bytes);

  size[vc_id] -= pkt->m_bytes;
  buff[vc_id].push_back(pkt);
}

packet_info_s* pcie_ep_c::pull_vc_buffer(int vc_id, int *size, 
    list<packet_info_s*>* buff) {
  assert(buff[vc_id].size() != 0);

  packet_info_s* pkt = buff[vc_id].front();
  buff[vc_id].pop_front();
  size[vc_id] += pkt->m_bytes;
  return pkt;
}

void pcie_ep_c::print_ep_info() {
  std::cout << "txvc" << ": ";
  for (int ii = 0; ii < m_vc_cnt; ii++) {
    for (auto pkt : m_txvc_buff[ii]) {
      std::cout << std::hex << pkt->m_req->m_addr << "; ";
    }
  }

  std::cout << "txphys" << ": ";
  for (auto pkt : *m_txphys_q) {
    std::cout << std::hex << pkt->m_req->m_addr << "; ";
  }

  std::cout << "rxvc" << ": ";
  for (int ii = 0; ii < m_vc_cnt; ii++) {
    for (auto pkt : m_rxvc_buff[ii]) {
      std::cout << std::hex << pkt->m_req->m_addr << "; ";
    }
  }

  std::cout << "rxphys" << ": ";
  for (auto pkt : *m_rxphys_q) {
    std::cout << std::hex << pkt->m_req->m_addr << "; ";
  }
}
