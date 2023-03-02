#pragma once
#include <array>
#include <filesystem>
#include <vector>

class File {
   public:
    File() = delete;
    File(const File &) = delete;
    File(File &&) = delete;
    File &operator=(const File &) = delete;
    File &operator=(File &&) = delete;

    [[nodiscard]] static const bool FileIsExist(const std::filesystem::path &);

    static void ReNameFile(const std::filesystem::path &,
                           const std::filesystem::path &);
    static void DeleteActualFile(const std::filesystem::path &);

    static void SetFileData(const std::filesystem::path &,
                            const std::vector<char> &);

    [[nodiscard]] static const std::string QueryDirectory(
        const std::filesystem::path &);
    [[nodiscard]] static const std::size_t GetFileSize(
        const std::filesystem::path &);
    [[nodiscard]] static const std::size_t GetRemoteFileSize(
        const std::filesystem::path &,
        const std::vector<std::pair<std::string, std::size_t>>);

    [[nodiscard]] static const std::vector<std::vector<char>>
    GetFileDataSplited(const std::filesystem::path &, const int &,
                       const std::size_t &);
};
