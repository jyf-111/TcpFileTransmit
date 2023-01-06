
#include "ProtoBuf.h"

#include <array>
#include <iostream>

typename ProtoBuf::Method ProtoBuf::GetMethod() const {
    Method method;
    std::copy(protoBuf.begin(), protoBuf.begin() + sizeof(Method),
              reinterpret_cast<uint8_t *>(&method));
    return method;
}

std::array<char, SIZE> ProtoBuf::GetData() const {
    std::array<char, SIZE> realData;
    std::copy(protoBuf.begin() + sizeof(Method), protoBuf.end(),
              realData.begin());
    return realData;
}

void ProtoBuf::SetMethod(Method method) {
    std::copy(reinterpret_cast<uint8_t *>(&method),
              reinterpret_cast<uint8_t *>(&method) + sizeof(Method),
              protoBuf.begin());
}

void ProtoBuf::SetData(std::array<char, SIZE> data) {
    std::copy(data.begin(), data.end(),
              this->protoBuf.begin() + sizeof(Method));
}

std::array<char, SIZE> ProtoBuf::GetProtoBuf() const {
    std::array<char, SIZE> realData;
    std::copy(protoBuf.begin(), protoBuf.end(), realData.begin());
    return realData;
}

void ProtoBuf::SetProtoBuf(std::array<char, SIZE> protoBuf) {
    this->protoBuf = protoBuf;
}
