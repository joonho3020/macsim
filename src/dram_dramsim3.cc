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
 * File         : dram_dramsim3.cc
 * Author       : Joonho
 * Date         : 8/10/2021
 * SVN          : $Id: dram.h 867 2009-11-05 02:28:12Z kacear $:
 * Description  : DRAMSim3 interface
 *********************************************************************************************/

#ifdef DRAMSIM3
#include "dramsim3.h"

#include "dram_dramsim3.h"
#include "assert_macros.h"
#include "debug_macros.h"
#include "bug_detector.h"
#include "memory.h"
#include "ioctrl.h"
#include "pcie_rc.h"

#include "all_knobs.h"
#include "statistics.h"

#include <string>

#undef DEBUG
#define DEBUG(args...) _DEBUG(*m_simBase->m_knobs->KNOB_DEBUG_DRAM, ##args)

using namespace dramsim3;

/** FIXME
 * How to handle redundant requests
 * Fix .ini file and output directories
 */

dram_dramsim3_c::dram_dramsim3_c(macsim_c* simBase) : dram_c(simBase) {
  m_output_buffer = new list<mem_req_s*>;
  m_tmp_output_buffer = new list<mem_req_s*>;
  m_pending_request = new list<mem_req_s*>;

  std::string config_file = *KNOB(KNOB_DRAMSIM3_CONFIG);
  std::string working_dir = *KNOB(KNOB_DRAMSIM3_OUT);
  m_dramsim = new MemorySystem(config_file, working_dir, NULL, NULL);

  std::function<void(uint64_t)> read_cb;
  std::function<void(uint64_t)> write_cb;
  read_cb = 
    std::bind(&dram_dramsim3_c::read_callback, this, 0, std::placeholders::_1);
  write_cb = 
    std::bind(&dram_dramsim3_c::write_callback, this, 0, std::placeholders::_1);
  m_dramsim->RegisterCallbacks(read_cb, write_cb);
}

dram_dramsim3_c::~dram_dramsim3_c() {
  delete m_output_buffer;
  delete m_tmp_output_buffer;
  delete m_pending_request;
  delete m_dramsim;
}

void dram_dramsim3_c::print_req(void) {
}

void dram_dramsim3_c::init(int id) {
  m_id = id;
}

void dram_dramsim3_c::run_a_cycle(bool pll_lock) {
  if (pll_lock) {
    ++m_cycle;
    return;
  }

  send();
  m_dramsim->ClockTick();
  cme_schedule();
  receive();
  ++m_cycle;
}

void dram_dramsim3_c::read_callback(unsigned id, uint64_t address) {
  // find requests with this address
  auto I = m_pending_request->begin();
  auto E = m_pending_request->end();
  while (I != E) {
    mem_req_s* req = (*I);
    ++I;

    if (req->m_addr == address) {
      if (*KNOB(KNOB_DRAM_ADDITIONAL_LATENCY)) {
        req->m_rdy_cycle = m_cycle + *KNOB(KNOB_DRAM_ADDITIONAL_LATENCY);
        m_tmp_output_buffer->push_back(req);
      } else {
        m_output_buffer->push_back(req);
      }
      m_pending_request->remove(req);

      // return first request with matching address
      // this may not necessarily be true but this is the best we can do
      // at this point
      break;
    }
  }
}

void dram_dramsim3_c::write_callback(unsigned id, uint64_t address) {
  // find requests with this address
  auto I = m_pending_request->begin();
  auto E = m_pending_request->end();
  while (I != E) {
    mem_req_s* req = (*I);
    ++I;

    if (req->m_addr == address) {
      // in case of WB, retire requests here
      MEMORY->free_req(req->m_core_id, req);
      m_pending_request->remove(req);

      // return first request with matching address
      // this may not necessarily be true but this is the best we can do
      // at this point
      break;
    }
  }
}

void dram_dramsim3_c::receive(void) {
  mem_req_s* req = NETWORK->receive(MEM_MC, m_id);
  if (!req) return;

  if (req && insert_new_req(req)) {
    NETWORK->receive_pop(MEM_MC, m_id);
    if (*KNOB(KNOB_BUG_DETECTOR_ENABLE)) {
      m_simBase->m_bug_detector->deallocate_noc(req);
    }

    STAT_EVENT(TOTAL_DRAM);

    // Get address range (Debug purpose)
    if (m_accessed_addr.find(req->m_addr) == m_accessed_addr.end()) {
      m_accessed_addr.insert(req->m_addr);
    }
  }
}

