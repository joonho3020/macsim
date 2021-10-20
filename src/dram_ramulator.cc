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
 * File         : dram_ramulator.cc
 * Author       : HPArch Research Group
 * Date         : 11/28/2017
 * Description  : Ramulator interface
 *********************************************************************************************/

#ifdef RAMULATOR

#include <map>

#include "all_knobs.h"
#include "bug_detector.h"
#include "debug_macros.h"
#include "memory.h"
#include "network.h"
#include "statistics.h"

#include "dram_ramulator.h"
#include "ramulator/src/Request.h"

#undef DEBUG
#define DEBUG(args...) _DEBUG(*m_simBase->m_knobs->KNOB_DEBUG_DRAM, ##args)

using namespace ramulator;

dram_ramulator_c::dram_ramulator_c(macsim_c *simBase)
  : dram_c(simBase),
    ramu_requestsInFlight(0),
    cme_requestsInFlight(0),
    wrapper(NULL),
    read_cb_func(
      std::bind(&dram_ramulator_c::readComplete, this, std::placeholders::_1)),
    write_cb_func(std::bind(&dram_ramulator_c::writeComplete, this,
                            std::placeholders::_1)) {
  std::string config_file(*KNOB(KNOB_RAMULATOR_CONFIG_FILE));
  configs.parse(config_file);
  configs.set_core_num(*KNOB(KNOB_NUM_SIM_CORES));

  wrapper = new ramulator::RamulatorWrapper(
    configs, *KNOB(KNOB_RAMULATOR_CACHELINE_SIZE),
    *KNOB(KNOB_RAMUDIMM_STATOUT));
}

dram_ramulator_c::~dram_ramulator_c() {
  wrapper->finish();
  delete wrapper;
}

void dram_ramulator_c::init(int id) {
  m_id = id;
}

void dram_ramulator_c::print_req(void) {
  std::cout << "---- DIMM ----" << std::endl;
  std::cout << std::dec << "DIMM reqs: " << ramu_requestsInFlight
                        << "CME reqs: " << cme_requestsInFlight << std::endl;

  std::cout << "Read q" << std::endl;
  for (auto iter : ramu_reads) {
    auto addr = iter.first;
    auto req_q = iter.second;
    std::cout << "Addr: " << std::hex << addr << ": ";
    for (auto req : req_q)  {
      std::cout << req << "; ";
    }
    std::cout << std::endl;
  }

  std::cout << "Write q" << std::endl;
  for (auto iter : ramu_writes) {
    auto addr = iter.first;
    auto req_q = iter.second;
    std::cout << "Addr: " << std::hex << addr << ": ";
    for (auto req : req_q)  {
      std::cout << req << "; ";
    }
    std::cout << std::endl;
  }

  std::cout << "ramu resp q" << std::endl;
  for (auto req : ramu_resp_queue) {
    std::cout << std::hex << req->m_addr << "; ";
  }
  std::cout << std::endl;

  std::cout << "cme resp q" << std::endl;
  for (auto req : cme_resp_queue) {
    std::cout << std::hex << req->m_addr << "; ";
  }
  std::cout << std::endl;
}

void dram_ramulator_c::run_a_cycle(bool lock) {
  if (*KNOB(KNOB_DEBUG_IO_SYS)) {
    print_req();
#ifdef CXL
    m_simBase->m_ioctrl->m_cme->print_cxlt3_info();
    m_simBase->m_ioctrl->m_rc->print_rc_info();
#endif
  }

  send();
  wrapper->tick();
  receive();
  ++m_cycle;
}

void dram_ramulator_c::readComplete(ramulator::Request &ramu_req) {
  DEBUG("Read to 0x%lx completed.\n", ramu_req.addr);
  auto &req_q = ramu_reads.find(ramu_req.addr)->second;
  mem_req_s *req = req_q.front();
  req_q.pop_front();
  if (!req_q.size()) ramu_reads.erase(ramu_req.addr);

  // added counter to track requests in flight
  --ramu_requestsInFlight;

  DEBUG("Queuing response for address 0x%lx\n", ramu_req.addr);
  ramu_resp_queue.push_back(req);
}

void dram_ramulator_c::writeComplete(ramulator::Request &ramu_req) {
  DEBUG("Write to 0x%lx completed.\n", ramu_req.addr);
  auto &req_q = ramu_writes.find(ramu_req.addr)->second;
  mem_req_s *req = req_q.front();
  req_q.pop_front();
  if (!req_q.size()) ramu_writes.erase(ramu_req.addr);

  // added counter to track requests in flight
  --ramu_requestsInFlight;

  // in case of WB, retire requests here
  DEBUG("Retiring request for address 0x%lx\n", ramu_req.addr);
  MEMORY->free_req(req->m_core_id, req);
}

void dram_ramulator_c::send(void) {
  send_ramu_req();
  send_cme_req();
}

