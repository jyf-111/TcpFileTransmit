#pragma once
#include <filesystem>

class AbstractFile {
   public:
    /**
     * @brief test FilePath is empty
     */
    virtual void PathIsValid() const = 0;

    /**
     * @brief get file path
     * @return file path
     */
    [[nodiscard]] virtual const std::filesystem::path &GetFilePath() const = 0;

    /**
     * @brief Write file path
     * @param data Data to write
     */
    virtual void SetFilePath(const std::filesystem::path &path) = 0;
};