void dram_dramsim3_c::send(void) {
  vector<mem_req_s*> temp_list;
  vector<mem_req_s*> cme_temp_list;

  // take care of DIMM requests
  for (auto I = m_tmp_output_buffer->begin(), E = m_tmp_output_buffer->end();
       I != E; ++I) {
    mem_req_s* req = *I;
    if (req->m_rdy_cycle <= m_cycle) {
      temp_list.push_back(req);
      m_output_buffer->push_back(req);
    } else {
      break;
    }
  }

  for (auto itr = temp_list.begin(), end = temp_list.end(); itr != end; ++itr) {
    m_tmp_output_buffer->remove((*itr));
  }

  for (auto I = m_output_buffer->begin(), E = m_output_buffer->end(); I != E;
       ++I) {
    mem_req_s* req = (*I);
    req->m_msg_type = NOC_FILL;
    bool insert_packet =
      NETWORK->send(req, MEM_MC, m_id, MEM_LLC, req->m_cache_id[MEM_LLC]);

    if (!insert_packet) {
      DEBUG("MC[%d] req:%d addr:0x%llx type:%s noc busy\n", m_id, req->m_id,
            req->m_addr, mem_req_c::mem_req_type_name[req->m_type]);
      break;
    }

    temp_list.push_back(req);
    if (*KNOB(KNOB_BUG_DETECTOR_ENABLE) && *KNOB(KNOB_ENABLE_NEW_NOC)) {
      m_simBase->m_bug_detector->allocate_noc(req);
    }
  }

  for (auto I = temp_list.begin(), E = temp_list.end(); I != E; ++I) {
    m_output_buffer->remove((*I));
  }

  // take care of CME requests
  for (auto I = m_cmeout_buffer->begin(), E = m_cmeout_buffer->end(); I != E;
      ++I) {
    mem_req_s* req = (*I);
    req->m_state = CME_NOC_DONE;
    req->m_msg_type = NOC_FILL;

    bool insert_packet = 
      NETWORK->send(req, MEM_MC, m_id, MEM_LLC, req->m_cache_id[MEM_LLC]);

    if (!insert_packet) {
      DEBUG("MC[%d] req:%d addr:0x%llx type:%s noc busy\n", m_id, req->m_id,
          req->m_addr, mem_req_c::mem_req_type_name[req->m_type]);
      break;
    }

    cme_temp_list.push_back(req);
    if (*KNOB(KNOB_BUG_DETECTOR_ENABLE) && *KNOB(KNOB_ENABLE_NEW_NOC)) {
      m_simBase->m_bug_detector->allocate_noc(req);
    }
  }

  for (auto I = cme_temp_list.begin(), E = cme_temp_list.end(); I != E; ++I) {
    m_cmeout_buffer->remove((*I));
  }
}

  // TODO : sophisticated interleaving policy is required
bool dram_dramsim3_c::insert_new_req(mem_req_s* mem_req) {
  Addr addr = mem_req->m_addr;
  bool is_write = (mem_req->m_type == MRT_WB);

  // insert to CME
  if (addr >= *KNOB(KNOB_CME_RANGE) && *KNOB(KNOB_CME_ENABLE)) {
    if (m_cme_free_list->empty()) {
      return false;
    } else {
      cme_entry_s* new_entry = m_cme_free_list->front();
      m_cme_free_list->pop_front();

      new_entry->set(mem_req, m_simBase->m_core_cycle[0]);
      m_cmein_buffer->push_back(new_entry);
      mem_req->m_state = CME_NOC_START;
      return true;
    }
  }
  // insert to DIMM
  else {
    uint64_t addr_ = static_cast<uint64_t>(addr);

    bool will_accept = m_dramsim->WillAcceptTransaction(addr_, is_write);
    if (will_accept) {
      m_dramsim->AddTransaction(addr_, is_write);
      m_pending_request->push_back(mem_req);
      return true;
    } else {
      return false;
    }
  }
}

void dram_dramsim3_c::cme_schedule() {
  pcie_rc_c* rc = m_simBase->m_ioctrl->m_rc;
  vector<cme_entry_s*> tmp_list;

  // incoming CME requests
  for (auto I = m_cmein_buffer->begin(), E = m_cmein_buffer->end(); I != E; 
      ++I) {
    mem_req_s* req = (*I)->m_req;
    rc->insert_request(req);
    m_cmepend_buffer->push_back(req);
    tmp_list.push_back(*I);
  }

  for (auto I = tmp_list.begin(), E = tmp_list.end(); I != E; ++I) {
    m_cmein_buffer->remove((*I));
    STAT_EVENT(TOTAL_DRAM_MERGE);
    (*I)->reset();
    m_cme_free_list->push_back((*I));
  }

  // returned CME requests
  while (1) {
    mem_req_s* req = rc->pop_request();
    if (!req) {
      break;
    }

    m_cmeout_buffer->push_back(req);
    m_cmepend_buffer->remove(req);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////
// wrapper functions to allocate dram controller object

dram_c* dramsim3_controller(macsim_c* simBase) {
  return new dram_dramsim3_c(simBase);
}

#endif