void dram_ramulator_c::send_ramu_req() {
  if (ramu_resp_queue.empty()) return;

  for (auto i = ramu_resp_queue.begin(); i != ramu_resp_queue.end(); ++i) {
    mem_req_s *req = *i;
    req->m_msg_type = NOC_FILL;
    if (NETWORK->send(req, MEM_MC, m_id, MEM_LLC, req->m_cache_id[MEM_LLC])) {
      DEBUG("Response to 0x%llx sent. req:%d\n", req->m_addr, req->m_id);
      ramu_resp_queue.pop_front();

      if (*KNOB(KNOB_BUG_DETECTOR_ENABLE) && *KNOB(KNOB_ENABLE_NEW_NOC)) {
        m_simBase->m_bug_detector->allocate_noc(req);
      }
    } else {
      DEBUG("Tried to send response to 0x%llx - NOC busy\n", req->m_addr);
      break;
    }
  }
}

void dram_ramulator_c::send_cme_req() {
#ifdef CXL
  // returned CME requests
  while (1) {
    mem_req_s* req = root_complex->pop_request();
    if (!req) {
      break;
    } else {
      cme_resp_queue.push_back(req);
      cme_requestsInFlight--;
    }
  }

  if (cme_resp_queue.empty()) return;

  for (auto i = cme_resp_queue.begin(); i != cme_resp_queue.end(); ++i) {
    mem_req_s *req = *i;
    req->m_msg_type = NOC_FILL;
    if (NETWORK->send(req, MEM_MC, m_id, MEM_LLC, req->m_cache_id[MEM_LLC])) {
      DEBUG("Response to 0x%llx sent. req:%d\n", req->m_addr, req->m_id);
      cme_resp_queue.pop_front();

      if (*KNOB(KNOB_BUG_DETECTOR_ENABLE) && *KNOB(KNOB_ENABLE_NEW_NOC)) {
        m_simBase->m_bug_detector->allocate_noc(req);
      }
    } else {
      DEBUG("Tried to send response to 0x%llx - NOC busy\n", req->m_addr);
      break;
    }
  }
#endif
}

void dram_ramulator_c::receive(void) {
  mem_req_s *req = NETWORK->receive(MEM_MC, m_id);
  if (!req) return;

  long addr = static_cast<long>(req->m_addr);
  if ((addr > *KNOB(KNOB_CME_RANGE)) && *KNOB(KNOB_CME_ENABLE)) {
    receive_cme_req(req);
  } else {
    receive_ramu_req(req);
  }
}

void dram_ramulator_c::receive_ramu_req(mem_req_s* req) {
  bool is_write = (req->m_type == MRT_WB);
  auto req_type = (is_write) ? ramulator::Request::Type::WRITE
                             : ramulator::Request::Type::READ;
  auto cb_func = (is_write) ? write_cb_func : read_cb_func;
  long addr = static_cast<long>(req->m_addr);

  ramulator::Request ramu_req(addr, req_type, cb_func, req->m_core_id);
  bool accepted = wrapper->send(ramu_req);

  if (accepted) {
    if (is_write) {
      ramu_writes[ramu_req.addr].push_back(req);
      DEBUG("Write to 0x%lx accepted and served. req:%d\n", ramu_req.addr,
            req->m_id);
    } else {
      ramu_reads[ramu_req.addr].push_back(req);
      DEBUG("Read to 0x%lx accepted. req:%d\n", ramu_req.addr, req->m_id);
    }

    // added counter to track requests in flight
    ++ramu_requestsInFlight;

    NETWORK->receive_pop(MEM_MC, m_id);
    if (*KNOB(KNOB_BUG_DETECTOR_ENABLE)) {
      m_simBase->m_bug_detector->deallocate_noc(req);
    }
  } else {
    if (is_write) {
      DEBUG("Write to 0x%lx NOT accepted. req:%d\n", ramu_req.addr, req->m_id);
    } else {
      DEBUG("Read to 0x%lx NOT accepted. req:%d\n", ramu_req.addr, req->m_id);
    }
  }
}

void dram_ramulator_c::receive_cme_req(mem_req_s* req) {
#ifdef CXL
  pcie_rc_c* root_complex = m_simBase->m_ioctrl->m_rc;
  root_complex->insert_request(req);

  // added counter to track requests in flight
  ++cme_requestsInFlight;

  NETWORK->receive_pop(MEM_MC, m_id);
  if (*KNOB(KNOB_BUG_DETECTOR_ENABLE)) {
    m_simBase->m_bug_detector->deallocate_noc(req);
  }
#endif
}

////////////////////////////////////////////////////////////////////////////////
// wrapper functions to allocate dram controller object

dram_c *ramulator_controller(macsim_c *simBase) {
  return new dram_ramulator_c(simBase);
}

#endif  // RAMULATOR
