#include "File.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

using namespace spdlog;

File::File() = default;

File::File(const std::filesystem::path &path) : path(std::move(path)) {}

void File::PathIsValid() const {}

const std::size_t File::GetFileSize() const {
    return std::filesystem::file_size(path);
}

std::string File::QueryDirectory() const {
    if (path.empty() || !std::filesystem::exists(path) ||
        !std::filesystem::is_directory(path)) {
        throw std::runtime_error("file path is not valid");
    }
    std::string tmp;
    for (const auto &p : std::filesystem::directory_iterator(path)) {
        tmp += p.path().string() + "\n";
    }
    return tmp;
}

const std::filesystem::path &File::GetFilePath() const { return path; }

const std::vector<std::vector<char>>File::GetFileDataSplited(
    const std::size_t &slice) const {
    if (path.empty() || !std::filesystem::is_regular_file(path)) {
        throw std::runtime_error("file path is not valid");
    }
    std::ifstream ifs(path, std::ios::binary);
    std::vector<std::vector<char>> file_data;

    const auto size = std::filesystem::file_size(path);

    while (ifs.tellg() < size) {
        if (ifs.tellg() + static_cast<std::ios::pos_type>(slice) >= size) {
            const auto s = size - ifs.tellg();
            std::vector<char> data(s);
            ifs.read(data.data(), s);
            file_data.push_back(data);
            break;
        } else {
            std::vector<char> data(slice);
            ifs.read(data.data(), slice);
            file_data.push_back(data);
        }
    }
    return file_data;
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
    if (path.empty() || !std::filesystem::exists(path)) {
        throw std::runtime_error("file path is not valid");
    }
    std::filesystem::remove(path);
}
