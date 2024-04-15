#!/bin/bash
#SBATCH -p fpga
#SBATCH --constraint=xilinx_u280_xrt2.14
#SBATCH -t 1:00:00
#SBATCH -o scale-%j.out
#SBATCH -e scale-%j.out
#SBATCH -J UDP-scale
#SBATCH -N 3
#SBATCH -n 8
#SBATCH --tasks-per-node=3
#SBATCH --mem=32g
#SBATCH --cpus-per-task=4
#SBATCH -x n2fpga03,n2fpga06

module reset
module load fpga xilinx/xrt/2.14 devel CMake changeFPGAlinks toolchain gompi

cd build

make scale

srun -n 3 --spread-job xbutil reset -d 0000:01:00.1 --force
srun -n 3 --spread-job xbutil reset -d 0000:81:00.1 --force
srun -n 3 --spread-job xbutil reset -d 0000:a1:00.1 --force

srun -n 1 --spread-job changeFPGAlinksXilinx --fpgalink=n00:acl0:ch0-eth \
                    --fpgalink=n00:acl1:ch0-eth \
                    --fpgalink=n00:acl2:ch0-eth \
                    --fpgalink=n00:acl0:ch1-eth \
                    --fpgalink=n00:acl1:ch1-eth \
                    --fpgalink=n00:acl2:ch1-eth \
                    --fpgalink=n01:acl0:ch0-eth \
                    --fpgalink=n01:acl1:ch0-eth \
                    --fpgalink=n01:acl2:ch0-eth \
                    --fpgalink=n01:acl0:ch1-eth \
                    --fpgalink=n01:acl1:ch1-eth \
                    --fpgalink=n01:acl2:ch1-eth \
                    --fpgalink=n02:acl0:ch0-eth \
                    --fpgalink=n02:acl1:ch0-eth \
                    --fpgalink=n02:acl2:ch0-eth \
                    --fpgalink=n02:acl0:ch1-eth \
                    --fpgalink=n02:acl1:ch1-eth \
                    --fpgalink=n02:acl2:ch1-eth 

sleep 10

srun ./scale
