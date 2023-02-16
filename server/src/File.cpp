#include "File.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

using namespace spdlog;

File::File() = default;

File::File(const std::filesystem::path &path) : path(std::move(path)) {}

void File::PathIsValid() const {
    if (path.empty() || !std::filesystem::exists(path)) {
        throw std::runtime_error("file is not valid");
    }
}

std::string File::QueryDirectory() const {
    PathIsValid();
    std::string tmp;
    for (const auto &p : std::filesystem::directory_iterator(path)) {
        tmp += p.path().string() + " ";
    }
    return tmp;
}

const std::filesystem::path &File::GetFilePath() const { return path; }

const std::vector<char> File::GetFileData() const {
    PathIsValid();
    std::ifstream ifs(path, std::ios::binary);
    auto size = std::filesystem::file_size(path);
    std::vector<char> data(size);
    ifs.read(data.data(), size);
    return data;
}

void File::SetFilePath(const std::filesystem::path &path) { this->path = path; }

void File::SetFileData(const std::vector<char> &data) const {
    if (path.empty()) throw std::runtime_error("file is not valid");
    std::ofstream ofs(path, std::ios::binary | std::ios::app);
    ofs.write(data.data(), data.size());
    ofs.close();
    debug("Writing to file {} ", path.string());
}

void File::DeleteActualFile() const {
    PathIsValid();
    std::filesystem::remove(path);
}
