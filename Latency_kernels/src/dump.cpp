/*
 * Copyright 2022 Xilinx, Inc.
 *           2023-2024 Gerrit Pape (papeg@mail.upb.de)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <hls_stream.h>

#include "constants.h"

extern "C" {
void dump(ap_uint<PTR_WIDTH>* data_output, unsigned int byte_size,
          unsigned int iterations, bool ack_enable,
          hls::stream<pkt>& data_input, hls::stream<ack_pkt>& ack_stream) {
iterations:
    for (unsigned int n = 0; n < iterations; n++) {
    read:
        for (int i = 0; i < ((byte_size + (PTR_BYTE_WIDTH - 1)) / PTR_BYTE_WIDTH); i++) {
#pragma HLS PIPELINE II = 1
            pkt temp = data_input.read();
            data_output[i] = temp.data;
        }
        if (ack_enable) {
            ack_pkt ack;
            ack_stream.write(ack);
        }
    }
}
}
