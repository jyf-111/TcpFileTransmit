#include "File.h"

#include <fstream>

#include "spdlog/spdlog.h"

using namespace spdlog;

File::File() = default;

File::File(const std::filesystem::path &path) : path(std::move(path)) {}

bool File::PathIsEmpty() const { return path.empty(); }

bool File::PathIsExist() const {
    return !path.empty() && std::filesystem::exists(path);
}

std::string File::QueryDirectory() const {
    std::string tmp;
    for (const auto &p : std::filesystem::directory_iterator(path)) {
        tmp += p.path().string() + "\n";
    }
    return tmp;
}

std::filesystem::path File::GetFilePath() const { return path; }

std::string File::GetFileData() const {
    std::ifstream ifs(path);
    return {std::istreambuf_iterator<char>(ifs),
            std::istreambuf_iterator<char>()};
}

void File::SetFilePath(const std::filesystem::path &path) { this->path = path; }

void File::SetFileData(const std::string &data) const {
    info("Writing to file {} begin", path.string());
    std::ofstream ofs(path, std::ios::app);
    ofs << data;
    info("Writing to file {} end", path.string());
}

void File::SetFileData(const std::array<char, SIZE> data) const {
    info("Writing to file {} begin", path.string());
    std::ofstream ofs(path, std::ios::app);
    ofs.write(data.data(), data.size());
    info("Writing to file {} end", path.string());
}

void File::DeleteActualFile() const {
    info("Deleting file {}", path.string());
    std::filesystem::remove(path);
}
