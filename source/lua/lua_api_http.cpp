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
#include "lua_api_http.h"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <future>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <string_view>
#include <functional>
#include <unordered_map>
#include <boost/asio.hpp>

namespace LuaAPI {

	// StreamSession class for managing streaming HTTP requests
	class StreamSession {
	public:
		StreamSession() :
			finished_(false), hasError_(false), statusCode_(0) { }

		void appendChunk(const std::string& chunk) {
			std::lock_guard<std::mutex> lock(mutex_);
			chunks_.push(chunk);
			cv_.notify_one();
		}

		std::string getNextChunk() {
			std::lock_guard<std::mutex> lock(mutex_);
			if (chunks_.empty()) {
				return "";
			}
			std::string chunk = chunks_.front();
			chunks_.pop();
			return chunk;
		}

		bool hasChunks() {
			std::lock_guard<std::mutex> lock(mutex_);
			return !chunks_.empty();
		}

		void setFinished() {
			std::lock_guard<std::mutex> lock(mutex_);
			finished_ = true;
			cv_.notify_all();
		}

		bool isFinished() {
			std::lock_guard<std::mutex> lock(mutex_);
			return finished_;
		}

		void setError(const std::string& error) {
			std::lock_guard<std::mutex> lock(mutex_);
			hasError_ = true;
			errorMessage_ = error;
			finished_ = true;
			cv_.notify_all();
		}

		bool hasError() {
			std::lock_guard<std::mutex> lock(mutex_);
			return hasError_;
		}

		std::string getError() {
			std::lock_guard<std::mutex> lock(mutex_);
			return errorMessage_;
		}

		void setStatusCode(int code) {
			statusCode_ = code;
		}

		int getStatusCode() {
			return statusCode_;
		}

		void setHeaders(const cpr::Header& headers) {
			std::lock_guard<std::mutex> lock(mutex_);
			responseHeaders_ = headers;
		}

		cpr::Header getHeaders() {
			std::lock_guard<std::mutex> lock(mutex_);
			return responseHeaders_;
		}

		std::string drainAllChunks() {
			std::lock_guard<std::mutex> lock(mutex_);
			std::string result;
			while (!chunks_.empty()) {
				result += chunks_.front();
				chunks_.pop();
			}
			return result;
		}

	private:
		std::queue<std::string> chunks_;
		std::mutex mutex_;
		std::condition_variable cv_;
		std::atomic<bool> finished_;
		std::atomic<bool> hasError_;
		std::string errorMessage_;
		std::atomic<int> statusCode_;
		cpr::Header responseHeaders_;
	};

	// Global map to store active stream sessions
	static std::unordered_map<uint64_t, std::shared_ptr<StreamSession>> g_streamSessions;
	static std::mutex g_sessionsMutex;
	static uint64_t g_nextSessionId = 1;

	// Security helper: Block localhost and loopback
	static bool isUrlSafe(const std::string& url_str) {
		std::string low = url_str;
		std::transform(low.begin(), low.end(), low.begin(), ::tolower);

		// Basic host extraction
		std::string host = low;
		size_t protocol_pos = host.find("://");
		if (protocol_pos != std::string::npos) {
			host = host.substr(protocol_pos + 3);
		}
		size_t path_pos = host.find_first_of("/?#");
		if (path_pos != std::string::npos) {
			host = host.substr(0, path_pos);
		}
		size_t port_pos = host.find(':');
		if (port_pos != std::string::npos) {
			host = host.substr(0, port_pos);
		}

		if (host.empty()) return false;

		// Resolve host to IP addresses
		try {
			boost::asio::io_context io_context;
			boost::asio::ip::tcp::resolver resolver(io_context);
			boost::asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, "");

			for (auto it = endpoints.begin(); it != endpoints.end(); ++it) {
				boost::asio::ip::address addr = it->endpoint().address();

				// Check if loopback
				if (addr.is_loopback()) return false;

				// Check if unspecified (0.0.0.0 or ::)
				if (addr.is_unspecified()) return false;

				// Check private ranges for IPv4
				if (addr.is_v4()) {
					auto v4 = addr.to_v4().to_bytes();
					// 10.0.0.0/8
					if (v4[0] == 10) return false;
					// 172.16.0.0/12
					if (v4[0] == 172 && (v4[1] >= 16 && v4[1] <= 31)) return false;
					// 192.168.0.0/16
					if (v4[0] == 192 && v4[1] == 168) return false;
					// 169.254.0.0/16 (Link-local)
					if (v4[0] == 169 && v4[1] == 254) return false;
				} else if (addr.is_v6()) {
					// Check unique local or link local for IPv6
					auto v6 = addr.to_v6();
					if (v6.is_link_local() || v6.is_site_local()) return false;
					
					// Unique local address (fc00::/7)
					auto bytes = v6.to_bytes();
					if ((bytes[0] & 0xfe) == 0xfc) return false;
				}
			}
		} catch (...) {
			// If we can't resolve it, we might want to block it just in case,
			// or if it's already an IP that failed to parse, it might be safe to let it fail later.
			// However, most invalid hosts should be blocked.
			return false;
		}

