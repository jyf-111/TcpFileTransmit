#include "TcpClient.h"

int main(int argc, char *argv[]) {
    TcpClient client("127.0.0.1", 1234);
	std::cout << client.handleGet(".") << std::endl;
    return 0;
}
