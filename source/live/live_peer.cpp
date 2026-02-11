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

#include "live/live_peer.h"
#include "app/managers/version_manager.h"
#include "live/live_server.h"
#include "live/live_tab.h"
#include "live/live_action.h"

#include "editor/editor.h"
#include "editor/action_queue.h"

LivePeer::LivePeer(LiveServer* server, boost::asio::ip::tcp::socket socket) :
	LiveSocket(),
	readMessage(), server(server), socket(std::move(socket)), color(), id(0), clientId(0), connected(false) {
	ASSERT(server != nullptr);
}

LivePeer::~LivePeer() {
	if (socket.is_open()) {
		socket.close();
	}
}

void LivePeer::close() {
	server->removeClient(id);
}

bool LivePeer::handleError(const boost::system::error_code& error) {
	if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
		logMessage(wxString() + getHostName() + ": disconnected.");
		close();
		return true;
	} else if (error == boost::asio::error::connection_aborted) {
		logMessage(name + " have left the server.");
		return true;
	}
	return false;
}

std::string LivePeer::getHostName() const {
	return socket.remote_endpoint().address().to_string();
}

void LivePeer::receiveHeader() {
	readMessage.clear(); // Ensure buffer is ready for header
	boost::asio::async_read(socket, boost::asio::buffer(readMessage.buffer, 4), [weak_self = std::weak_ptr<LivePeer>(shared_from_this())](const boost::system::error_code& error, size_t bytesReceived) -> void {
		if (auto self = weak_self.lock()) {
			if (error) {
				if (!self->handleError(error)) {
					self->logMessage(wxString() + self->getHostName() + ": " + error.message());
				}
			} else if (bytesReceived < 4) {
				self->logMessage(wxString() + self->getHostName() + ": Could not receive header[size: " + std::to_string(bytesReceived) + "], disconnecting client.");
			} else {
				self->receive(self->readMessage.read<uint32_t>());
			}
		}
	});
}

void LivePeer::receive(uint32_t packetSize) {
	if (packetSize > 10 * 1024 * 1024) { // 10 MB limit
		logMessage(wxString() + getHostName() + ": Packet too large (" + std::to_string(packetSize) + "), disconnecting client.");
		close();
		return;
	}

	readMessage.buffer.resize(readMessage.position + packetSize);
	boost::asio::async_read(socket, boost::asio::buffer(&readMessage.buffer[readMessage.position], packetSize), [weak_self = std::weak_ptr<LivePeer>(shared_from_this())](const boost::system::error_code& error, size_t bytesReceived) -> void {
		if (auto self = weak_self.lock()) {
			if (error) {
				if (!self->handleError(error)) {
					self->logMessage(wxString() + self->getHostName() + ": " + error.message());
				}
			} else if (bytesReceived < self->readMessage.buffer.size() - 4) {
				self->logMessage(wxString() + self->getHostName() + ": Could not receive packet[size: " + std::to_string(bytesReceived) + "], disconnecting client.");
			} else {
				wxTheApp->CallAfter([weak_self]() {
					if (auto self = weak_self.lock()) {
						try {
							if (self->connected) {
								self->parseEditorPacket(std::move(self->readMessage));
							} else {
								self->parseLoginPacket(std::move(self->readMessage));
							}
							self->receiveHeader();
						} catch (std::exception& e) {
							self->logMessage(wxString() + self->getHostName() + ": Exception in packet handling: " + e.what());
							self->close();
						}
					}
				});
			}
		}
	});
}

void LivePeer::send(NetworkMessage& message) {
	memcpy(&message.buffer[0], &message.size, 4);
	boost::asio::async_write(socket, boost::asio::buffer(message.buffer, message.size + 4), [weak_self = std::weak_ptr<LivePeer>(shared_from_this())](const boost::system::error_code& error, size_t bytesTransferred) -> void {
		if (auto self = weak_self.lock()) {
			if (error) {
				self->logMessage(wxString() + self->getHostName() + ": " + error.message());
			}
		}
	});
}

void LivePeer::parseLoginPacket(NetworkMessage message) {
	uint8_t packetType;
	while (message.position < message.buffer.size()) {
		packetType = message.read<uint8_t>();
		switch (packetType) {
			case PACKET_HELLO_FROM_CLIENT:
				parseHello(message);
				break;
			case PACKET_READY_CLIENT:
				parseReady(message);
				break;
			default: {
				log->Message("Invalid login packet receieved, connection severed.");
				close();
				break;
			}
		}
	}
}

void LivePeer::parseEditorPacket(NetworkMessage message) {
	uint8_t packetType;
	while (message.position < message.buffer.size()) {
		packetType = message.read<uint8_t>();
		switch (packetType) {
			case PACKET_REQUEST_NODES:
				parseNodeRequest(message);
				break;
			case PACKET_CHANGE_LIST:
				parseReceiveChanges(message);
				break;
			case PACKET_ADD_HOUSE:
				parseAddHouse(message);
				break;
			case PACKET_EDIT_HOUSE:
				parseEditHouse(message);
				break;
			case PACKET_REMOVE_HOUSE:
				parseRemoveHouse(message);
				break;
			case PACKET_CLIENT_UPDATE_CURSOR:
				parseCursorUpdate(message);
				break;
			case PACKET_CLIENT_TALK:
				parseChatMessage(message);
				break;
			default: {
				log->Message("Invalid editor packet receieved, connection severed.");
				close();
				break;
			}
		}
	}
}

