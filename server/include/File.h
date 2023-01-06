#pragma once

#include <spdlog/spdlog.h>

#include <array>
#include <filesystem>

#define SIZE 65536

/**
 * @brief Class for reading and writing files
 */
class File {
    std::filesystem::path path;

   public:
    File(const std::filesystem::path &path);
    File(const File &) = delete;
    File(File &&) = delete;
    File &operator=(const File &) = default;
    File &operator=(File &&) = delete;

    /*
     * @brief Read file data
     * @return File data
     */
    [[nodiscard]] std::string GetFileData() const;
    /*
     * @brief get file path
     * @return file path
     */
    [[nodiscard]] std::filesystem::path GetFilePath() const;
    /*
     * @brief Write file path
     * @param data Data to write
     */
    void SetFilePath(const std::filesystem::path &path);
    /*
     * @brief Write file data
     * @param data Data to write
     */
    void SetFileData(const std::string &data) const;
    /*
     * @brief Write file data
     * @param data Data to write
     */
    void SetFileData(const std::array<char, SIZE>) const;
};
