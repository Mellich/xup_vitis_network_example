/*
 * Copyright 2022 Xilinx, Inc.
 *           2023-2024 Gerrit Pape (papeg@mail.upb.de)
 *           2024 Marius Meyer (marius.meyer@uni-paderborn.de)
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
#include <ap_axi_sdata.h>
#include <ap_int.h>

#define PTR_WIDTH 512
#define PTR_BYTE_WIDTH (PTR_WIDTH / 8)
#define DEST_WIDTH 16

typedef ap_axiu<PTR_WIDTH, 1, 1, DEST_WIDTH> pkt;
typedef ap_axiu<1, 0, 0, 0> ack_pkt;
