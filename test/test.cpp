#include <gtest/gtest.h>

#include <asio.hpp>

#include "File.h"
#include "Properties.h"
#include "ProtoBuf.h"

TEST(FileTest, test) {
    File file("test.txt");
    ASSERT_EQ(file.GetFilePath().filename(), "test.txt");
}

TEST(Method, ProtoBuf) {
    ASSERT_EQ(ProtoBuf::MethodToString(ProtoBuf::Method::Get), "GET");
    ASSERT_EQ(ProtoBuf::MethodToString(ProtoBuf::Method::Post), "POST");
    ASSERT_EQ(ProtoBuf::MethodToString(ProtoBuf::Method::Delete), "DELETE");
}

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    asio::io_context io;
    asio::ip::tcp::resolver resolver(io);
    resolver.async_resolve(
        "jyfwin.japaneast.cloudapp.azure.com", "1234",
        [](const asio::error_code& e, asio::ip::tcp::resolver::iterator iter) {
            if (e) {
                error("query Error: {}", e.message());
                return;
            }
            info("ip: {}", iter->endpoint().address().to_string());
        });
    io.run();
    return RUN_ALL_TESTS();
}
