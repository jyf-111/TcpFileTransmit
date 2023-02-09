#include <asio.hpp>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>


int main(int argc, char *argv[]) {
    std::filebuf buf;
    buf.open(".classpath", std::ios::in);
    std::istream is(&buf);
    std::string s;
    while (getline(is,s)) {
        std::cout << s << std::endl;
        is.ignore();
    }
    return 0;
}
