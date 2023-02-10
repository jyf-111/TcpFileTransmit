#include "File.h"

#include <spdlog/spdlog.h>

#include <fstream>
#include <memory>

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
        tmp += p.path().string() + " ";
    }
    return tmp;
}

std::filesystem::path File::GetFilePath() const { return path; }

std::string File::GetFileData() const {
    std::unique_ptr<std::ifstream> ifs = std::make_unique<std::ifstream>(path);
    return {std::istreambuf_iterator<char>(*ifs),
            std::istreambuf_iterator<char>()};
}

void File::SetFilePath(const std::filesystem::path &path) { this->path = path; }

void File::SetFileData(const std::string &data) const {
    std::ofstream ofs(path, std::ios::app);
    ofs << data;
    ofs.close();
    debug("Writing to file {} ", path.string());
}

void File::DeleteActualFile() const {
    std::filesystem::remove(path);
    debug("Deleting file {}", path.string());
}
