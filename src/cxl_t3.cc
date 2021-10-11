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
 * File         : cxl_t3.h
 * Author       : Joonho
 * Date         : 8/10/2021
 * SVN          : $Id: cxl_t3.h 867 2009-11-05 02:28:12Z kacear $:
 * Description  : CXL type 3 device
 *********************************************************************************************/
#ifdef DRAMSIM3

#include <iostream>
#include <list>

#include "cxl_t3.h"
#include "macsim.h"
#include "memory.h"
#include "dramsim3.h"
#include "all_knobs.h"

using namespace dramsim3;

cxlt3_c::cxlt3_c(macsim_c* simBase) 
  : pcie_ep_c(simBase) {
  m_pending_req = new list<mem_req_s*>();
  m_pushed_req = new list<mem_req_s*>();
  m_done_req = new list<mem_req_s*>();
  m_tmp_done_req = new list<mem_req_s*>();

  std::string config_file = *KNOB(KNOB_CME_DRAMSIM3_CONFIG);
  std::string working_dir = *KNOB(KNOB_CME_DRAMSIM3_OUT);
  m_dramsim = new MemorySystem(config_file, working_dir, NULL, NULL);

  std::function<void(uint64_t)> read_cb;
  std::function<void(uint64_t)> write_cb;
  read_cb = 
    std::bind(&cxlt3_c::read_callback, this, 0, std::placeholders::_1);
  write_cb = 
    std::bind(&cxlt3_c::write_callback, this, 0, std::placeholders::_1);
  m_dramsim->RegisterCallbacks(read_cb, write_cb);
}

cxlt3_c::~cxlt3_c() {
  delete m_pending_req;
  delete m_pushed_req;
  delete m_done_req;
  delete m_tmp_done_req;
  delete m_dramsim;
}

void cxlt3_c::run_a_cycle(bool pll_locked) {
  if (pll_locked) {
    m_cycle++;
    return;
  }

  // send response
  process_txphys();
  process_txlogic();
  start_transaction();

  // process memory requests
  m_dramsim->ClockTick();
  process_pending_req();

  // receive memory request
  end_transaction(); 
  process_rxlogic();
  process_rxphys();

  m_cycle++;
}

void cxlt3_c::start_transaction() {
  vector<mem_req_s*> tmp_list;
  for (auto iter = m_tmp_done_req->begin(); iter != m_tmp_done_req->end();
      ++iter) {
    mem_req_s* req = *iter;
    if (req->m_rdy_cycle <= m_cycle && push_txvc(req)) {
      tmp_list.push_back(req);
    } else {
      break;
    }
  }

  for (auto iter = tmp_list.begin(); iter != tmp_list.end(); ++iter) {
    m_tmp_done_req->remove(*iter);
  }

  tmp_list.clear();
  for (auto iter = m_done_req->begin(); iter != m_done_req->end(); ++iter) {
    mem_req_s* req = *iter;
    if (push_txvc(req)) {
      tmp_list.push_back(req);
    } else {
      break;
    }
  }

  for (auto iter = tmp_list.begin(), end = tmp_list.end(); iter != end; ++iter) {
    m_done_req->remove(*iter);
  }
}

void cxlt3_c::end_transaction() {
  while (1) {
    mem_req_s* req = pull_rxvc();
    if (!req) {
      break;
    } else {
      m_pending_req->push_back(req);
    }
  }
}

void cxlt3_c::process_pending_req() {
  vector<mem_req_s*> tmp_list;

  for (auto iter = m_pending_req->begin(); iter != m_pending_req->end();
      iter++) {
    mem_req_s* req = *iter;
    bool is_write = (req->m_type == MRT_WB);
    uint64_t addr_ = static_cast<uint64_t>(req->m_addr);
    bool will_accept = m_dramsim->WillAcceptTransaction(addr_, is_write);
    if (will_accept) {
      m_dramsim->AddTransaction(addr_, is_write);
      m_pushed_req->push_back(req);
      tmp_list.push_back(req);
    }
  }

  for (auto iter = tmp_list.begin(), end = tmp_list.end(); iter != end; ++iter) {
    m_pending_req->remove(*iter);
  }
}

void cxlt3_c::read_callback(unsigned id, uint64_t address) {
  // find requests with this address
  for (auto iter = m_pushed_req->begin(), end = m_pushed_req->end();
      iter != end; ++iter) {
    mem_req_s* req = *iter;

    if (req->m_addr == address) {
      if (*KNOB(KNOB_DRAM_ADDITIONAL_LATENCY)) {
        req->m_rdy_cycle = m_cycle + *KNOB(KNOB_DRAM_ADDITIONAL_LATENCY);
        m_tmp_done_req->push_back(req);
      } else {
        m_done_req->push_back(req);
      }
      m_pushed_req->remove(req);

      // return first request with matching address
      // this may not necessarily be true but this is the best we can do
      // at this point
      break;
    }
  }
}

void cxlt3_c::write_callback(unsigned id, uint64_t address) {
  // find requests with this address
  for (auto iter = m_pushed_req->begin(), end = m_pushed_req->end();
      iter != end; ++iter) {
    mem_req_s* req = *iter;

    if (req->m_addr == address) {
      // in case of WB, retire requests here
      MEMORY->free_req(req->m_core_id, req);
      m_pushed_req->remove(req);

      // return first request with matching address
      // this may not necessarily be true but this is the best we can do
      // at this point
      break;
    }
  }
}

void cxlt3_c::print_cxlt3_info() {
  std::cout << "-------------- CME ------------------" << std::endl;
  print_ep_info();

  std::cout << "pending q" << ": ";
  for (auto req : *m_pending_req) {
    std::cout << std::hex << req->m_addr << " ; ";
  }

  std::cout << "pushed q" << ": ";
  for (auto req : *m_pushed_req) {
    std::cout << std::hex << req->m_addr << " ; ";
  }
  
  std::cout << "done q" << ": ";
  for (auto req : *m_done_req) {
    std::cout << std::hex << req->m_addr << " ; ";
  }

  std::cout << std::endl;
}

#endif //DRAMSIM3