		return true;
	}

	// HTTP GET request
	static sol::table httpGet(sol::this_state ts, const std::string& url, sol::optional<sol::table> optHeaders) {
		sol::state_view lua(ts);
		sol::table result = lua.create_table();

		if (!isUrlSafe(url)) {
			result["error"] = "Security: URL blocked (Localhost access denied)";
			result["ok"] = false;
			result["headers"] = lua.create_table();
			return result;
		}

		cpr::Header headers;
		if (optHeaders) {
			sol::table headersTable = *optHeaders;
			for (auto& pair : headersTable) {
				if (pair.first.is<std::string>() && pair.second.is<std::string>()) {
					headers[pair.first.as<std::string>()] = pair.second.as<std::string>();
				}
			}
		}

		cpr::Response response = cpr::Get(cpr::Url { url }, headers, cpr::Timeout { 10000 });

		result["status"] = static_cast<int>(response.status_code);
		result["body"] = response.text;
		result["error"] = response.error.message;
		result["ok"] = response.status_code >= 200 && response.status_code < 300;

		// Parse response headers
		sol::table respHeaders = lua.create_table();
		for (const auto& h : response.header) {
			respHeaders[h.first] = h.second;
		}
		result["headers"] = respHeaders;

		return result;
	}

	// HTTP POST request
	static sol::table httpPost(sol::this_state ts, const std::string& url, const std::string& body, sol::optional<sol::table> optHeaders) {
		sol::state_view lua(ts);
		sol::table result = lua.create_table();

		if (!isUrlSafe(url)) {
			result["error"] = "Security: URL blocked (Localhost access denied)";
			result["ok"] = false;
			result["headers"] = lua.create_table();
			return result;
		}

		cpr::Header headers;
		if (optHeaders) {
			sol::table headersTable = *optHeaders;
			for (auto& pair : headersTable) {
				if (pair.first.is<std::string>() && pair.second.is<std::string>()) {
					headers[pair.first.as<std::string>()] = pair.second.as<std::string>();
				}
			}
		}

		cpr::Response response = cpr::Post(
			cpr::Url { url },
			cpr::Body { body },
			headers,
			cpr::Timeout { 10000 }
		);

		result["status"] = static_cast<int>(response.status_code);
		result["body"] = response.text;
		result["error"] = response.error.message;
		result["ok"] = response.status_code >= 200 && response.status_code < 300;

		// Parse response headers
		sol::table respHeaders = lua.create_table();
		for (const auto& h : response.header) {
			respHeaders[h.first] = h.second;
		}
		result["headers"] = respHeaders;

		return result;
	}

	// Helper function to convert Lua table to JSON
	static std::function<nlohmann::json(sol::object)> getLuaToJsonConverter() {
		auto luaToJsonPtr = std::make_shared<std::function<nlohmann::json(sol::object)>>();
		auto& luaToJson = *luaToJsonPtr;
		luaToJson = [luaToJsonPtr](sol::object obj) -> nlohmann::json {
			auto& luaToJson = *luaToJsonPtr;
			if (obj.is<bool>()) {
				return obj.as<bool>();
			} else if (obj.is<int>()) {
				return obj.as<int>();
			} else if (obj.is<double>()) {
				return obj.as<double>();
			} else if (obj.is<std::string>()) {
				return obj.as<std::string>();
			} else if (obj.is<sol::table>()) {
				sol::table tbl = obj.as<sol::table>();

				// Check if it's an array (sequential integer keys starting at 1)
				bool isArray = true;
				size_t expectedKey = 1;
				for (auto& pair : tbl) {
					if (!pair.first.is<size_t>() || pair.first.as<size_t>() != expectedKey) {
						isArray = false;
						break;
					}
					expectedKey++;
				}

				if (isArray && expectedKey > 1) {
					nlohmann::json arr = nlohmann::json::array();
					for (auto& pair : tbl) {
						arr.push_back(luaToJson(pair.second));
					}
					return arr;
				} else {
					nlohmann::json jsonObj = nlohmann::json::object();
					for (auto& pair : tbl) {
						std::string key;
						if (pair.first.is<std::string>()) {
							key = pair.first.as<std::string>();
						} else if (pair.first.is<int>()) {
							key = std::to_string(pair.first.as<int>());
						} else {
							continue;
						}
						jsonObj[key] = luaToJson(pair.second);
					}
					return jsonObj;
				}
			} else if (obj.is<sol::nil_t>()) {
				return nullptr;
			}
			return nullptr;
		};
		return luaToJson;
	}

	// HTTP POST with JSON body
	static sol::table httpPostJson(sol::this_state ts, const std::string& url, sol::table jsonBody, sol::optional<sol::table> optHeaders) {
		sol::state_view lua(ts);

		auto luaToJson = getLuaToJsonConverter();
		std::string jsonStr = luaToJson(jsonBody).dump();

		// Add Content-Type header if not present
		sol::table headers = lua.create_table();
		headers["Content-Type"] = "application/json";

		if (optHeaders) {
			for (auto& pair : *optHeaders) {
				if (pair.first.is<std::string>() && pair.second.is<std::string>()) {
					headers[pair.first.as<std::string>()] = pair.second.as<std::string>();
				}
			}
		}

		return httpPost(ts, url, jsonStr, headers);
	}

	// Start a streaming POST request - returns session ID
	static sol::table httpPostStream(sol::this_state ts, const std::string& url, const std::string& body, sol::optional<sol::table> optHeaders) {
		sol::state_view lua(ts);
		sol::table result = lua.create_table();

		if (!isUrlSafe(url)) {
			result["error"] = "Security: URL blocked (Localhost access denied)";
			result["ok"] = false;
			result["headers"] = lua.create_table();
			return result;
		}

		auto session = std::make_shared<StreamSession>();
		uint64_t sessionId;

		{
			std::lock_guard<std::mutex> lock(g_sessionsMutex);
			sessionId = g_nextSessionId++;
			while (g_streamSessions.find(sessionId) != g_streamSessions.end()) {
				sessionId = g_nextSessionId++;
			}
			g_streamSessions[sessionId] = session;
		}

		cpr::Header headers;
		if (optHeaders) {
			sol::table headersTable = *optHeaders;
			for (auto& pair : headersTable) {
				if (pair.first.is<std::string>() && pair.second.is<std::string>()) {
					headers[pair.first.as<std::string>()] = pair.second.as<std::string>();
				}
			}
		}

		// IMPORTANT: Using .detach() here means the background thread lifecycle
		// is independent of the Session object. The lambda captures the session
		// shared_ptr, ensuring it stays alive during the request.
		// Note that cpr::Post cannot be cancelled once started; closing a Session
		// does not stop the in-flight HTTP request. On application shutdown,
		// detached threads may be abruptly terminated.
		std::thread([session, url, body, headers]() {
			std::function<bool(std::string_view, intptr_t)> writeCallback = [session](std::string_view data, intptr_t /*userdata*/) -> bool {
				session->appendChunk(std::string(data));
				return true;
			};

			cpr::Response response = cpr::Post(
				cpr::Url { url },
				cpr::Body { body },
				headers,
				cpr::WriteCallback { writeCallback, 0 },
				cpr::Timeout { 30000 }
			);

			session->setStatusCode(static_cast<int>(response.status_code));
			session->setHeaders(response.header);

			if (response.error) {
				session->setError(response.error.message);
			} else {
				session->setFinished();
			}
		}).detach();

		result["sessionId"] = sessionId;
		result["ok"] = true;

		return result;
	}

	// Start a streaming POST request with JSON body - returns session ID
	static sol::table httpPostJsonStream(sol::this_state ts, const std::string& url, sol::table jsonBody, sol::optional<sol::table> optHeaders) {
		sol::state_view lua(ts);

		auto luaToJson = getLuaToJsonConverter();
		std::string jsonStr = luaToJson(jsonBody).dump();

		// Add Content-Type header if not present
		sol::table headers = lua.create_table();
		headers["Content-Type"] = "application/json";

		if (optHeaders) {
			for (auto& pair : *optHeaders) {
				if (pair.first.is<std::string>() && pair.second.is<std::string>()) {
					headers[pair.first.as<std::string>()] = pair.second.as<std::string>();
				}
			}
		}

		return httpPostStream(ts, url, jsonStr, headers);
	}

	// Read available chunks from a stream session
	static sol::table httpStreamRead(sol::this_state ts, uint64_t sessionId) {
		sol::state_view lua(ts);
		sol::table result = lua.create_table();

		std::shared_ptr<StreamSession> session;
		{
			std::lock_guard<std::mutex> lock(g_sessionsMutex);
			auto it = g_streamSessions.find(sessionId);
			if (it == g_streamSessions.end()) {
				result["ok"] = false;
				result["error"] = "Invalid session ID";
				result["finished"] = true;
				return result;
			}
			session = it->second;
		}

		// Atomic chunk draining
		std::string data = session->drainAllChunks();

		result["data"] = data;
		result["finished"] = session->isFinished();
		result["hasError"] = session->hasError();

		if (session->hasError()) {
			result["error"] = session->getError();
			result["ok"] = false;
		} else {
			result["ok"] = true;
		}

		if (session->isFinished()) {
			result["status"] = session->getStatusCode();

			// Return headers when finished
			sol::table respHeaders = lua.create_table();
			for (const auto& h : session->getHeaders()) {
				respHeaders[h.first] = h.second;
			}
			result["headers"] = respHeaders;
		}

		// Automatic cleanup of finished sessions
		if (session->isFinished() && !session->hasChunks()) {
			std::lock_guard<std::mutex> lock(g_sessionsMutex);
			g_streamSessions.erase(sessionId);
		}

		return result;
	}

	// Close and cleanup a stream session
	static bool httpStreamClose(uint64_t sessionId) {
		std::lock_guard<std::mutex> lock(g_sessionsMutex);
		auto it = g_streamSessions.find(sessionId);
		if (it != g_streamSessions.end()) {
			g_streamSessions.erase(it);
			return true;
		}
		return false;
	}

	// Check if a stream session is finished
	static sol::table httpStreamStatus(sol::this_state ts, uint64_t sessionId) {
		sol::state_view lua(ts);
		sol::table result = lua.create_table();

		std::shared_ptr<StreamSession> session;
		{
			std::lock_guard<std::mutex> lock(g_sessionsMutex);
			auto it = g_streamSessions.find(sessionId);
			if (it == g_streamSessions.end()) {
				result["valid"] = false;
				result["finished"] = true;
				return result;
			}
			session = it->second;
		}

		result["valid"] = true;
		result["finished"] = session->isFinished();
		result["hasError"] = session->hasError();
		result["hasData"] = session->hasChunks();

		if (session->hasError()) {
			result["error"] = session->getError();
		}

		if (session->isFinished()) {
			result["status"] = session->getStatusCode();
		}

		return result;
	}

	void registerHttp(sol::state& lua) {
		sol::table http = lua.create_named_table("http");

		// Basic HTTP methods
		http["get"] = httpGet;
		http["post"] = httpPost;
		http["postJson"] = httpPostJson;

		// Streaming HTTP methods
		http["postStream"] = httpPostStream;
		http["postJsonStream"] = httpPostJsonStream;
		http["streamRead"] = httpStreamRead;
		http["streamClose"] = httpStreamClose;
		http["streamStatus"] = httpStreamStatus;
	}

} // namespace LuaAPI
