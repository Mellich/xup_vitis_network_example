#!/bin/bash
#SBATCH -p fpga
#SBATCH --constraint=xilinx_u280_xrt2.14
#SBATCH -t 1:00:00
#SBATCH -o %N-eth-%j.out
#SBATCH -e %N-eth-%j.out
#SBATCH -J UDP-test-eth
#SBATCH -N 1
#SBATCH -n 1
#SBATCH --mem=32g
#SBATCH --cpus-per-task=4

module reset
module load fpga xilinx/xrt/2.14 devel CMake changeFPGAlinks

cd build

xbutil reset -d 0000:01:00.1 --force
xbutil reset -d 0000:81:00.1 --force
xbutil reset -d 0000:a1:00.1 --force

changeFPGAlinksXilinx --fpgalink=n00:acl0:ch0-eth \
                    --fpgalink=n00:acl1:ch0-eth \
                    --fpgalink=n00:acl2:ch0-eth \
                    --fpgalink=n00:acl0:ch1-eth \
                    --fpgalink=n00:acl1:ch1-eth \
                    --fpgalink=n00:acl2:ch1-eth

sleep 1

./test
