#include <asio.hpp>
#include <iostream>
#include <iterator>

#include "ProtoBuf.h"

int main(int argc, char *argv[]) {
    ProtoBuf protobuf(ProtoBuf::Method::Get, ".", "data");
    asio::streambuf buf;
    std::ostream os(&buf);
    os << protobuf;
    std::istream is(&buf);
    ProtoBuf protobuf2;
    is >> protobuf2;
    std::cout << protobuf2 << std::endl;

    return 0;
}
