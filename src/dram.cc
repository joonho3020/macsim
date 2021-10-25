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
 * File         : dram.cc
 * Author       : HPArch Research Group
 * Date         : 2/18/2013
 * SVN          : $Id: dram.h 867 2009-11-05 02:28:12Z kacear $:
 * Description  : Memory controller
 *********************************************************************************************/

#include <iostream>
#include <fstream>
#include "dram.h"

cme_entry_s::cme_entry_s(macsim_c *simBase) {
  reset();
  m_simBase = simBase;
}

void cme_entry_s::set(mem_req_s* req, Counter cur_tick) {
  m_req = req;
  m_start_req = cur_tick;
  m_cycles = 0;
}

void cme_entry_s::reset() {
  m_req = NULL;
  m_cycles = -1;
  m_start_req = -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////


dram_c::dram_c(macsim_c* simBase) : m_simBase(simBase) {
  // CME buffers
  m_cmein_buffer = new list<cme_entry_s*>;
  m_cmepend_buffer = new list<mem_req_s*>;
  m_cmeout_buffer = new list<mem_req_s*>;
  m_cme_free_list = new list<cme_entry_s*>;

  for (int ii = 0; ii < 30; ii++) {
    cme_entry_s* new_entry = new cme_entry_s(m_simBase);
    m_cme_free_list->push_back(new_entry);
  }
}

dram_c::~dram_c() {
  if (!m_accessed_addr.empty()) {
    std::cerr << "min access addr: " << 
      hex << *m_accessed_addr.begin() << std::endl;
    std::cerr << "max access addr:" << 
      hex << *m_accessed_addr.rbegin() << std::endl;
  } else {
    std::cerr << "No accessed memory" << std::endl;
  }
  ofstream address_file;
  string fname = "address_dist.txt";

  address_file.open(fname.c_str());
  if (address_file.is_open()) {
    for (auto iter : m_access_dist) {
      address_file << std::hex << iter.first << " " 
                   << std::dec << iter.second << std::endl;
    }
  }
  address_file.close();

  delete m_cmein_buffer;
  delete m_cmepend_buffer;
  delete m_cmeout_buffer;
  delete m_cme_free_list;
}

void dram_c::pop_pushedq(mem_req_s *req) {
  m_cmepend_buffer->remove(req);
}

