#pragma once

#include <array>

#define SIZE 65536
class ProtoBuf {
   private:
    std::array<uint8_t, SIZE> protoBuf;

   public:
    enum class Method { Get, Post, Delete };

    ProtoBuf() = default;

    ProtoBuf(const ProtoBuf &) = default;

    ProtoBuf(ProtoBuf &&) = default;

    ProtoBuf &operator=(const ProtoBuf &) = default;

    ProtoBuf &operator=(ProtoBuf &&) = default;

    template <typename U>
    [[nodiscard]] U GetProtoBuf() const;

    [[nodiscard]] Method GetMethod() const;

    template <typename U>
    [[nodiscard]] U GetData() const;

    template <typename U>
    void SetProtoBuf(U);

    void SetMethod(Method);

    template <typename U>
    void SetData(U);
};

template <typename U>
U ProtoBuf::GetProtoBuf() const {
    return reinterpret_cast<U>(protoBuf);
}

typename ProtoBuf::Method ProtoBuf::GetMethod() const {
    Method method;
    std::copy(protoBuf.begin(), protoBuf.begin() + sizeof(Method),
              reinterpret_cast<uint8_t *>(&method));
    return method;
}

template <typename U>
U ProtoBuf::GetData() const {
    U realData;
    std::copy(protoBuf.begin() + sizeof(Method), protoBuf.end(),
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

template <typename U>
void ProtoBuf::SetData(U data) {
    std::copy(data.begin(), data.end(),
              this->protoBuf.begin() + sizeof(Method));
}
