#include "File.h"

#include <fstream>

File::File(const std::filesystem::path &path) : path(std::move(path)) {}

std::filesystem::path File::GetFilePath() const { return path; }

std::string File::GetFileData() const {
    std::ifstream ifs(path);
    return {std::istreambuf_iterator<char>(ifs),
            std::istreambuf_iterator<char>()};
}

void File::SetFilePath(const std::filesystem::path &path) { this->path = path; }

void File::SetFileData(const std::string &data) const {
    std::ofstream ofs(path);
    ofs << data;
}