void LivePeer::parseHello(NetworkMessage& message) {
	if (connected) {
		close();
		return;
	}

	uint32_t rmeVersion = message.read<uint32_t>();
	if (rmeVersion != __RME_VERSION_ID__) {
		NetworkMessage outMessage;
		outMessage.write<uint8_t>(PACKET_KICK);
		outMessage.write<std::string>("Wrong editor version.");

		send(outMessage);
		close();
		return;
	}

	uint32_t netVersion = message.read<uint32_t>();
	if (netVersion != __LIVE_NET_VERSION__) {
		NetworkMessage outMessage;
		outMessage.write<uint8_t>(PACKET_KICK);
		outMessage.write<std::string>("Wrong protocol version.");

		send(outMessage);
		close();
		return;
	}

	uint32_t clientVersion = message.read<uint32_t>();
	std::string nickname = message.read<std::string>();
	std::string password = message.read<std::string>();

	if (server->getPassword() != wxString(password.c_str(), wxConvUTF8)) {
		log->Message("Client tried to connect, but used the wrong password, connection refused.");
		close();
		return;
	}

	name = wxString(nickname.c_str(), wxConvUTF8);
	log->Message(name + " (" + getHostName() + ") connected.");

	NetworkMessage outMessage;
	if (static_cast<ClientVersionID>(clientVersion) != g_version.GetCurrentVersionID()) {
		outMessage.write<uint8_t>(PACKET_CHANGE_CLIENT_VERSION);
		outMessage.write<uint32_t>(g_version.GetCurrentVersionID());
	} else {
		outMessage.write<uint8_t>(PACKET_ACCEPTED_CLIENT);
	}
	send(outMessage);
}

void LivePeer::parseReady(NetworkMessage& message) {
	if (connected) {
		close();
		return;
	}

	connected = true;

	// Find free client id
	clientId = server->getFreeClientId();
	if (clientId == 0) {
		NetworkMessage outMessage;
		outMessage.write<uint8_t>(PACKET_KICK);
		outMessage.write<std::string>("Server is full.");

		send(outMessage);
		close();
		return;
	}

	server->updateClientList();

	// Let's reply
	NetworkMessage outMessage;
	outMessage.write<uint8_t>(PACKET_HELLO_FROM_SERVER);

	Map& map = server->getEditor()->map;
	outMessage.write<std::string>(map.getName());
	outMessage.write<uint16_t>(map.getWidth());
	outMessage.write<uint16_t>(map.getHeight());

	send(outMessage);
}

void LivePeer::parseNodeRequest(NetworkMessage& message) {
	Map& map = server->getEditor()->map;
	uint32_t nodes = message.read<uint32_t>();
	if (nodes > 65536) {
		log->Message("Client requested too many nodes, connection severed.");
		close();
		return;
	}

	for (; nodes != 0; --nodes) {
		uint32_t ind = message.read<uint32_t>();

		int32_t ndx = ind >> 18;
		int32_t ndy = (ind >> 4) & 0x3FFF;
		bool underground = ind & 1;

		MapNode* node = map.createLeaf(ndx * 4, ndy * 4);
		if (node) {
			sendNode(clientId, node, ndx, ndy, underground ? 0xFF00 : 0x00FF);
		}
	}
}

void LivePeer::parseReceiveChanges(NetworkMessage& message) {
	Editor& editor = *server->getEditor();

	// -1 on address since we skip the first START_NODE when sending
	const std::string& data = message.read<std::string>();
	mapReader.assign(reinterpret_cast<const uint8_t*>(data.c_str() - 1), data.size());

	BinaryNode* rootNode = mapReader.getRootNode();
	BinaryNode* tileNode = rootNode->getChild();

	// We need ownership of the action, but createAction returns unique_ptr.
	// We'll move it into a temporary unique_ptr, get the raw pointer for metadata, then move to queue.
	std::unique_ptr<Action> rawAction = editor.actionQueue->createAction(ACTION_REMOTE);
	NetworkedAction* action = static_cast<NetworkedAction*>(rawAction.get());
	action->owner = clientId;

	if (tileNode) {
		do {
			Tile* tile = readTile(tileNode, editor, nullptr);
			if (tile) {
				action->addChange(std::make_unique<Change>(tile));
			}
		} while (tileNode->advance());
	}
	mapReader.close();

	editor.actionQueue->addAction(std::move(rawAction));

	g_gui.RefreshView();
	g_gui.UpdateMinimap();
}

void LivePeer::parseAddHouse(NetworkMessage& message) {
}

void LivePeer::parseEditHouse(NetworkMessage& message) {
}

void LivePeer::parseRemoveHouse(NetworkMessage& message) {
}

void LivePeer::parseCursorUpdate(NetworkMessage& message) {
	LiveCursor cursor = readCursor(message);
	cursor.id = clientId;

	if (cursor.color != color) {
		setUsedColor(cursor.color);
		server->updateClientList();
	}

	server->broadcastCursor(cursor);
	g_gui.RefreshView();
}

void LivePeer::parseChatMessage(NetworkMessage& message) {
	const std::string& chatMessage = message.read<std::string>();
	server->broadcastChat(name, wxstr(chatMessage));
}
