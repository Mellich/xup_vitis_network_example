#include "xrt/xrt_bo.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"
#include "experimental/xrt_ip.h"
#include "vnx/cmac.hpp"
#include "vnx/networklayer.hpp"

#include <thread>
#include <stdlib.h>
#include <future>

const size_t data_size = 2147483648;

bool setup_nw(int index, vnx::Networklayer network_layer, vnx::CMAC cmac)
{
    auto link_status = cmac.link_status();
    link_status = cmac.link_status();
    if (link_status.at("rx_status"))
    {
        std::cout << "Link successful!" << std::endl;
    }
    else
    {
        std::cout << "No link found." << std::endl;
    }

    if (!link_status.at("rx_status"))
    {
        return false;
    }

    std::cout << "Populating socket table..." << std::endl;

    network_layer.update_ip_address("192.168.0." + std::to_string(index));

    network_layer.configure_socket(0, "192.168.0." + std::to_string((index + 1) % 2), 5000,
                                   5000, true);

    network_layer.populate_socket_table();

    std::cout << "Starting ARP discovery..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(4));
    network_layer.arp_discovery();
    return true;
}

int run_test(std::string bfd)
{
    xrt::device dev(bfd);
    auto uuid = dev.load_xclbin("../../basic.intf3.xilinx_u280_gen3x16_xdma_1_202211_1/vnx_basic_if3.xclbin");
    vnx::Networklayer network_layer(xrt::ip(dev, uuid, "networklayer:{networklayer_0}"));
    vnx::CMAC cmac(xrt::ip(dev, uuid, "cmac_0"));
    vnx::Networklayer network_layer2(xrt::ip(dev, uuid, "networklayer:{networklayer_1}"));
    vnx::CMAC cmac2(xrt::ip(dev, uuid, "cmac_1"));

    bool nw1_up = false;
    bool nw2_up = false;
    while (!(nw1_up && nw2_up))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::future<bool> c1, c2;
        if (!nw1_up)
        {
            c1 = std::async(std::launch::async, setup_nw, 0, network_layer, cmac);
        }
        if (!nw2_up)
        {
            c2 = std::async(std::launch::async, setup_nw, 1, network_layer2, cmac2);
        }
        if (!nw1_up)
        {
            nw1_up = c1.get();
        }
        if (!nw2_up)
        {
            nw2_up = c2.get();
        }
    }
    std::cout << "Finishing ARP discovery..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    network_layer.arp_discovery();
    network_layer2.arp_discovery();
    std::cout << "ARP discovery finished!" << std::endl;

    long abserror = 0;
    for (int rep = 0; rep < 10; rep++)
    {
        std::cout << "Start repetition " << rep << std::endl
                  << std::endl;
        xrt::kernel mm2s(dev, uuid, "krnl_mm2s:{krnl_mm2s_0}");
        xrt::kernel s2mm(dev, uuid, "krnl_s2mm:{krnl_s2mm_1}");

        xrt::kernel mm2s2(dev, uuid, "krnl_mm2s:{krnl_mm2s_1}");
        xrt::kernel s2mm2(dev, uuid, "krnl_s2mm:{krnl_s2mm_0}");

        std::cout << "Generate Input data" << std::endl;
        xrt::bo bo_in(dev, data_size, 0, mm2s.group_id(0));
        xrt::bo bo_in2(dev, data_size, 0, mm2s2.group_id(0));
        xrt::bo bo_out(dev, data_size, 0, s2mm.group_id(0));
        xrt::bo bo_out2(dev, data_size, 0, s2mm2.group_id(0));

        srand(0);
        for (int i = 0; i < data_size / sizeof(int); i++)
        {
            bo_in.map<int *>()[i] = rand();
            bo_in2.map<int *>()[i] = rand();
        }

        bo_in.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        bo_in2.sync(XCL_BO_SYNC_BO_TO_DEVICE);

        network_layer.get_udp_in_pkts();
        network_layer.get_udp_app_in_pkts();
        network_layer.get_udp_app_out_pkts();
        network_layer.get_udp_out_pkts();

        network_layer2.get_udp_in_pkts();
        network_layer2.get_udp_app_in_pkts();
        network_layer2.get_udp_app_out_pkts();
        network_layer2.get_udp_out_pkts();
        xrt::run r = s2mm(bo_out, 0, data_size);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto t1 = std::chrono::high_resolution_clock::now();
        xrt::run r3 = mm2s(bo_in, 0, data_size, 0);
        r.wait(std::chrono::seconds(5));
        r3.wait();
        auto t2 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        std::cout << "0 -> 1: " << ms << "ms, " << (data_size / (ms) * 1.0e-6) << "GB/s" << std::endl;

        network_layer.get_udp_in_pkts();
        network_layer.get_udp_app_in_pkts();
        network_layer.get_udp_app_out_pkts();
        network_layer.get_udp_out_pkts();

        network_layer2.get_udp_in_pkts();
        network_layer2.get_udp_app_in_pkts();
        network_layer2.get_udp_app_out_pkts();
        network_layer2.get_udp_out_pkts();

        xrt::run r2 = s2mm2(bo_out2, 0, data_size);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        t1 = std::chrono::high_resolution_clock::now();
        xrt::run r4 = mm2s2(bo_in2, 0, data_size, 0);
        r2.wait(std::chrono::seconds(5));
        r4.wait();
        t2 = std::chrono::high_resolution_clock::now();
        ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        std::cout << "1 -> 0: " << ms << "ms, " << (data_size / (ms) * 1.0e-6) << "GB/s" << std::endl;

        network_layer.get_udp_in_pkts();
        network_layer.get_udp_app_in_pkts();
        network_layer.get_udp_app_out_pkts();
        network_layer.get_udp_out_pkts();

        network_layer2.get_udp_in_pkts();
        network_layer2.get_udp_app_in_pkts();
        network_layer2.get_udp_app_out_pkts();
        network_layer2.get_udp_out_pkts();

        bo_out.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        bo_out2.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

        int error1 = 0;
        int error2 = 0;
        for (int i = 0; i < data_size / sizeof(int); i++)
        {
            if (bo_out.map<int *>()[i] != bo_in.map<int *>()[i])
            {
                error1++;
            }
            if (bo_out2.map<int *>()[i] != bo_in2.map<int *>()[i])
            {
                error2++;
            }
        }
        std::cout << "Error1: " << error1 << ", Error2: " << error2 << std::endl;
        abserror += error1 + error2;
    }
    return abserror;
}

int main()
{
    long abserror = run_test("0000:01:00.1") + run_test("0000:81:00.1") + run_test("0000:a1:00.1");
    if (abserror == 0)
    {
        std::cout << "SUCCESS!" << std::endl;
    }
    else
    {
        std::cout << "FAILED! ERRORS: " << abserror << std::endl;
    }
    return abserror != 0;
}