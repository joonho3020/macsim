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
}

mxp_wrapper_c::~mxp_wrapper_c() {
  delete m_cxlsim;
}

void mxp_wrapper_c::init(int argc, char **argv) {
  m_cxlsim = new cxlsim::cxlsim_c();
  m_cxlsim->init(argc, argv);

  cxlsim::callback_t *memreq_done_callback
    = new cxlsim::Callback<mxp_wrapper_c, void, Addr, bool, void*>(&(*this), &mxp_wrapper_c::mxp_memreq_callback);

  cxlsim::callback_t *uopreq_done_callback
    = new cxlsim::Callback<mxp_wrapper_c, void, Addr, bool, void*>(&(*this), &mxp_wrapper_c::mxp_uopreq_callback);

  m_cxlsim->register_memreq_callback(memreq_done_callback);
  m_cxlsim->register_uopreq_callback(uopreq_done_callback);
}

void mxp_wrapper_c::run_a_cycle(bool pll_locked) {
  m_cxlsim->run_a_cycle(pll_locked);
}

bool mxp_wrapper_c::insert_mem_request(Addr addr, bool write, void* req) {
  return m_cxlsim->insert_mem_request(addr, write, (void*)req);
}

bool mxp_wrapper_c::insert_uop_request(void* req, int core_id, int uop_type, 
                    int mem_type, Addr addr, Counter unique_id, int latency, 
                    std::vector<std::pair<Counter, int>> src_uop_list) {
  return m_cxlsim->insert_uop_request(req, core_id, uop_type, mem_type, addr, 
                                      unique_id, latency, src_uop_list);
}

void mxp_wrapper_c::mxp_memreq_callback(Addr addr, bool write, void* req) {
  if (req != NULL) m_done_memreqs.push_back(req);
}

void mxp_wrapper_c::mxp_uopreq_callback(Addr addr, bool write, void* req) {
  if (req != NULL) m_done_uopreqs.push_back(req);
}

void* mxp_wrapper_c::pull_done_memreqs() {
  if (m_done_memreqs.empty()) {
    return NULL;
  } else {
    void* req = m_done_memreqs.front();
    m_done_memreqs.pop_front();
    return req;
  }
}

void* mxp_wrapper_c::pull_done_uopreqs() {
  if (m_done_uopreqs.empty()) {
    return NULL;
  } else {
    void* req = m_done_uopreqs.front();
    m_done_uopreqs.pop_front();
    return req;
  }
}

void mxp_wrapper_c::print() {
  m_cxlsim->m_mxp->print_cxlt3_uops();
}

} // namespace cxlsim

#endif //CXL 
