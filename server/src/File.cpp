#include "File.h"

#include <fstream>

using namespace spdlog;

File::File(const std::filesystem::path &path) : path(std::move(path)) {}

std::filesystem::path File::GetFilePath() const { return path; }

std::string File::GetFileData() const {
    std::ifstream ifs(path);
    return {std::istreambuf_iterator<char>(ifs),
            std::istreambuf_iterator<char>()};
}

void File::SetFilePath(const std::filesystem::path &path) { this->path = path; }

void File::SetFileData(const std::string &data) const {
	info("Writing to file {} begin", path.string());
    std::ofstream ofs(path,std::ios::app);
    ofs << data;
	info("Writing to file {} end", path.string());
}

void File::SetFileData(const std::array<char, SIZE> data) const {
	info("Writing to file {} begin", path.string());
    std::ofstream ofs(path, std::ios::app);
    ofs.write(data.data(), data.size());
	info("Writing to file {} end", path.string());
}
