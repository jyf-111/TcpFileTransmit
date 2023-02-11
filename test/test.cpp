#include <gtest/gtest.h>
#include <signal.h>

#include <asio.hpp>

#include "File.h"
#include "ProtoBuf.h"
#include "asio/io_context.hpp"

TEST(FileTest, test) {
    File file("test.txt");
    ASSERT_EQ(file.GetFilePath().filename(), "test.txt");
}

TEST(ProtoBuf, Method) {
    ASSERT_EQ(ProtoBuf::MethodToString(ProtoBuf::Method::Get), "GET");
    ASSERT_EQ(ProtoBuf::MethodToString(ProtoBuf::Method::Post), "POST");
    ASSERT_EQ(ProtoBuf::MethodToString(ProtoBuf::Method::Delete), "DELETE");
}

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);

    asio::io_context io;
    asio::signal_set sig(io, SIGINT, SIGTERM);
    sig.async_wait([](const std::error_code& e, int signal_number) {
        switch (signal_number) {
            case SIGINT:
                std::cout << "SIGINT received, shutting down" << std::endl;
                // io.stop();
                break;
            case SIGTERM:
                std::cout << "SIGTERM received, shutting down" << std::endl;
                // io.stop();
                break;
            default:
                std::cout << "Signal {}" << std::endl;
                // io.stop();
                break;
        }
    });
    asio::io_context::work work(io);
    io.run();

    return RUN_ALL_TESTS();
}
