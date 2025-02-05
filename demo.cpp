#include <systemc>
#include <tlm>
#include <iostream>

#include <tlm_utils/simple_initiator_socket.h>
#include "tlm_utils/passthrough_target_socket.h"

using namespace sc_core;
using namespace tlm;

// mem_req_t 定义
struct mem_req_t {
    uint64_t address;
    uint32_t data;
    bool write;  // true: write, false: read
};

// 自定义协议类型
struct mem_req_protocol_types {
    typedef mem_req_t tlm_payload_type;  // 自定义的 payload 类型
    typedef tlm_phase tlm_phase_type;    // 仍然使用 tlm_phase 类型
};

// 发起者模块：发送mem_req_t类型的请求
SC_MODULE(mem_req_initiator) {
    tlm_utils::simple_initiator_socket<mem_req_initiator,64, mem_req_protocol_types> socket;

    SC_CTOR(mem_req_initiator) {
        SC_THREAD(send_request);
    }

    // 发送请求的线程
    void send_request() {
        // 创建一个mem_req_t类型的请求
        mem_req_t req;
        req.address = 0x1000;
        req.data = 0xABCD1234;
        req.write = true;  // 写请求

        // 定义传输的时延
        sc_time delay = SC_ZERO_TIME;

        // 发送请求
        std::cout << "Sending WRITE request to address " << std::hex << req.address << std::endl;
        socket->b_transport(req, delay);

        // 模拟一些延迟后发送一个读取请求
        wait(20, SC_NS);

        req.write = false;  // 读请求
        std::cout << "Sending READ request from address " << req.address << std::endl;
        socket->b_transport(req, delay);
		std::cout << "The read data is: " << std::hex << req.data << std::endl;
    }
};



// 目标模块：处理mem_req_t类型的请求
SC_MODULE(mem_req_target) {
   // tlm_utils::simple_target_socket<mem_req_target,64, mem_req_protocol_types> socket;
	
	tlm_utils::passthrough_target_socket<mem_req_target,64,mem_req_protocol_types> socket;

    SC_CTOR(mem_req_target) {
        socket.register_b_transport(this, &mem_req_target::my_b_transport);
    }

    // 使用 mem_req_t 作为payload
    void my_b_transport(mem_req_t& req, sc_time& delay) {
		static uint32_t tmp=0;
        // 根据请求类型处理
        if (req.write) {
			tmp = req.data;
            std::cout << "Received WRITE request to address " << req.address
                      << " with data " << std::hex << req.data << std::endl;
        } else {
            std::cout << "Received READ request from address " << std::hex << req.address << std::endl;
			req.data = tmp + 1;
        }

        // 模拟延迟：假设每个请求延迟10ns
        delay = sc_time(10, SC_NS);
    }
};



// 顶层模块，连接发起者和目标
SC_MODULE(testbench) {
    mem_req_initiator initiator;
    mem_req_target target;

    SC_CTOR(testbench) :
        initiator("initiator"),
        target("target") {
        // 连接发起者和目标模块的TLM Socket
        initiator.socket.bind(target.socket);
    }
};

// 仿真入口
int sc_main(int argc, char* argv[]) {
    testbench tb("tb");

    // 启动仿真
    sc_start();

    return 0;
}
