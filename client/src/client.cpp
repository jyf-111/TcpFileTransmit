#include <spdlog/spdlog.h>

#include <array>
#include <asio.hpp>
#include <fstream>
#include <iostream>

#include "ProtoBuf.h"

using namespace spdlog;

int main(int argc, char *argv[]) {
    asio::io_service io;
    asio::ip::tcp::endpoint ep(asio::ip::address::from_string("127.0.0.1"),
                               1234);
    asio::ip::tcp::socket sock(io);
    sock.connect(ep);
    info("Connect success");

    try {
		// TODO protoBuf 
        ProtoBuf pb;
		pb.SetMethod(ProtoBuf::Method::Get);
		pb.SetData({'a','b','c'});
		
        sock.write_some(asio::buffer(pb.GetProtoBuf()));
        info("Send success");
    } catch (std::exception &e) {
        error("Error: {}", e.what());
        return 0;
    }
}
