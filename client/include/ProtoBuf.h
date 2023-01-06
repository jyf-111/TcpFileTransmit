#pragma once

#include <array>
#define SIZE 65536

class ProtoBuf {
   public:
    std::array<char, SIZE> protoBuf;

    enum class Method { Get, Post, Delete };

    [[nodiscard]] Method GetMethod() const;

    void SetMethod(Method);

    [[nodiscard]] std::array<char, SIZE> GetData() const;

    void SetData(std::array<char, SIZE>);

    [[nodiscard]] std::array<char, SIZE> GetProtoBuf() const;

    void SetProtoBuf(std::array<char, SIZE>);

    ProtoBuf() = default;

    ProtoBuf(const ProtoBuf &) = default;

    ProtoBuf(ProtoBuf &&) = default;

    ProtoBuf &operator=(const ProtoBuf &) = default;

    ProtoBuf &operator=(ProtoBuf &&) = default;
};
