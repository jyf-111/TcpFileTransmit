#include "File.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

const std::size_t File::GetFileSize(const std::filesystem::path &path) {
#ifdef _WIN32
    if (std::filesystem::is_directory(path))
        return std::filesystem::file_size(path);
#endif
    if (std::filesystem::is_directory(path))
        return 4096;
    else
        return std::filesystem::file_size(path);
}

const bool File::FileIsExist(const std::filesystem::path &path) {
    return std::filesystem::exists(path);
}

const std::size_t File::GetRemoteFileSize(
    const std::filesystem::path &path,
    const std::vector<std::pair<std::string, std::size_t>> dirList) {
    std::size_t size = 0;
    for (const auto &[filename, filesize] : dirList) {
        if (std::filesystem::path(filename).filename().string() ==
            path.filename().string()) {
            size = filesize;
            spdlog::get("logger")->info("has remote swap file size = {}", size);
        }
    }
    return size;
}

void File::ReNameFile(const std::filesystem::path &from,
                      const std::filesystem::path &to) {
    std::filesystem::rename(from, to);
}

const std::string File::QueryDirectory(const std::filesystem::path &path) {
    if (path.empty() || !std::filesystem::exists(path)) {
        throw std::runtime_error("path is empty or is not exist");
    }
    std::string tmp;
    for (const auto &p : std::filesystem::directory_iterator(path)) {
        if (p.is_directory())
            tmp += p.path().string() + "/ " +
                   std::to_string(GetFileSize(p.path())) + "\n";
        else
            tmp += p.path().string() + " " +
                   std::to_string(GetFileSize(p.path())) + "\n";
    }
    return tmp;
}

const std::vector<std::vector<char>> File::GetFileDataSplited(
    const std::filesystem::path &path, const int &index,
    const std::size_t &slice) {
    if (path.empty() || !std::filesystem::is_regular_file(path)) {
        throw std::runtime_error("path is empty or is not regular file");
    }
    std::ifstream ifs(path, std::ios::binary);
    ifs.seekg(index);

    std::vector<std::vector<char>> file_data;
    const auto &size = File::GetFileSize(path);

    while (ifs.tellg() < size) {
        if (ifs.tellg() + static_cast<std::ios::pos_type>(slice) >= size) {
            const auto &s = size - ifs.tellg();
            std::vector<char> data(s);
            ifs.read(data.data(), s);
            file_data.push_back(std::move(data));
            break;
        } else {
            std::vector<char> data(slice);
            ifs.read(data.data(), slice);
            file_data.push_back(std::move(data));
        }
    }
    ifs.close();
    return file_data;
}

void File::SetFileData(const std::filesystem::path &path,
                       const std::vector<char> &data) {
    if (path.empty()) throw std::runtime_error("file is not valid");
    std::ofstream ofs(path, std::ios::binary | std::ios::app);
    ofs.write(data.data(), data.size());
    ofs.close();
}

void File::DeleteActualFile(const std::filesystem::path &path) {
    if (path.empty() || !std::filesystem::exists(path)) {
        throw std::runtime_error("file path is not valid");
    }
    std::filesystem::remove(path);
}
