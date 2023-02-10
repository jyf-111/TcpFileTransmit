#include <algorithm>
#include <asio.hpp>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>

int main(int argc, char *argv[]) {
    std::string s("helloworld");
    std::cout << s << std::endl;
    try {
        // s.replace(s.find(' '), 1, "\n");
        std::replace(s.begin(), s.end(), ' ', '\n');
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
    std::cout << s << std::endl;
}
