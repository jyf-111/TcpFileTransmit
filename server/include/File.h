#pragma once
#include <array>
#include <filesystem>
#include <vector>

/**
 * @brief Class for reading and writing files
 */
class File {
    std::filesystem::path path;

   public:
    File();
    File(const std::filesystem::path &path);
    File(const File &) = default;
    File(File &&) = default;
    File &operator=(const File &) = default;
    File &operator=(File &&) = default;

    /**
     * @brief test FilePath is empty
     */
    void PathIsValid() const;

    /**
     * @brief rename file
     */
    void ReNameFile(const std::filesystem::path &);
    /**
     * @brief get file size
     * @return file size
     */
    [[nodiscard]] const std::size_t GetFileSize() const;

    /**
     * @brief get file path
     * @return file path
     */
    [[nodiscard]] const std::filesystem::path &GetFilePath() const;

    /**
     * @brief Write file path
     * @param data Data to write
     */
    void SetFilePath(const std::filesystem::path &path);

    /**
     * @brief query directory
     * @return std::string
     */
    [[nodiscard]] std::string QueryDirectory() const;

    /**
     * @brief delete file
     */
    void DeleteActualFile() const;

    /**
     * @brief Read file data
     * @return File data spilted
     */
    [[nodiscard]] const std::vector<std::vector<char>> GetFileDataSplited(
        const std::size_t &slice) const;

    /**
     * @brief Write file data
     * @param data Data to write
     */
    void SetFileData(const std::vector<char> &data) const;
};
