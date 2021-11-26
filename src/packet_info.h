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
 * File         : packet_info.h
 * Author       : Joonho
 * Date         : 10/8/2021
 * SVN          : $Id: packet_info.h,v 1.5 2008-09-17 21:01:41 kacear Exp $:
 * Description  : PCIe packet information
 *********************************************************************************************/

#ifndef PACKET_INFO_H
#define PACKET_INFO_H

#include <string>
#include <list>
#include <deque>
#include <functional>

#include "macsim.h"
#include "global_defs.h"
#include "global_types.h"

typedef struct message_s {
  /**
   * Constructor
   */
  message_s(macsim_c* simBase);
  void init(void);
  void print(void);

  int m_id; /**< unique request id */
  Counter m_txtrans_end;
  Counter m_rxtrans_end;  /**< rxlogic finished cycle */
  int m_vc_id; /**< VC id */
  mem_req_s* m_req; /**< packet may be a result of mem request */
  macsim_c* m_simBase; /**< reference to macsim base class for sim globals */
} message_s;

typedef struct flit_s {
  flit_s(macsim_c* simBase);
  void init(void);
  void print(void);

  int m_id;
  int m_bits;
  bool m_phys_sent;
  Counter m_txdll_end;
  Counter m_phys_end;
  Counter m_rxdll_end;
  list<message_s*> m_msgs;
  macsim_c* m_simBase;
} flit_s;

#endif /* #ifndef MEMORY_H_INCLUDED  */
