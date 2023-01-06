#include <gtest/gtest.h>

#include <array>
#include "File.h"

TEST(FileTest, test) {
    File file("test.txt");
    ASSERT_EQ(file.GetFilePath().filename(), "test.txt");
}

#define SIZE 65536
template<typename T>
void func(std::array<T, SIZE> arr) {
	std::cout << arr.size() << std::endl;
}

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
