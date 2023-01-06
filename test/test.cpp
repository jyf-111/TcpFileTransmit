#include <gtest/gtest.h>

#include <array>
#include "File.h"

TEST(FileTest, test) {
    File file("test.txt");
    ASSERT_EQ(file.GetFilePath().filename(), "test.txt");
}

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
