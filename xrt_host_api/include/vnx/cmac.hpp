// Copyright (C) 2022 Xilinx, Inc
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <experimental/xrt_ip.h>
#include <map>
#include <string>

namespace vnx {
constexpr char stat_tx_status_name[] = "stat_tx_status";
constexpr char stat_rx_status_name[] = "stat_rx_status";
  
typedef std::map<std::string, std::uint32_t> stats_t;

class CMAC {
public:
  CMAC(xrt::xclbin::ip &xclbin_ip, xrt::ip &cmac);
  CMAC(xrt::xclbin::ip &&xclbin_ip, xrt::ip &&cmac);

  /**
   * Retrieves the link status from the CMAC kernel.
   *
   * Contains boolean status for the following keys:
   * rx_status
   * rx_aligned
   * rx_misaligned
   * rx_aligned_err
   * rx_hi_ber
   * rx_remote_fault
   * rx_local_fault
   * rx_got_signal_os
   * tx_local_fault
   */
  std::map<std::string, bool> link_status();

  // TODO: implement this function
  stats_t statistics();

private:
  xrt::ip cmac;

  size_t stat_tx_status_address;
  size_t stat_rx_status_address;
  std::map<std::string, size_t> stat_debug_address_map;

  void fetch_register_offsets(xrt::xclbin::ip &xclbin_ip);

};
} // namespace vnx
