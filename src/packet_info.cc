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
 * File         : packet_info.cc
 * Author       : Joonho
 * Date         : 10/08/2021
 * SVN          : $Id: packet_info.h,v 1.5 2008-09-17 21:01:41 kacear Exp $:
 * Description  : PCIe packet information
 *********************************************************************************************/

#include <iostream>

#include "packet_info.h"
#include "memreq_info.h"

message_s::message_s(macsim_c* simBase) {
  init();
  m_simBase = simBase;
}

void message_s::init(void) {
  m_id = 0;
  m_txtrans_end = 0;
  m_rxtrans_end = 0;
  m_vc_id = -1;
  m_req = NULL;
}

void message_s::print(void) {
  Addr addr = m_req ? m_req->m_addr : 0x00;
  
  std::cout << "= <MSG> addr: " << std::hex << addr
                << " channel: " << std::dec << m_vc_id << std::endl;
}

//////////////////////////////////////////////////////////////////////////////

flit_s::flit_s(macsim_c* simBase) {
  init();
  m_simBase = simBase;
}

void flit_s::init(void) {
  m_id = 0;
  m_bits = 0;
  m_txdll_end = 0;
  m_phys_end = 0;
  m_rxdll_end = 0;
  m_msgs.clear();
}

void flit_s::print(void) {
  std::cout << "====== <FLIT> " << std::endl;
  for (auto msg : m_msgs) {
    msg->print();
  }
}

//////////////////////////////////////////////////////////////////////////////