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
 * File         : pcie_endpoint.h
 * Author       : Joonho
 * Date         : 8/10/2021
 * SVN          : $Id: cxl_t3.h 867 2009-11-05 02:28:12Z kacear $:
 * Description  : PCIe endpoint device
 *********************************************************************************************/

#ifndef PCIE_EP_H
#define PCIE_EP_H

#ifdef CXL

#include <list>
#include <deque>

#include "packet_info.h"
#include "macsim.h"

#define TX true
#define RX false

#define WOD_CHANNEL 0
#define WD_CHANNEL 1

class pcie_ep_c {
public:
  /**
   * Constructor
   */
  pcie_ep_c(macsim_c* simBase);

  /**
   * Destructor
   */
  ~pcie_ep_c();

  /**
   * Initialize PCIe endpoint
   */
  void init(int id, bool master, pool_c<message_s>* msg_pool, pcie_ep_c* peer);

  /**
   * Tick a cycle
   */
  virtual void run_a_cycle(bool pll_locked);

  /**
   * Receive packet from transmit side & put in rx physical q
   */
  bool insert_rxphys(message_s* pkt);

  bool check_peer_credit(message_s* pkt);

  /**
   * Print for debugging
   */
  void print_ep_info();

private:
  pcie_ep_c(); // do not implement

  /**
   * Gets cycles required to transfer the packet over physical layer
   */
  Counter get_phys_latency(message_s* pkt);

  bool dll_layer_full(bool tx);
  bool phys_layer_full(bool tx);

  void init_new_msg(message_s* pkt, int bits, int vc_id, mem_req_s* req);

  bool txvc_not_full(int channel);

protected:
  /**
   * Start PCIe transaction by inserting requests
   */
  virtual void start_transaction();

  /**
   * Push request to TX VC buffer
   */
  bool push_txvc(mem_req_s* mem_req);

  /**
   * End PCIe transaction by pulling requests
   */
  virtual void end_transaction();

  /**
   * Pull request from RX VC buffer
   */
  mem_req_s* pull_rxvc();

  // PCIE layer related
  void process_txtrans();
  void process_txdll();
  void process_txphys();

  void process_rxphys();
  void process_rxdll();
  void process_rxtrans();

public:
  static int m_unique_id; /**< unique packet id */

private:
  int m_id; /**< unique id of each endpoint */
  bool m_master;
  pool_c<message_s>* m_msg_pool; /**< packet pool */

  int m_lanes; /**< PCIe lanes connected to endpoint */
  float m_perlane_bw; /**< PCIe per lane BW in GB (cycles to send 1B) */
  Counter m_prev_txphys_cycle; /**< finish cycle of previously sent packet */

  int m_txvc_rr_idx;
  int m_vc_cnt; /**< VC number */
  int m_txvc_cap; /**< VC buffer capacity */
  int m_rxvc_cap; /**< VC buffer capacity */
  list<message_s*>* m_txvc_buff; /**< buffer of TX VC */
  list<message_s*>* m_rxvc_buff; /**< buffer of RX VC */

  int m_txdll_cap;
  list<message_s*> m_txdll_q;

  int m_phys_cap; /**< maximum numbers of packets in physical layer q */
  list<message_s*> m_txphys_q; /**< physical layer send queue */
  list<message_s*> m_rxphys_q; /**< physical layer receive queue */

public:
  pcie_ep_c* m_peer_ep; /**< endpoint connected to this endpoint */
  macsim_c* m_simBase; /**< simulation base */
  Counter m_cycle; /**< PCIe clock cycle */
};

#endif // CXL
#endif //PCIE_EP_H
