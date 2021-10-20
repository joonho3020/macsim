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
#ifdef CXL

#include <iostream>
#include <list>

#include "cxl_t3.h"
#include "pcie_rc.h"
#include "macsim.h"
#include "memory.h"
#include "debug_macros.h"

#include "all_knobs.h"
#include "statistics.h"

#include "dram_ramulator.h"
#include "ramulator/src/Request.h"

#undef DEBUG
#define DEBUG(args...) _DEBUG(*m_simBase->m_knobs->KNOB_DEBUG_DRAM, ##args)

using namespace ramulator;

cxlt3_c::cxlt3_c(macsim_c* simBase) 
  : pcie_ep_c(simBase),
    cme_requestsInFlight(0),
    wrapper(NULL),
    read_cb_func(
      std::bind(&cxlt3_c::readComplete, this, std::placeholders::_1)),
    write_cb_func(std::bind(&cxlt3_c::writeComplete, this,
                            std::placeholders::_1)) {
  // init queues
  m_pending_req = new list<mem_req_s*>();

  // init ramulator
  std::string config_file(*KNOB(KNOB_RAMULATOR_CONFIG_FILE));
  configs.parse(config_file);
  configs.set_core_num(*KNOB(KNOB_NUM_SIM_CORES));

  wrapper = new ramulator::RamulatorWrapper(
    configs, *KNOB(KNOB_RAMULATOR_CACHELINE_SIZE),
    *KNOB(KNOB_RAMUCME_STATOUT));

  // init others
  m_cycle_internal = 0;
}

cxlt3_c::~cxlt3_c() {
  wrapper->finish();
  delete wrapper;
  delete m_pending_req;
}

void cxlt3_c::run_a_cycle(bool pll_locked) {
  // send response
  process_txphys();
  process_txlogic();
  start_transaction();

  // process memory requests
/* m_dramsim->ClockTick(); */
  process_pending_req();

  // receive memory request
  end_transaction(); 
  process_rxlogic();
  process_rxphys();

  m_cycle++;
}

void cxlt3_c::run_a_cycle_internal(bool pll_locked) {
  wrapper->tick();
  m_cycle_internal++;
}

void cxlt3_c::start_transaction() {
  for (auto I = cme_resp_queue.begin(); I != cme_resp_queue.end(); ++I) {
    mem_req_s* req = *I;
    if (push_txvc(req)) {
      cme_resp_queue.pop_front();
      req->m_state = CME_PCIE_RETURNING;
    } else {
      break;
    }
  }
}

// transactions ends in the viewpoint of RC
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
  for (auto I = m_pending_req->begin(); I != m_pending_req->end(); I++) {
    mem_req_s* req = *I;
    if (push_ramu_req(req)) {
      tmp_list.push_back(req);
    }
  }

  for (auto I = tmp_list.begin(), end = tmp_list.end(); I != end; ++I) {
    m_pending_req->remove(*I);
  }
}

bool cxlt3_c::push_ramu_req(mem_req_s* req) {
  bool is_write = (req->m_type == MRT_WB);
  auto req_type = (is_write) ? ramulator::Request::Type::WRITE
                             : ramulator::Request::Type::READ;
  auto cb_func = (is_write) ? write_cb_func : read_cb_func;
  long addr = static_cast<long>(req->m_addr);

  ramulator::Request ramu_req(addr, req_type, cb_func, req->m_core_id);
  bool accepted = wrapper->send(ramu_req);

  if (accepted) {
    if (is_write) {
      cme_writes[ramu_req.addr].push_back(req);
      DEBUG("Write to 0x%lx accepted and served. req:%d\n", ramu_req.addr,
            req->m_id);
    } else {
      cme_reads[ramu_req.addr].push_back(req);
      DEBUG("Read to 0x%lx accepted. req:%d\n", ramu_req.addr, req->m_id);
    }

    // added counter to track requests in flight
    ++cme_requestsInFlight;
    req->m_state = CME_REQ_START;

    return true;
  } else {
    if (is_write) {
      DEBUG("Write to 0x%lx NOT accepted. req:%d\n", ramu_req.addr, req->m_id);
    } else {
      DEBUG("Read to 0x%lx NOT accepted. req:%d\n", ramu_req.addr, req->m_id);
    }
    return false;
  }
}

void cxlt3_c::readComplete(ramulator::Request &ramu_req) {
  DEBUG("Read to 0x%lx completed.\n", ramu_req.addr);
  auto &req_q = cme_reads.find(ramu_req.addr)->second;
  mem_req_s *req = req_q.front();
  req_q.pop_front();
  if (!req_q.size()) cme_reads.erase(ramu_req.addr);

  // added counter to track requests in flight
  --cme_requestsInFlight;
  req->m_state = CME_REQ_DONE;
  
  // read stats should be updated when they return to MC

  DEBUG("Queuing response for address 0x%lx\n", ramu_req.addr);
  cme_resp_queue.push_back(req);
}

void cxlt3_c::writeComplete(ramulator::Request &ramu_req) {
  DEBUG("Write to 0x%lx completed.\n", ramu_req.addr);
  auto &req_q = cme_writes.find(ramu_req.addr)->second;
  mem_req_s *req = req_q.front();
  req_q.pop_front();
  if (!req_q.size()) cme_writes.erase(ramu_req.addr);

  // added counter to track requests in flight
  --cme_requestsInFlight;
  req->m_state = CME_REQ_DONE;

  // update CME return latency stats : writes don't return so finish here
  STAT_EVENT(AVG_CME_WR_TURN_LATENCY_BASE);
  STAT_EVENT_N(AVG_CME_WR_TURN_LATENCY, 
      m_simBase->m_core_cycle[req->m_core_id] - req->m_insert_cycle);

  // in case of WB, retire requests here
  DEBUG("Retiring request for address 0x%lx\n", ramu_req.addr);
  MEMORY->free_req(req->m_core_id, req);
}

void cxlt3_c::print_cxlt3_info() {
  std::cout << "-------------- CME ------------------" << std::endl;
  print_ep_info();

  std::cout << "pending q" << ": ";
  for (auto req : *m_pending_req) {
    std::cout << std::hex << req->m_addr << " ; ";
  }

  std::cout << cme_requestsInFlight << std::endl;

  std::cout << "Read q" << std::endl;
  for (auto iter : cme_reads) {
    auto addr = iter.first;
    auto req_q = iter.second;
    std::cout << "Addr: " << std::hex << addr << ": ";
    for (auto req : req_q)  {
      std::cout << req << "; ";
    }
    std::cout << std::endl;
  }

  std::cout << "Write q" << std::endl;
  for (auto iter : cme_writes) {
    auto addr = iter.first;
    auto req_q = iter.second;
    std::cout << "Addr: " << std::hex << addr << ": ";
    for (auto req : req_q)  {
      std::cout << req << "; ";
    }
    std::cout << std::endl;
  }

  for (auto iter : *m_pending_req) {
    auto addr = iter->m_addr;
    std::cout << "Addr: " << std::hex << addr << ": ";
  }
  std::cout << std::endl;
}

#endif //CXL
