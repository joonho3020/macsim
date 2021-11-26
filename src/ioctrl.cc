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
 * File         : ioctrl.cc
 * Author       : Joonho
 * Date         : 10/10/2021
 * SVN          : $Id: ioctrl.cc 867 2009-11-05 02:28:12Z kacear $:
 * Description  : IO domain
 *********************************************************************************************/

#ifdef CXL

#include "ioctrl.h"
#include "pcie_rc.h"
#include "cxl_t3.h"
#include "packet_info.h"
#include "utils.h"
#include "all_knobs.h"

ioctrl_c::ioctrl_c(macsim_c* simBase) {
  // simulation related
  m_simBase = simBase;
  m_cycle = 0;

  // memory pool for packets
  m_msg_pool = new pool_c<message_s>;
  m_flit_pool = new pool_c<flit_s>;

  // io devices
  m_rc = new pcie_rc_c(simBase);
  m_cme = new cxlt3_c(simBase);
}

ioctrl_c::~ioctrl_c() {
  delete m_rc;
  delete m_cme;
  delete m_msg_pool;
  delete m_flit_pool;
}

void ioctrl_c::initialize() {
  m_rc->init(0, true, m_msg_pool, m_flit_pool, m_cme);
  m_cme->init(1, false, m_msg_pool, m_flit_pool, m_rc);
}

void ioctrl_c::run_a_cycle(bool pll_locked) {
  m_cme->run_a_cycle(pll_locked);
  m_rc->run_a_cycle(pll_locked);
  m_cycle++;

  if (*KNOB(KNOB_DEBUG_IO_SYS)) {
    std::cout << std::endl;
    std::cout << "io cycle : " << std::dec << m_cycle << std::endl;
    // m_simBase->m_dram_controller[0]->print_req();
    m_cme->print_cxlt3_info();
    m_rc->print_rc_info();
  }
}
#endif // CXL
