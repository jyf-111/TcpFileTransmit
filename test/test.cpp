#include <gtest/gtest.h>

#include <asio.hpp>

#include "File.h"
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

    return RUN_ALL_TESTS();
}
