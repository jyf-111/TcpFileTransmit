#pragma once

#include <spdlog/spdlog.h>

#include <array>
#include <filesystem>

#define SIZE 65536

class File {
    std::filesystem::path path;

   public:
    File(const std::filesystem::path &path);
    File(const File &) = delete;
    File(File &&) = delete;
    File &operator=(const File &) = default;
    File &operator=(File &&) = delete;

    [[nodiscard]] std::string GetFileData() const;
    [[nodiscard]] std::filesystem::path GetFilePath() const;

    void SetFilePath(const std::filesystem::path &path);
    void SetFileData(const std::string &data) const;
    void SetFileData(const std::array<char, SIZE>) const;
};
