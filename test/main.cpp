#include <ios>
#include <iostream>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <fstream>

int main(int argc, char *argv[])
{
	auto path = std::filesystem::path("data.txt");
	std::ofstream ofs(path);
	std::cout << std::boolalpha << ofs.is_open() << std::endl;
	return 0;
}
