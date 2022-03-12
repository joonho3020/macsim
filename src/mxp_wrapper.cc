/*
Copyright (c) <2012>, <Seoul National University> All rights reserved.

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
 * File         : mxp_wrapper.cc
 * Author       : Joonho
 * Date         : 12/04/2021
 * Description  : MXP interface
 *********************************************************************************************/

#ifdef CXL 

#include <cassert>

#include "CXLSim/src/all_knobs.h"
#include "CXLSim/src/all_stats.h"
#include "CXLSim/src/assert_macros.h"
#include "CXLSim/src/Callback.h"
#include "CXLSim/src/cxl_t3.h"
#include "CXLSim/src/cxlsim.h"
#include "CXLSim/src/global_types.h"
#include "CXLSim/src/global_defs.h"
#include "CXLSim/src/knob.h"
#include "CXLSim/src/packet_info.h"
#include "CXLSim/src/pcie_endpoint.h"
#include "CXLSim/src/pcie_rc.h"
#include "CXLSim/src/ramulator_wrapper.h"
#include "CXLSim/src/statistics.h"
#include "CXLSim/src/statsEnums.h"
#include "CXLSim/src/utils.h"

#include "mxp_wrapper.h"


namespace cxlsim {

mxp_wrapper_c::mxp_wrapper_c() {
  cxlsim_c* m_cxlsim = NULL;
  m_in_flight_reqs = 0;
}

mxp_wrapper_c::~mxp_wrapper_c() {
  delete m_cxlsim;
}

void mxp_wrapper_c::init(int argc, char **argv) {
  m_cxlsim = new cxlsim::cxlsim_c();
  m_cxlsim->init(argc, argv);

  cxlsim::callback_t *done_callback
    = new cxlsim::Callback<mxp_wrapper_c, void, Addr, bool, Counter, void*>(&(*this), &mxp_wrapper_c::mxp_callback);

  m_cxlsim->register_callback(done_callback);
}

void mxp_wrapper_c::run_a_cycle(bool pll_locked) {
  m_cxlsim->run_a_cycle(pll_locked);
  m_cycle++;
}

Counter mxp_wrapper_c::insert_request(Addr addr, bool write, void* mem_req) {
  Counter req_id = m_cxlsim->insert_request(addr, write, (void*)mem_req);
  if (req_id) {
    m_in_flight_reqs++;
    m_in_flight_req_ids[addr][req_id]++;
  }
  return req_id;
}

void mxp_wrapper_c::mxp_callback(Addr addr, bool write, Counter req_id, void* req) {
  m_in_flight_reqs--;
  m_in_flight_req_ids[addr].erase(req_id);

  if (req != NULL) m_done_reqs.push_back(req);
}

void* mxp_wrapper_c::pull_done_reqs() {
  if (m_done_reqs.empty()) {
    return NULL;
  } else {
    void* req = m_done_reqs.front();
    m_done_reqs.pop_front();
    return req;
  }
}

void mxp_wrapper_c::print() {
  for (auto addrmap : m_in_flight_req_ids) {
    auto reqid_map = addrmap.second;
    if (reqid_map.size() == 0) continue;

    std::cout << "Addr: " << addrmap.first << ": ";

    for (auto reqid : reqid_map) {
      auto id = reqid.first;
      auto cnt = reqid.second;

      if (reqid.second > 0) {
        std::cout << id << " ";
      }
    }
    std::cout << std::endl;
  }
}

} // namespace cxlsim

#endif //CXL 
