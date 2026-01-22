//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "logging/logger.h"
#include "net_connection.h"

NetworkMessage::NetworkMessage() :
	position(0), size(0) { }

void NetworkMessage::clear() {
	buffer.clear();
	position = 0;
	size = 0;
}

void NetworkMessage::expand(const size_t length) {
	if (position + length > buffer.size()) {
		buffer.resize(position + length);
	}
	size = std::max(size, position + length);
}

template <>
std::string NetworkMessage::read<std::string>() {
	uint16_t len = read<uint16_t>();
	if (position + len > buffer.size()) {
		return std::string();
	}
	std::string str((char*)&buffer[position], len);
	position += len;
	return str;
}

template <>
void NetworkMessage::write<std::string>(const std::string& value) {
	uint16_t len = (uint16_t)value.length();
	write<uint16_t>(len);
	expand(len);
	memcpy(&buffer[position], value.c_str(), len);
	position += len;
}

template <>
Position NetworkMessage::read<Position>() {
	Position pos;
	pos.x = read<uint16_t>();
	pos.y = read<uint16_t>();
	pos.z = read<uint8_t>();
	return pos;
}

template <>
void NetworkMessage::write<Position>(const Position& value) {
	write<uint16_t>(value.x);
	write<uint16_t>(value.y);
	write<uint8_t>(value.z);
}

NetworkConnection::NetworkConnection() :
	service(nullptr), stopped(false) { }

NetworkConnection::~NetworkConnection() {
	stop();
}

NetworkConnection& NetworkConnection::getInstance() {
	static NetworkConnection instance;
	return instance;
}

bool NetworkConnection::start() {
	if (service) {
		return false;
	}

	service = new boost::asio::io_context();
	stopped = false;

	thread = std::thread([this]() {
		try {
			// Keep the io_context running even if there is no work
			boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work = boost::asio::make_work_guard(*service);
			service->run();
		} catch (std::exception& e) {
			LOG_ERROR("Network thread exception: {}", e.what());
		}
	});

	return true;
}

void NetworkConnection::stop() {
	if (!service) {
		return;
	}

	service->stop();
	stopped = true;
	if (thread.joinable()) {
		thread.join();
	}

	delete service;
	service = nullptr;
}

boost::asio::io_context& NetworkConnection::get_service() {
	return *service;
}
