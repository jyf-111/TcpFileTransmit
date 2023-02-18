#pragma once

#include <spdlog/spdlog.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Class for ProtoBuf
 */
class ProtoBuf {
   public:
    /**
     * @brief Method enum
     */
    enum class Method {
        Query,
        Get,
        Post,
        Delete,
    };
    ProtoBuf(const Method &method, const std::filesystem::path &path,
             const std::vector<char> &data);
    ProtoBuf() = default;
    ProtoBuf(const ProtoBuf &) = default;
    ProtoBuf(ProtoBuf &&) = default;
    ProtoBuf &operator=(const ProtoBuf &) = default;
    ProtoBuf &operator=(ProtoBuf &&) = default;

    /**
     * @brief Method toString
     * @param Method
     */
    static std::string MethodToString(const Method &);

    /**
     * @brief String to Method
     * @param std::string
     */
    static Method StringToMethod(std::string &);

    /**
     * @brief Method for get whole size
     */
    [[nodiscard]] const std::size_t &GetSize() const;

    /**
     * @brief Method for set whole size
     */
    void SetSize(const std::size_t &);

    /**
     * @brief Method for get whole size
     */
    [[nodiscard]] const std::size_t &GetHeadSize() const;

    /**
     * @brief Method for set whole size
     */
    void SetHeadSize(const std::size_t &);

    /**
     * @brief Method for get isFile
     * @return const bool
     */
    [[nodiscard]] const bool &GetIsFile() const;

    /**
     * @brief Method for set isFile
     */
    void SetIsFile(const bool &);

    /**
     * @brief Method for get index
     * @return const std::size_t
     */
    [[nodiscard]] const std::size_t &GetIndex() const;

    /**
     * @brief Method for set index
     */
    void SetIndex(const std::size_t &);

    /**
     * @brief Method for get total
     * @return const std::size_t
     */
    [[nodiscard]] const std::size_t &GetTotal() const;

    /**
     * @brief Method for set total
     */
    inline void SetTotal(const std::size_t &total);
    /**
     * @brief Method for get method
     * @return Method
     */
    [[nodiscard]] Method GetMethod() const;
    /*
     * @brief Method for set method
     */
    void SetMethod(Method);

    /**
     * @brief Method for get path
     * @return std::filesystem::path
     */
    [[nodiscard]] std::filesystem::path GetPath() const;

    /*
     * @brief Method for set Path
     */
    void SetPath(std::filesystem::path);

    /**
     * @brief Method for get data
     * @return std::string
     */
    [[nodiscard]] const std::vector<char> &GetData() const;

    /**
     * @brief Method for set data
     */
    void SetData(const std::vector<char> &);

    /**
     * @brief print protoBuf with ostream
     */
    friend std::ostream &operator<<(std::ostream &os, const ProtoBuf &protoBuf);

    /**
     * @brief print protoBuf with istream
     */
    friend std::istream &operator>>(std::istream &is, const ProtoBuf &protoBuf);

   private:
    std::size_t size;
    std::size_t headsize;
    bool isFile = false;
    std::size_t index = 0;
    std::size_t total = 0;
    Method method;
    std::filesystem::path path;
    std::vector<char> data;
};

inline ProtoBuf::ProtoBuf(const Method &method,
                          const std::filesystem::path &path,
                          const std::vector<char> &data) {
    this->method = method;
    this->path = path;
    this->data = data;

    this->headsize = 4 * sizeof(std::size_t) +
                     ProtoBuf::MethodToString(method).size() +
                     path.string().size() + 3;
    this->size = headsize + data.size();
}

inline std::string ProtoBuf::MethodToString(const Method &method) {
    switch (method) {
        case Method::Query:
            return "QUERY";
        case Method::Get:
            return "GET";
        case Method::Post:
            return "POST";
        case Method::Delete:
            return "DELETE";
        default:
            throw std::runtime_error("Unknown method");
    }
}

