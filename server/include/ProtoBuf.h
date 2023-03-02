#pragma once

#include <spdlog/spdlog.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

using namespace spdlog;

class ProtoBuf {
   public:
    enum class [[nodiscard]] Method {
        Query,
        Get,
        Post,
        Delete,
    };

    [[nodiscard]] const std::string toString() const;

    static std::string MethodToString(const Method &);
    static Method StringToMethod(std::string &);
    friend std::ostream &operator<<(std::ostream &os, const ProtoBuf &protoBuf);
    friend std::istream &operator>>(std::istream &is, const ProtoBuf &protoBuf);

    ProtoBuf(const Method &method, const std::filesystem::path &path,
             const std::vector<char> &data);
    ProtoBuf() = default;
    ProtoBuf(const ProtoBuf &) = default;
    ProtoBuf(ProtoBuf &&) = default;
    ProtoBuf &operator=(const ProtoBuf &) = default;
    ProtoBuf &operator=(ProtoBuf &&) = default;

    [[nodiscard]] const std::size_t &GetSize() const;
    [[nodiscard]] const std::size_t &GetHeadSize() const;
    [[nodiscard]] const bool &GetIsDir() const;
    [[nodiscard]] const bool &GetIsFile() const;
    [[nodiscard]] const std::size_t &GetIndex() const;
    [[nodiscard]] const std::size_t &GetTotal() const;
    [[nodiscard]] Method GetMethod() const;
    [[nodiscard]] std::filesystem::path GetPath() const;
    [[nodiscard]] const std::vector<char> &GetData() const;
    void SetSize(const std::size_t &);
    void SetHeadSize(const std::size_t &);
    void SetIsDir(const bool &);
    void SetIsFile(const bool &);
    void SetIndex(const std::size_t &);
    inline void SetTotal(const std::size_t &total);
    void SetMethod(Method);
    void SetPath(std::filesystem::path);
    void SetData(const std::vector<char> &);

   private:
    std::size_t size;
    std::size_t headsize;
    bool isDir = false;
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
                     path.string().size() + 4;
    this->size = headsize + data.size();
}

inline const std::string ProtoBuf::toString() const {
    return "protobuf " + std::to_string(size) + " " + std::to_string(headsize) +
           " " + std::to_string(isDir) + " " + std::to_string(isFile) + " " +
           std::to_string(index) + " " + std::to_string(total) + " " +
           MethodToString(method) + " " + path.string();
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

inline std::ostream &operator<<(std::ostream &os, const ProtoBuf &protoBuf) {
    os.write(reinterpret_cast<const char *>(&protoBuf.GetSize()),
             sizeof(std::size_t));
    os.write(reinterpret_cast<const char *>(&protoBuf.GetHeadSize()),
             sizeof(std::size_t));
    os.write(reinterpret_cast<const char *>(&protoBuf.GetIsDir()),
             sizeof(bool));
    os.write(reinterpret_cast<const char *>(&protoBuf.GetIsFile()),
             sizeof(bool));
    os.write(reinterpret_cast<const char *>(&protoBuf.GetIndex()),
             sizeof(std::size_t));
    os.write(reinterpret_cast<const char *>(&protoBuf.GetTotal()),
             sizeof(std::size_t));
    os << ProtoBuf::MethodToString(protoBuf.method) + " " +
              protoBuf.path.string() + " ";

    spdlog::get("logger")->debug("send {}", protoBuf.toString());
    const auto &data = protoBuf.GetData();
    os.write(data.data(), data.size());
    return os;
}

inline std::istream &operator>>(std::istream &is, ProtoBuf &protoBuf) {
    std::size_t size;
    std::size_t headsize;
    bool isFile;
    bool isDir;
    std::size_t index;
    std::size_t total;
    std::string method;
    std::filesystem::path path;
    is.read(reinterpret_cast<char *>(&size), sizeof(std::size_t));
    is.read(reinterpret_cast<char *>(&headsize), sizeof(std::size_t));
    is.read(reinterpret_cast<char *>(&isDir), sizeof(bool));
    is.read(reinterpret_cast<char *>(&isFile), sizeof(bool));
    is.read(reinterpret_cast<char *>(&index), sizeof(std::size_t));
    is.read(reinterpret_cast<char *>(&total), sizeof(std::size_t));
    is >> method >> path;
    is.ignore();
    std::vector<char> data(size - headsize);
    is.read(&data[0], size - headsize);

    protoBuf.SetSize(size);
    protoBuf.SetHeadSize(headsize);
    protoBuf.SetIsDir(isDir);
    protoBuf.SetIsFile(isFile);
    protoBuf.SetIndex(index);
    protoBuf.SetTotal(total);
    protoBuf.SetMethod(ProtoBuf::StringToMethod(method));
    protoBuf.SetPath(path);
    protoBuf.SetData(data);
    spdlog::get("logger")->debug("recv {}", protoBuf.toString());
    return is;
}

inline const std::size_t &ProtoBuf::GetSize() const { return this->size; }

inline void ProtoBuf::SetSize(const std::size_t &size) { this->size = size; }

inline const std::size_t &ProtoBuf::GetHeadSize() const {
    return this->headsize;
}

inline void ProtoBuf::SetHeadSize(const std::size_t &headsize) {
    this->headsize = headsize;
}

inline const bool &ProtoBuf::GetIsDir() const { return this->isDir; }

inline void ProtoBuf::SetIsDir(const bool &isDir) { this->isDir = isDir; }

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
