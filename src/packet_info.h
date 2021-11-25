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

///////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Memory request state
///////////////////////////////////////////////////////////////////////////////////////////////
typedef enum Pkt_State_enum {
  PKT_INVAL,
  PKT_TVC,
  PKT_RVC,
  PKT_PHY,
  PKT_DLL
} Pkt_State;

///////////////////////////////////////////////////////////////////////////////////////////////
/// \brief packet request type
///////////////////////////////////////////////////////////////////////////////////////////////
typedef enum Pkt_Type_enum {
  PKT_NONE,
  PKT_MRD,
  PKT_MWR,
  PKT_CPLD,
  PKT_FCTRL,
  PKT_ACK,
  PKT_NAK
} Pkt_Type;


///////////////////////////////////////////////////////////////////////////////////////////////
/// \brief pcie packet data structure
///////////////////////////////////////////////////////////////////////////////////////////////
typedef struct msg_s {
  /**
   * Constructor
   */
  msg_s(macsim_c* simBase);
  void init(void);

  int m_id; /**< unique request id */
  int m_bytes; /**< bytes of a packet */
  Counter m_logic_end; /**< logic layer end cycle */
  Counter m_phys_start; /**< physical layer start cycle */
  Counter m_rxlogic_finished;  /**< rxlogic finished cycle */
  bool m_done; /**< packet transfer done */
  int m_vc_id; /**< VC id */
  int m_credits; /**< credits for flow ctrl */
  // int m_pkt_src; /**< packet source endpoint */
  // int m_pkt_dst; /**< packet destination endpoint */
  Pkt_Type m_pkt_type; /**< packet type */
  Pkt_State m_pkt_state; /**< packet state */
  mem_req_s* m_req; /**< packet may be a result of mem request */
  macsim_c* m_simBase; /**< reference to macsim base class for sim globals */
} msg_s;

#endif /* #ifndef MEMORY_H_INCLUDED  */
