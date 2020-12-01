/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2020 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#ifndef SRSENB_SCHEDULER_H
#define SRSENB_SCHEDULER_H

#include "sched_grid.h"
#include "sched_harq.h"
#include "sched_ue.h"
#include "srslte/common/log.h"
#include "srslte/interfaces/enb_interfaces.h"
#include "srslte/interfaces/sched_interface.h"
#include <map>
#include <mutex>
#include <pthread.h>
#include <queue>

namespace srsenb {

namespace sched_utils {

inline bool is_in_tti_interval(uint32_t tti, uint32_t tti1, uint32_t tti2)
{
  tti %= 10240;
  tti1 %= 10240;
  tti2 %= 10240;
  if (tti1 <= tti2) {
    return tti >= tti1 and tti <= tti2;
  }
  return tti >= tti1 or tti <= tti2;
}

} // namespace sched_utils

/* Caution: User addition (ue_cfg) and removal (ue_rem) are not thread-safe
 * Rest of operations are thread-safe
 *
 * The subclass sched_ue is thread-safe so that access to shared variables like buffer states
 * from scheduler thread and other threads is protected for each individual user.
 */

class sched : public sched_interface
{
public:
  /*************************************************************
   *
   * FAPI-like Interface
   *
   ************************************************************/

  sched();
  ~sched() override;

  void init(rrc_interface_mac* rrc);
  int  cell_cfg(const std::vector<cell_cfg_t>& cell_cfg) override;
  void set_sched_cfg(sched_args_t* sched_cfg);
  int  reset() final;

  int  ue_cfg(uint16_t rnti, const ue_cfg_t& ue_cfg) final;
  int  ue_rem(uint16_t rnti) final;
  bool ue_exists(uint16_t rnti) final;

  void phy_config_enabled(uint16_t rnti, bool enabled);

  int bearer_ue_cfg(uint16_t rnti, uint32_t lc_id, ue_bearer_cfg_t* cfg) final;
  int bearer_ue_rem(uint16_t rnti, uint32_t lc_id) final;

  uint32_t get_ul_buffer(uint16_t rnti) final;
  uint32_t get_dl_buffer(uint16_t rnti) final;

  int dl_rlc_buffer_state(uint16_t rnti, uint32_t lc_id, uint32_t tx_queue, uint32_t retx_queue) final;
  int dl_mac_buffer_state(uint16_t rnti, uint32_t ce_code, uint32_t nof_cmds = 1) final;

  int dl_ack_info(uint32_t tti, uint16_t rnti, uint32_t enb_cc_idx, uint32_t tb_idx, bool ack) final;
  int dl_rach_info(uint32_t enb_cc_idx, dl_sched_rar_info_t rar_info) final;
  int dl_ri_info(uint32_t tti, uint16_t rnti, uint32_t enb_cc_idx, uint32_t ri_value) final;
  int dl_pmi_info(uint32_t tti, uint16_t rnti, uint32_t enb_cc_idx, uint32_t pmi_value) final;
  int dl_cqi_info(uint32_t tti, uint16_t rnti, uint32_t enb_cc_idx, uint32_t cqi_value) final;
  int ul_crc_info(uint32_t tti, uint16_t rnti, uint32_t enb_cc_idx, bool crc) final;
  int ul_sr_info(uint32_t tti, uint16_t rnti) override;
  int ul_bsr(uint16_t rnti, uint32_t lcg_id, uint32_t bsr) final;
  int ul_phr(uint16_t rnti, uint32_t enb_cc_idx, int phr) final;
  int ul_snr_info(uint32_t tti, uint16_t rnti, uint32_t enb_cc_idx, float snr, uint32_t ul_ch_code) final;

  int dl_sched(uint32_t tti, uint32_t enb_cc_idx, dl_sched_res_t& sched_result) final;
  int ul_sched(uint32_t tti, uint32_t enb_cc_idx, ul_sched_res_t& sched_result) final;

  /* Custom functions
   */
  void                                  set_dl_tti_mask(uint8_t* tti_mask, uint32_t nof_sfs) final;
  std::array<int, SRSLTE_MAX_CARRIERS>  get_enb_ue_cc_map(uint16_t rnti) final;
  std::array<bool, SRSLTE_MAX_CARRIERS> get_scell_activation_mask(uint16_t rnti) final;
  int                                   ul_buffer_add(uint16_t rnti, uint32_t lcid, uint32_t bytes) final;

  class carrier_sched;

protected:
  void new_tti(srslte::tti_point tti_rx);
  bool is_generated(srslte::tti_point, uint32_t enb_cc_idx) const;
  // Helper methods
  template <typename Func>
  int ue_db_access(uint16_t rnti, Func, const char* func_name = nullptr);

  // args
  srslte::log_ref                  log_h;
  rrc_interface_mac*               rrc       = nullptr;
  sched_args_t                     sched_cfg = {};
  std::vector<sched_cell_params_t> sched_cell_params;

  std::map<uint16_t, sched_ue> ue_db;

  // independent schedulers for each carrier
  std::vector<std::unique_ptr<carrier_sched> > carrier_schedulers;

  // Storage of past scheduling results
  sched_result_list sched_results;

  srslte::tti_point last_tti;
  std::mutex        sched_mutex;
  bool              configured = false;
};

} // namespace srsenb

#endif // SRSENB_SCHEDULER_H
