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

#include <list>
#include <deque>

#include "packet_info.h"
#include "macsim.h"

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
  void init(int id, pool_c<packet_info_s>* pkt_pool, pcie_ep_c* peer);

  /**
   * Tick a cycle
   */
  virtual void run_a_cycle(bool pll_locked);

  /**
   * Receive packet from transmit side & put in rx physical q
   */
  bool insert_rxphys(packet_info_s* pkt);

  /**
   * Print for debugging
   */
  void print_ep_info();

private:
  pcie_ep_c(); // do not implement

  /**
   * Gets cycles required to transfer the packet over physical layer
   */
  Counter get_phys_latency(packet_info_s* pkt);

  /**
   * Updates how much credits it has
   */
  void decrease_credit(packet_info_s* pkt);

  /**
   * Updates how much credits it has by received flow ctrl packet
   */
  void update_credit(packet_info_s* fctrl_pkt);

  /**
   * Simple updates on credits 
   */
  void update_credit(int vc_id, int credit);

  /**
   * Checks it has enough credits to send a TLP packet
   */
  bool check_credit(packet_info_s* pkt);

  /**
   * Checks it physical layer can accept new entries into the queue
   */
  bool phys_layer_full(bool tx);

  /**
   * Inserts a TLP to physical layer
   */
  void insert_tx_phys(packet_info_s* pkt, bool front);
  
protected:
  /**
   * Start PCIe transaction by inserting requests
   */
  virtual void start_transaction();

  /**
   * End PCIe transaction by pulling requests
   */
  virtual void end_transaction();

  /**
   * Choose a packet from VC or DLLP & sends it to physical TX
   */
  void process_txlogic();

  /**
   * Sends a packet over the physical layer
   */
  void process_txphys();

  /**
   * Receives a packet from the physical RX & stores it in RX VC buffer
   */
  void process_rxphys();

  /**
   * Receives a packet from the physical RX & processes it
   */
  void process_rxlogic();

  /**
   * Push request to TX VC buffer
   */
  bool push_txvc(mem_req_s* mem_req);

  /**
   * Pull request from RX VC buffer
   */
  mem_req_s* pull_rxvc();

  /**
   * Initialize a new packet
   */
  void init_new_pkt(packet_info_s* pkt, int bytes, int vc_id, int credits,
    Pkt_Type pkt_type, Pkt_State pkt_state, mem_req_s* req);

  /**
   * Insert packet to VC buffer
   */
  void insert_vc_buff(int vc_id, int* size, list<packet_info_s*> *buff,
      packet_info_s* pkt);

  /**
   * Pull packet from VC buffer
   */
  packet_info_s* pull_vc_buffer(int vc_id, int* size, 
      list<packet_info_s*> *buff);

public:
  static int m_unique_id; /**< unique packet id */

private:
  int m_id; /**< unique id of each endpoint */
  int m_memreq_size; /**< size of mem_req_s in bytes */
  pool_c<packet_info_s>* m_pkt_pool; /**< packet pool */
  int m_rr_idx; /**< index used for RR policy */
  int m_lanes; /**< PCIe lanes connected to endpoint */
  float m_perlane_bw; /**< PCIe per lane BW in GB (cycles to send 1B) */
  Counter m_prev_txphys_cycle; /**< finish cycle of previously sent packet */
  pcie_ep_c* m_peer_ep; /**< endpoint connected to this endpoint */

  int m_vc_cnt; /**< VC number */
  int m_vc_cap; /**< VC buffer capacity */
  int* m_txvc_size; /**< remaining space of TX VC */
  int* m_rxvc_size; /**< remaining space of RX VC */
  list<packet_info_s*>* m_txvc_buff; /**< buffer of TX VC */
  list<packet_info_s*>* m_rxvc_buff; /**< buffer of RX VC */

  int m_credit_cap; /**< initial credit of each VC */
  int* m_credit; /**< remaining RX VC buff of pair endpoint */

  int m_phys_cap; /**< maximum numbers of packets in physical layer q */
  int m_txphys_size; /**< remaining TX phys q entries */
  int m_rxphys_size; /**< remaining RX phys q entries */
  deque<packet_info_s*>* m_txphys_q; /**< physical layer send queue */
  deque<packet_info_s*>* m_rxphys_q; /**< physical layer receive queue */

protected:
  macsim_c* m_simBase; /**< simulation base */
  Counter m_cycle; /**< PCIe clock cycle */
};

#endif //PCIE_EP_H
