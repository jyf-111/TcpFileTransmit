#pragma once

#include <array>
#define SIZE 65536

template <typename T>
class ProtoBuf {
   private:
    std::array<T, SIZE> protoBuf;

   public:
    enum class Method { Get, Post, Delete };

    ProtoBuf() = default;

    ProtoBuf(const ProtoBuf &) = default;

    ProtoBuf(ProtoBuf &&) = default;

    ProtoBuf &operator=(const ProtoBuf &) = default;

    ProtoBuf &operator=(ProtoBuf &&) = default;

    [[nodiscard]] Method GetMethod() const;

    void SetMethod(Method);

    [[nodiscard]] std::array<T, SIZE> GetData() const;

    void SetData(std::array<T, SIZE>);

    [[nodiscard]] std::array<T, SIZE> GetProtoBuf() const;

    void SetProtoBuf(std::array<T, SIZE>);
};

template <typename T>
typename ProtoBuf<T>::Method ProtoBuf<T>::GetMethod() const {
    Method method;
    std::copy(protoBuf.begin(), protoBuf.begin() + sizeof(Method),
              reinterpret_cast<uint8_t *>(&method));
    return method;
}

template <typename T>
std::array<T, SIZE> ProtoBuf<T>::GetData() const {
    std::array<T, SIZE> realData;
    std::copy(protoBuf.begin() + sizeof(Method), protoBuf.end(),
              realData.begin());
    return realData;
}

template <typename T>
void ProtoBuf<T>::SetMethod(Method method) {
    std::copy(reinterpret_cast<uint8_t *>(&method),
              reinterpret_cast<uint8_t *>(&method) + sizeof(Method),
              protoBuf.begin());
}

template <typename T>
void ProtoBuf<T>::SetData(std::array<T, SIZE> data) {
    std::copy(data.begin(), data.end(),
              this->protoBuf.begin() + sizeof(Method));
}


template <typename T>
std::array<T, SIZE> ProtoBuf<T>::GetProtoBuf() const {
    std::array<T, SIZE> realData;
    std::copy(protoBuf.begin(), protoBuf.end(), realData.begin());
    return realData;
}

template <typename T>
void ProtoBuf<T>::SetProtoBuf(std::array<T, SIZE> protoBuf) {
    this->protoBuf = protoBuf;
}
