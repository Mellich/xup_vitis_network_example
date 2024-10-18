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
void issue(ap_uint<PTR_WIDTH>* data_input, unsigned int byte_size,
           unsigned int frame_size, unsigned int iterations, bool ack_enable,
           unsigned int dest, hls::stream<pkt>& data_output,
           hls::stream<ack_pkt>& ack_stream) {
    const unsigned int num_iterations = ((byte_size + (PTR_BYTE_WIDTH - 1)) / PTR_BYTE_WIDTH);
    for (unsigned int n = 0; n < iterations; n++) {
        for (unsigned int i = 0; i < num_iterations; i++) {
#pragma HLS PIPELINE II = 1
            pkt temp;
            temp.data = data_input[i];
            temp.last = (i % frame_size == (frame_size - 1) || (i  == (num_iterations - 1)));
            temp.keep = -1;
            temp.dest = dest;
            data_output.write(temp);
       }
        if (ack_enable) {
            ack_pkt ack = ack_stream.read();
        }
    }
}
}
