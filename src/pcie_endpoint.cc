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

#include "pcie_endpoint.h"

#include "memreq_info.h"
#include "all_knobs.h"
#include "all_stats.h"

int pcie_ep_c::m_unique_id = 0;

pcie_ep_c::pcie_ep_c(macsim_c* simBase) {
  // set memory request size
  m_memreq_size = *KNOB(KNOB_PCIE_MEMREQ_BYTES);

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

  // simulation related
  m_simBase = simBase;
  m_cycle = 0;
}

pcie_ep_c::~pcie_ep_c() {
  delete[] m_txvc_buff;
  delete[] m_rxvc_buff;
  delete m_txphys_q;
  delete m_rxphys_q;
}

void pcie_ep_c::init(int id, pool_c<packet_info_s>* pkt_pool) {
  m_id = id;
  m_pkt_pool = pkt_pool;
}

void pcie_ep_c::run_a_cycle(bool pll_locked) {
  if (pll_locked) {
    m_cycle++;
    return;
  }
  process_rxphys();
  process_txphys();
  process_txlogic();

  m_cycle++;
}

void pcie_ep_c::process_txlogic() {
}

void pcie_ep_c::process_txphys() {
}

void pcie_ep_c::process_rxphys() {
}

Counter pcie_ep_c::get_phys_latency(packet_info_s* pkt) {
}

void pcie_ep_c::update_credit(packet_info_s* flow_pkt, bool increment) {
}

bool pcie_ep_c::check_credit(packet_info_s* pkt) {
}

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
    packet_info_s* new_pkt = m_pkt_pool->acquire_entry(m_simBase);
    Pkt_Type pkt_type = (mem_req->m_type == MRT_WB) ? PKT_MWR : PKT_MRD;
    Pkt_State pkt_state = PKT_TVC;
    init_new_pkt(new_pkt, m_memreq_size, vc_id, pkt_type, pkt_state,
      mem_req);
    
    m_txvc_size[vc_id] -= m_memreq_size;
  }
}

mem_req_s* pcie_ep_c::pull_rxvc() {
}

void pcie_ep_c::init_new_pkt(packet_info_s* pkt, int bytes, int vc_id, 
  Pkt_Type pkt_type, Pkt_State pkt_state, mem_req_s* req) {
  pkt->m_id = ++m_unique_id;
  pkt->m_bytes = bytes;
  pkt->m_vc_id = vc_id;
  pkt->m_pkt_type = pkt_type;
  pkt->m_pkt_state = pkt_state;
  pkt->m_req = req;
}