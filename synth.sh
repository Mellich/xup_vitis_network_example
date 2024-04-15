#!/bin/bash
#SBATCH -p normal
#SBATCH -q fpgasynthesis
#SBATCH -t 12:00:00
#SBATCH --cpus-per-task=16
#SBATCH --mem=64g

module reset
module load fpga xilinx/xrt/2.16

export LM_LICENSE_FILE="27000@kiso.uni-paderborn.de"

make all DEVICE=xilinx_u280_gen3x16_xdma_1_202211_1 INTERFACE=3 DESIGN=latency MAX_SOCKETS=16