inline ProtoBuf::Method ProtoBuf::StringToMethod(std::string &string) {
    if (string == "QUERY")
        return Method::Query;
    else if (string == "GET")
        return Method::Get;
    else if (string == "POST")
        return Method::Post;
    else if (string == "DELETE")
        return Method::Delete;
    else
        throw std::runtime_error("Unknown method");
}

inline const std::size_t &ProtoBuf::GetSize() const { return this->size; }

inline void ProtoBuf::SetSize(const std::size_t &size) { this->size = size; }

inline const std::size_t &ProtoBuf::GetHeadSize() const {
    return this->headsize;
}

inline void ProtoBuf::SetHeadSize(const std::size_t &headsize) {
    this->headsize = headsize;
}

inline const bool &ProtoBuf::GetIsFile() const { return this->isFile; }

inline void ProtoBuf::SetIsFile(const bool &isFile) { this->isFile = isFile; }

inline const std::size_t &ProtoBuf::GetIndex() const { return this->index; }

inline void ProtoBuf::SetIndex(const std::size_t &index) {
    this->index = index;
}

inline const std::size_t &ProtoBuf::GetTotal() const { return this->total; }

inline void ProtoBuf::SetTotal(const std::size_t &total) {
    this->total = total;
}

inline ProtoBuf::Method ProtoBuf::GetMethod() const { return method; }

inline void ProtoBuf::SetMethod(Method method) { this->method = method; }

inline std::filesystem::path ProtoBuf::GetPath() const { return path; }

inline void ProtoBuf::SetPath(std::filesystem::path path) { this->path = path; }

inline const std::vector<char> &ProtoBuf::GetData() const { return data; }

inline void ProtoBuf::SetData(const std::vector<char> &data) {
    this->data = data;
}

inline std::ostream &operator<<(std::ostream &os, const ProtoBuf &protoBuf) {
    os.write(reinterpret_cast<const char *>(&protoBuf.GetSize()),
             sizeof(std::size_t));
    os.write(reinterpret_cast<const char *>(&protoBuf.GetHeadSize()),
             sizeof(std::size_t));
    os.write(reinterpret_cast<const char *>(&protoBuf.GetIsFile()),
             sizeof(bool));
    os.write(reinterpret_cast<const char *>(&protoBuf.GetIndex()),
             sizeof(std::size_t));
    os.write(reinterpret_cast<const char *>(&protoBuf.GetTotal()),
             sizeof(std::size_t));
    os << ProtoBuf::MethodToString(protoBuf.method) + " " +
              protoBuf.path.string() + " ";

    const auto &data = protoBuf.GetData();
    os.write(data.data(), data.size());
    return os;
}

inline std::istream &operator>>(std::istream &is, ProtoBuf &protoBuf) {
    std::size_t size;
    std::size_t headsize;
    bool isFile;
    std::size_t index;
    std::size_t total;
    std::string method;
    std::filesystem::path path;
    is.read(reinterpret_cast<char *>(&size), sizeof(std::size_t));
    is.read(reinterpret_cast<char *>(&headsize), sizeof(std::size_t));
    is.read(reinterpret_cast<char *>(&isFile), sizeof(bool));
    is.read(reinterpret_cast<char *>(&index), sizeof(std::size_t));
    is.read(reinterpret_cast<char *>(&total), sizeof(std::size_t));

    is >> method >> path;
    is.ignore();
    std::vector<char> data(size - headsize);
    is.read(&data[0], size - headsize);

    protoBuf.SetSize(size);
    protoBuf.SetHeadSize(headsize);
    protoBuf.SetIsFile(isFile);
    protoBuf.SetIndex(index);
    protoBuf.SetTotal(total);
    protoBuf.SetMethod(ProtoBuf::StringToMethod(method));
    protoBuf.SetPath(path);
    protoBuf.SetData(data);
    return is;
}
