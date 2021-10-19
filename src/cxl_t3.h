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

#ifndef CXLT3_H
#define CXLT3_H

#ifdef CXL
#ifdef DRAMSIM3
#include <list>

#include "macsim.h"
#include "dram.h"
#include "memreq_info.h"
#include "pcie_endpoint.h"
#include "packet_info.h"

namespace dramsim3{
  class MemorySystem;
};

class cxlt3_c : public pcie_ep_c
{
public:
  /**
   * Constructor
   */
  cxlt3_c(macsim_c* simBase);

  /**
   * Destructor
   */
  ~cxlt3_c();

  /**
   * Tick a cycle
   */
  void run_a_cycle(bool pll_locked);

  /*
   * Tick a internal cycle as CME has a faster internal clock frequency
   */
  void run_a_cycle_internal(bool pll_locked);

  /**
   * Print for debugging
   */
  void print_cxlt3_info();

private:
  /**
   * Start PCIe transaction by inserting requests
   */
  void start_transaction() override;

  /**
   * End PCIe transaction by pulling requests
   */
  void end_transaction() override;

  /**
   * Process pending memory requests
   */
  void process_pending_req();

  /**
   * DRAMSim3 read callback function
   */
  void read_callback(unsigned, uint64_t);

  /**
   * DRAMSim3 write callback function
   */
  void write_callback(unsigned, uint64_t);

private:
  list<mem_req_s*>* m_pending_req; /**< mem reqs pending */
  list<mem_req_s*>* m_pushed_req; /**< mem reqs pushed to dramsim */
  list<mem_req_s*>* m_done_req; /**< mem reqs returned from dramsim */
  list<mem_req_s*>* m_tmp_done_req; /**< mem reqs with additional latency */
  dramsim3::MemorySystem* m_dramsim; /**< DRAMSim3 */

  Counter m_cycle_internal; /**< internal cycle for DRAM */
};

#endif // DRAMSIM3
#endif // CXL

#endif //CXLT3_H
