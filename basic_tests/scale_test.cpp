#include "xrt/xrt_bo.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"
#include "experimental/xrt_ip.h"
#include "vnx/cmac.hpp"
#include "vnx/networklayer.hpp"
#include "mpi.h"

#include <thread>
#include <stdlib.h>
#include <future>

// Data size sent per kernel execution in bytes
const size_t data_size = 2147483648;
// Timeout in ms that is used to wait for the receiving kernel to complete
// If timeout occurs, the data transfer will be handled as failed
const size_t timeout_in_ms = 30000;
// Show debug information like socket tables and network stack statistics for each rank
const bool debug_output = false;

bool setup_nw(int index, int rank, int size, vnx::Networklayer network_layer, vnx::CMAC cmac)
{
    auto link_status = cmac.link_status();
    link_status = cmac.link_status();
    if (link_status.at("rx_status"))
    {
        std::cout << "Rank " << rank << "," << index << ": Link successful!" << std::endl;
    }
    else
    {
        std::cout << "Rank " << rank << "," << index << ": No link found." << std::endl;
    }

    if (!link_status.at("rx_status"))
    {
        return false;
    }

    network_layer.update_ip_address("192.168.0." + std::to_string(2 * rank + index));

    for (int r = 0; r < 2 * size; r++)
    {
        // Also creates loopback socket, but we will not use it
        network_layer.configure_socket(r, "192.168.0." + std::to_string(r), 5000,
                                           5000, true);
    }
    network_layer.populate_socket_table();

    std::this_thread::sleep_for(std::chrono::seconds(4));
    network_layer.arp_discovery();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    network_layer.arp_discovery();
    return true;
}

