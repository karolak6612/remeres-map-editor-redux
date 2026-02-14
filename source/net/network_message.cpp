#include "net/network_message.h"
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>

NetworkMessage::NetworkMessage() {
	clear();
}

void NetworkMessage::clear() {
	buffer.resize(4);
	position = 4;
	size = 0;
}

void NetworkMessage::expand(const size_t length) {
	if (position + length >= buffer.size()) {
		buffer.resize(position + length + 1);
	}
	size += length;
}

template <>
std::string NetworkMessage::read<std::string>() {
	const uint16_t length = read<uint16_t>();
	if (position + length > buffer.size()) {
		throw std::runtime_error("NetworkMessage::read<std::string>: Buffer underflow");
	}
	std::string value(reinterpret_cast<char*>(&buffer[position]), length);
	position += length;
	return value;
}

template <>
Position NetworkMessage::read<Position>() {
	Position position_val;
	position_val.x = read<uint16_t>();
	position_val.y = read<uint16_t>();
	position_val.z = read<uint8_t>();
	return position_val;
}

template <>
void NetworkMessage::write<std::string>(const std::string& value) {
	const size_t length = value.length();
	write<uint16_t>(static_cast<uint16_t>(length));

	expand(length);
	std::memcpy(&buffer[position], value.data(), length);
	position += length;
}

template <>
void NetworkMessage::write<Position>(const Position& value) {
	write<uint16_t>(value.x);
	write<uint16_t>(value.y);
	write<uint8_t>(value.z);
}
