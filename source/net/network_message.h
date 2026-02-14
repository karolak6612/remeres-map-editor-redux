#ifndef _RME_NETWORK_MESSAGE_H_
#define _RME_NETWORK_MESSAGE_H_

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include "map/position.h"

struct NetworkMessage {
    NetworkMessage();

    void clear();
    void expand(const size_t length);

    template <typename T>
    T read() {
        if (position + sizeof(T) > buffer.size()) {
             throw std::runtime_error("NetworkMessage::read: Buffer underflow");
        }
        T value;
        std::memcpy(&value, &buffer[position], sizeof(T));
        position += sizeof(T);
        return value;
    }

    template <typename T>
    void write(const T& value) {
        expand(sizeof(T));
        std::memcpy(&buffer[position], &value, sizeof(T));
        position += sizeof(T);
    }

    std::vector<uint8_t> buffer;
    size_t position;
    size_t size;
};

template <>
std::string NetworkMessage::read<std::string>();

template <>
Position NetworkMessage::read<Position>();

template <>
void NetworkMessage::write<std::string>(const std::string& value);

template <>
void NetworkMessage::write<Position>(const Position& value);

#endif