int run_test(std::string bfd, std::string bitstream, int rank, int size)
{
    xrt::device dev(bfd);
    auto uuid = dev.load_xclbin(bitstream);
    MPI_Barrier(MPI_COMM_WORLD);
    vnx::Networklayer network_layer(xrt::ip(dev, uuid, "networklayer:{networklayer_0}"));
    vnx::CMAC cmac(xrt::ip(dev, uuid, "cmac_0"));
    vnx::Networklayer network_layer2(xrt::ip(dev, uuid, "networklayer:{networklayer_1}"));
    vnx::CMAC cmac2(xrt::ip(dev, uuid, "cmac_1"));

    MPI_Barrier(MPI_COMM_WORLD);

    bool nw1_up = false;
    bool nw2_up = false;
    std::future<bool> c1, c2;
    c1 = std::async(std::launch::async, setup_nw, 0, rank, size, network_layer, cmac);
    c2 = std::async(std::launch::async, setup_nw, 1, rank, size, network_layer2, cmac2);
    nw1_up = c1.get();
    nw2_up = c2.get();

    MPI_Barrier(MPI_COMM_WORLD);
    if (!(nw1_up && nw2_up))
    {
        std::cerr << "Network not set up correctly on " << rank << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    if (debug_output)
    {
        for (int i = 0; i < size; i++)
        {
            if (rank == i)
            {
                std::cout << "RANK " << rank << ": Socket Table 1" << std::endl;
                network_layer.print_socket_table(2 * size);
                std::cout << "RANK " << rank << ": Socket Table 2" << std::endl;
                network_layer2.print_socket_table(2 * size);
                std::cout << std::flush;
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    long abserror = 0;
    for (int rep = 0; rep < 1; rep++)
    {
        if (rank == 0)
        {
            std::cout << "Start repetition " << rep << std::endl
                      << std::endl;
        }
        xrt::kernel mm2s(dev, uuid, "krnl_mm2s:{krnl_mm2s_0}");
        xrt::kernel s2mm(dev, uuid, "krnl_s2mm:{krnl_s2mm_0}");

        xrt::kernel mm2s2(dev, uuid, "krnl_mm2s:{krnl_mm2s_1}");
        xrt::kernel s2mm2(dev, uuid, "krnl_s2mm:{krnl_s2mm_1}");

        if (rank == 0)
        {
            std::cout << "Generate Input data" << std::endl;
        }
        xrt::bo bo_in(dev, data_size, 0, mm2s.group_id(0));
        xrt::bo bo_in2(dev, data_size, 0, mm2s2.group_id(0));
        xrt::bo bo_out(dev, data_size, 0, s2mm.group_id(0));
        xrt::bo bo_out2(dev, data_size, 0, s2mm2.group_id(0));

        srand(0);
        for (int i = 0; i < data_size / sizeof(int); i++)
        {
            bo_in.map<int *>()[i] = i + 2 * rank;
            bo_in2.map<int *>()[i] = i + 2 * rank + 1;
        }

        bo_in.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        bo_in2.sync(XCL_BO_SYNC_BO_TO_DEVICE);

        if (debug_output)
        {
            for (int i = 0; i < size; i++)
            {
                if (rank == i)
                {
                    std::cout << "RANK " << rank << ": Initial Values " << std::endl;
                    network_layer.get_udp_in_pkts();
                    network_layer.get_udp_app_in_pkts();
                    network_layer.get_udp_app_out_pkts();
                    network_layer.get_udp_out_pkts();

                    network_layer2.get_udp_in_pkts();
                    network_layer2.get_udp_app_in_pkts();
                    network_layer2.get_udp_app_out_pkts();
                    network_layer2.get_udp_out_pkts();
                    std::cout << std::flush;
                }
                MPI_Barrier(MPI_COMM_WORLD);
            }
        }

        xrt::run r = s2mm(bo_out, 0, data_size);
        xrt::run r2 = s2mm2(bo_out2, 0, data_size);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        MPI_Barrier(MPI_COMM_WORLD);
        auto t1 = std::chrono::high_resolution_clock::now();
        xrt::run r3 = mm2s(bo_in, 0, data_size, (2 * rank + 1) % (2 * size));
        r2.wait(std::chrono::milliseconds(timeout_in_ms));
        r3.wait();
        auto t2 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        double max_ms;
        MPI_Reduce(&ms, &max_ms, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        if (rank == 0)
        {
            std::cout << "0 -> 0: " << max_ms << "ms, " << ((size * data_size) / (max_ms) * 1.0e-6) << "GB/s" << std::endl;
        }
        if (debug_output)
        {
            for (int i = 0; i < size; i++)
            {
                if (rank == i)
                {
                    std::cout << "RANK " << rank << ": Time in ms: " << ms << std::endl;
                    network_layer.get_udp_in_pkts();
                    network_layer.get_udp_app_in_pkts();
                    network_layer.get_udp_app_out_pkts();
                    network_layer.get_udp_out_pkts();

                    network_layer2.get_udp_in_pkts();
                    network_layer2.get_udp_app_in_pkts();
                    network_layer2.get_udp_app_out_pkts();
                    network_layer2.get_udp_out_pkts();
                    std::cout << std::flush;
                }
                MPI_Barrier(MPI_COMM_WORLD);
            }
        }

        if (ms > timeout_in_ms)
        {
            MPI_Abort(MPI_COMM_WORLD, 2);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        t1 = std::chrono::high_resolution_clock::now();
        xrt::run r4 = mm2s2(bo_in2, 0, data_size, (2 * size + 2 * (rank + 1)) % (2 * size));
        r.wait(std::chrono::milliseconds(timeout_in_ms));
        r4.wait();
        t2 = std::chrono::high_resolution_clock::now();
        ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        MPI_Reduce(&ms, &max_ms, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        if (rank == 0)
        {
            std::cout << "1 -> 0: " << max_ms << "ms, " << ((size * data_size) / (max_ms) * 1.0e-6) << "GB/s" << std::endl;
        }

        if (debug_output)
        {
            for (int i = 0; i < size; i++)
            {
                if (rank == i)
                {
                    std::cout << "RANK " << rank << ": Time in ms: " << ms << std::endl;
                    network_layer.get_udp_in_pkts();
                    network_layer.get_udp_app_in_pkts();
                    network_layer.get_udp_app_out_pkts();
                    network_layer.get_udp_out_pkts();

                    network_layer2.get_udp_in_pkts();
                    network_layer2.get_udp_app_in_pkts();
                    network_layer2.get_udp_app_out_pkts();
                    network_layer2.get_udp_out_pkts();
                    std::cout << std::flush;
                }
                MPI_Barrier(MPI_COMM_WORLD);
            }
        }

        if (ms > timeout_in_ms)
        {
            MPI_Abort(MPI_COMM_WORLD, 3);
        }
        MPI_Barrier(MPI_COMM_WORLD);

        bo_out.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        bo_out2.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

        int error1 = 0;
        int error2 = 0;
        for (int i = 0; i < data_size / sizeof(int); i++)
        {
            if (bo_out.map<int *>()[i] != i + ((2 * (rank + size) - 1) % (2*size)))
            {
                error1++;
            }
            if (bo_out2.map<int *>()[i] != i + ((2 * (rank + size)) % (2*size)))
            {
                error2++;
            }
        }
        if (error1 + error2)
        {
            std::cerr << "Rank " << rank << "= Error1: " << error1 << ", Error2: " << error2 << std::endl;
        }
        abserror += error1 + error2;
    }
    return abserror;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Please give path to bitstream as argument" << std::endl;
        return 1;
    }
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    std::string used_bfd;
    switch (rank % 3)
    {
    case 0:
        used_bfd = "0000:01:00.1";
        break;
    case 1:
        used_bfd = "0000:81:00.1";
        break;
    default:
        used_bfd = "0000:a1:00.1";
    }
    std::string bitstream(argv[1]);
    long abserror = run_test(used_bfd, bitstream, rank, size);
    long global_error;
    MPI_Reduce(&abserror, &global_error, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    if (rank == 0)
    {
        if (global_error == 0)
        {
            std::cout << "SUCCESS!" << std::endl;
        }
        else
        {
            std::cout << "FAILED! ERRORS: " << global_error << std::endl;
        }
    }
    else
    {
        global_error = 0;
    }
    MPI_Finalize();
    return abserror != 0;
}