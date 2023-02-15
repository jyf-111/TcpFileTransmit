#pragma once

#include <array>
#include <filesystem>
#include <iterator>
#include <string>

/* @brief Class for ProtoBuf
 *
 * protoBuf
 * { method }{ filepath } { data }
 */
class ProtoBuf {
   public:
    /* @brief Method enum */
    enum class Method {
        Get,
        Post,
        Delete,
    };
    ProtoBuf(Method method, std::filesystem::path path, std::string data);
    ProtoBuf() = default;
    ProtoBuf(const ProtoBuf &) = default;
    ProtoBuf(ProtoBuf &&) = default;
    ProtoBuf &operator=(const ProtoBuf &) = default;
    ProtoBuf &operator=(ProtoBuf &&) = default;

    /**
     * @brief Method toString
     * @param Method
     */
    static std::string MethodToString(const Method &method);

    /**
     * @brief String to Method
     * @param std::string
     */
    static Method StringToMethod(std::string &string);

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
    [[nodiscard]] std::string GetData() const;

    /**
     * @brief Method for set data
     */
    void SetData(std::string);

    /**
     * @brief print protoBuf with ostream
     */
    friend std::ostream &operator<<(std::ostream &os, const ProtoBuf &protoBuf);

    /**
     * @brief print protoBuf with istream
     */
    friend std::istream &operator>>(std::istream &is, const ProtoBuf &protoBuf);

   private:
    Method method;
    std::filesystem::path path;
    std::string data;
};

inline ProtoBuf::ProtoBuf(Method method, std::filesystem::path path,
                          std::string data) {
    this->method = method;
    this->path = std::move(path);
    this->data = std::move(data);
}

inline std::string ProtoBuf::MethodToString(const Method &method) {
    switch (method) {
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
    if (string == "GET")
        return Method::Get;
    else if (string == "POST")
        return Method::Post;
    else if (string == "DELETE")
        return Method::Delete;
    else
        throw std::runtime_error("Unknown method");
}

inline ProtoBuf::Method ProtoBuf::GetMethod() const { return method; }

inline void ProtoBuf::SetMethod(Method method) { this->method = method; }

inline std::filesystem::path ProtoBuf::GetPath() const { return path; }

inline void ProtoBuf::SetPath(std::filesystem::path path) { this->path = path; }

inline std::string ProtoBuf::GetData() const { return data; }

inline void ProtoBuf::SetData(std::string data) { this->data = data; }

inline std::ostream &operator<<(std::ostream &os, const ProtoBuf &protoBuf) {
    return os << ProtoBuf::MethodToString(protoBuf.method) << " "
              << protoBuf.path << " " << protoBuf.data << '\n';
}

inline std::istream &operator>>(std::istream &is, ProtoBuf &protoBuf) {
    std::string method;
    std::filesystem::path path;
    std::string data;
    is >> method >> path;
    data = {std::istreambuf_iterator<char>(is),
            std::istreambuf_iterator<char>()};

    protoBuf.SetMethod(ProtoBuf::StringToMethod(method));
    protoBuf.SetPath(path);
    protoBuf.SetData(data);
    return is;
}
