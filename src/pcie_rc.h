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
 * File         : pcie_rc.h
 * Author       : Joonho
 * Date         : 10/10/2021
 * SVN          : $Id: pcie_rc.h 867 2009-11-05 02:28:12Z kacear $:
 * Description  : PCIe root complex
 *********************************************************************************************/

#ifndef PCIE_RC_H
#define PCIE_RC_H

#ifdef CXL

#include <list>

#include "pcie_endpoint.h"
#include "memreq_info.h"

class pcie_rc_c : public pcie_ep_c 
{
public:
  /**
   * Constructor
   */
  pcie_rc_c(macsim_c* simBase);
  
  /**
   * Destructor
   */
  ~pcie_rc_c();

  /**
   * Tick a cycles
   */
  void run_a_cycle(bool pll_lock);

  /**
   * Insert pcie request into pending queue
   */
  void insert_request(mem_req_s* req);

  /**
   * Pop a finished pcie request
   */
  mem_req_s* pop_request();

  /**
   * Print for debugging
   */
  void print_rc_info();

private:
  /**
   * Start PCIe transaction by inserting requests
   */
  void start_transaction() override;

  /**
   * End PCIe transaction by pulling requests
   */
  void end_transaction() override;

private:
  list<mem_req_s*>* m_pending_req; /**< requests pending */
  list<mem_req_s*>* m_done_req;    /**< requests finished */
};

#endif // CXL
#endif // PCIE_RC_H
