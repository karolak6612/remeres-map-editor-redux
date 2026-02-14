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

#include "app/main.h"
#include "net/net_connection.h"
#include <spdlog/spdlog.h>

// NetworkConnection
NetworkConnection::NetworkConnection() :
	service(nullptr), thread(), stopped(false) {
	//
}

NetworkConnection::~NetworkConnection() {
	stop();
}

NetworkConnection& NetworkConnection::getInstance() {
	static NetworkConnection connection;
	return connection;
}

bool NetworkConnection::start() {
	std::lock_guard<std::mutex> lock(connection_mutex);

	if (thread.joinable()) {
		if (stopped) {
			return false;
		}
		return true;
	}

	stopped = false;
	if (!service) {
		service = std::make_unique<boost::asio::io_context>();
	}

	thread = std::thread([this]() -> void {
		boost::asio::io_context& serviceRef = *service;
		try {
			while (!stopped) {
				serviceRef.run_one();
				serviceRef.restart();
			}
		} catch (std::exception& e) {
			spdlog::error("{}", e.what());
		}
	});
	return true;
}

void NetworkConnection::stop() {
	std::unique_lock<std::mutex> lock(connection_mutex);

	if (!service) {
		return;
	}

	spdlog::info("NetworkConnection::stop started");
	spdlog::default_logger()->flush();

	service->stop();
	stopped = true;

	if (thread.joinable()) {
		lock.unlock();
		spdlog::info("NetworkConnection::stop - joining thread");
		spdlog::default_logger()->flush();
		thread.join();
		spdlog::info("NetworkConnection::stop - thread joined");
		spdlog::default_logger()->flush();
		lock.lock();
	}

	service.reset();

	spdlog::info("NetworkConnection::stop finished");
	spdlog::default_logger()->flush();
}

boost::asio::io_context& NetworkConnection::get_service() {
	return *service;
}
