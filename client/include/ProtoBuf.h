
#pragma once

#include <array>
#include <filesystem>

#include "ProtoBuf.h"

#define SIZE 65536
#define PATHLENGTH 256

/* @brief Class for ProtoBuf
 *
 * protoBuf
 * { method }{ filepath } { data }
 */
class ProtoBuf {
   private:
    std::array<uint8_t, SIZE> protoBuf;

   public:
    /* @brief Method enum */
    enum class Method {
        Get,
        Post,
        Delete,
    };

    ProtoBuf() = default;

    ProtoBuf(const ProtoBuf &) = default;

    ProtoBuf(ProtoBuf &&) = default;

    ProtoBuf &operator=(const ProtoBuf &) = default;

    ProtoBuf &operator=(ProtoBuf &&) = default;

    /* @brief Method toString
     * @param Method
     */
    static std::string MethodToString(const Method &method);

    /* @brief Method for get protoBuf
     * @return U
     */
    template <typename U>
    [[nodiscard]] U GetProtoBuf() const;

    /* @brief Method for get method
     * @return Method
     */
    [[nodiscard]] Method GetMethod() const;

    /* @brief Method for get path
     * @return std::filesystem::path
     */
    [[nodiscard]] std::filesystem::path GetPath() const;

    /* @brief Method for get data
     * @return U
     */
    template <typename U>
    [[nodiscard]] U GetData() const;

    /* @brief Method for set protoBuf */
    template <typename U>
    void SetProtoBuf(U);

    /* @brief Method for set method */
    void SetMethod(Method);

    /* @brief Method for set path */
    void SetPath(std::filesystem::path);

    /* @brief Method for set data */
    template <typename U>
    void SetData(U);
};

std::string ProtoBuf::MethodToString(const Method &method) {
    switch (method) {
        case Method::Get:
            return "GET";
        case Method::Post:
            return "POST";
        case Method::Delete:
            return "DELETE";
        default:
            return "UNKNOWN";
    }
}

template <typename U>
U ProtoBuf::GetProtoBuf() const {
    U returnProtoBuf;
    std::memcpy(&returnProtoBuf, protoBuf.data(), sizeof(U));
    return returnProtoBuf;
}

ProtoBuf::Method ProtoBuf::GetMethod() const {
    Method method;
    std::copy(protoBuf.begin(), protoBuf.begin() + sizeof(Method),
              reinterpret_cast<uint8_t *>(&method));
    return method;
}

std::filesystem::path ProtoBuf::GetPath() const {
    std::array<char, PATHLENGTH> path;
    std::copy(protoBuf.begin() + sizeof(Method),
              protoBuf.begin() + sizeof(Method) + PATHLENGTH, path.begin());
    return {path.data()};
}

template <typename U>
U ProtoBuf::GetData() const {
    U realData;
    std::copy(protoBuf.begin() + sizeof(Method) + PATHLENGTH, protoBuf.end(),
              realData.begin());
    return realData;
}

template <typename U>
void ProtoBuf::SetProtoBuf(U protoBuf) {
    std::copy(protoBuf.begin(), protoBuf.end(), this->protoBuf.begin());
}

void ProtoBuf::SetMethod(Method method) {
    std::copy(reinterpret_cast<uint8_t *>(&method),
              reinterpret_cast<uint8_t *>(&method) + sizeof(Method),
              protoBuf.begin());
}

void ProtoBuf::SetPath(std::filesystem::path path) {
	// NOTE: make sure path takes PATHLENGTH bytes
	std::string tmp = path.string();
	tmp.resize(PATHLENGTH,'\0');
    std::copy(tmp.begin(), tmp.end(),
              protoBuf.begin() + sizeof(Method));
}
template <typename U>
void ProtoBuf::SetData(U data) {
    std::copy(data.begin(), data.end(),
              this->protoBuf.begin() + sizeof(Method) + PATHLENGTH);
}
