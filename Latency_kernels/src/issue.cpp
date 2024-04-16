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
void issue(hls::stream<pkt>& data_output, ap_uint<PTR_WIDTH>* data_input,
           unsigned int byte_size, unsigned int frame_size,
           unsigned int iterations, bool ack_enable, unsigned int dest,
           hls::stream<ack_pkt>& ack_stream) {
    const bool framing = frame_size != 0;
    const unsigned int num_frames =
        framing ? (byte_size / PTR_BYTE_WIDTH / frame_size) : 1;
    const unsigned int iterations_per_frame =
        framing ? frame_size : (byte_size / PTR_BYTE_WIDTH);
    for (unsigned int n = 0; n < iterations; n++) {
        for (int frame = 0; frame < num_frames; frame++) {
            for (int i = 0; i < iterations_per_frame; i++) {
#pragma HLS PIPELINE II = 1
                pkt temp;
                temp.data = data_input[frame * iterations_per_frame + i];
                if (framing) {
                    temp.keep = -1;
                    temp.last = (i == (frame_size - 1));
                }
                temp.dest = dest;
                data_output.write(temp);
            }
        }
        if (ack_enable) {
            ack_pkt ack = ack_stream.read();
        }
    }
}
}