#pragma once
#include <array>
#include <filesystem>

#include "AbstractFile.h"

/**
 * @brief Class for reading and writing files
 */
class File : public AbstractFile {
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
    void PathIsValid() const override;

    /**

     * @brief get file path
     * @return file path
     */
    [[nodiscard]] const std::filesystem::path &GetFilePath() const override;

    /**
     * @brief Write file path
     * @param data Data to write
     */
    void SetFilePath(const std::filesystem::path &path) override;

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
     * @return File data
     */
    [[nodiscard]] const std::vector<char> GetFileData() const;

    /**
     * @brief Write file data
     * @param data Data to write
     */
    void SetFileData(const std::vector<char> &data) const;
};
