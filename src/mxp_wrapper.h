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
 * File         : mxp_wrapper.h
 * Author       : Joonho
 * Date         : 12/04/2021
 * Description  : MXP interface
 *********************************************************************************************/

#ifndef MXP_WRAPPER_H
#define MXP_WRAPPER_H

#ifdef CXL

#include <list>

#include "CXLSim/src/cxlsim.h"

namespace cxlsim {

class mxp_wrapper_c 
{
public:
  mxp_wrapper_c();
  ~mxp_wrapper_c();

  void init(int argc, char** argv);

  void run_a_cycle(bool pll_locked);

  bool insert_mem_request(Addr addr, bool write, void* req);

  bool insert_uop_request(void* req, int uop_type, int mem_type,
                          Addr addr, Counter unique_id, int latency,
                          std::vector<std::pair<Counter, int>> src_uop_list);

  void mxp_memreq_callback(Addr addr, bool write, void* mem_req);

  void mxp_uopreq_callback(Addr addr, bool write, void* uop);

  void* pull_done_memreqs(void);
  void* pull_done_uopreqs(void);

public:
  Counter m_cycle;
  cxlsim::cxlsim_c* m_cxlsim;

private:
  std::list<void*> m_done_memreqs;
  std::list<void*> m_done_uopreqs;
};

} // namespace cxlsim


#endif //CXL

#endif //MXP_WRAPPER_H
