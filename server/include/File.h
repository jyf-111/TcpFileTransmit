#pragma once
#include <array>
#include <filesystem>
#include <vector>

class File {
    std::filesystem::path path;

   public:
    File();
    File(const std::filesystem::path &path);
    File(const File &) = default;
    File(File &&) = default;
    File &operator=(const File &) = default;
    File &operator=(File &&) = default;

    static void ReNameFile(const std::filesystem::path &,
                           const std::filesystem::path &);
    [[nodiscard]] const std::size_t GetFileSize() const;

    [[nodiscard]] const std::filesystem::path &GetFilePath() const;

    void SetFilePath(const std::filesystem::path &path);

    [[nodiscard]] std::string QueryDirectory() const;

    void DeleteActualFile() const;

    [[nodiscard]] const std::vector<std::vector<char>> GetFileDataSplited(
        const std::size_t &slice) const;

    void SetFileData(const std::vector<char> &data) const;
};
