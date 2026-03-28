#include "mcp_server.h"
#include <spdlog/spdlog.h>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <iostream>
#include <wx/app.h> // For wxWakeUpIdle()

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

struct PendingRequest {
	std::string payload;
	std::function<void(const std::string&)> on_complete;
};

class McpServerImpl {
public:
	McpServerImpl() :
		ioc(1),
		acceptor(net::make_strand(ioc)),
		running(false) {
	}

	~McpServerImpl() {
		stop();
	}

	bool start(uint16_t port) {
		if (running) return false;

		try {
			net::ip::tcp::endpoint endpoint{net::ip::make_address("127.0.0.1"), port};
			beast::error_code ec;

			acceptor.open(endpoint.protocol(), ec);
			if (ec) throw beast::system_error{ec};

			acceptor.set_option(net::socket_base::reuse_address(true), ec);
			if (ec) throw beast::system_error{ec};

			acceptor.bind(endpoint, ec);
			if (ec) throw beast::system_error{ec};

			acceptor.listen(net::socket_base::max_listen_connections, ec);
			if (ec) throw beast::system_error{ec};

			running = true;
			do_accept();

			server_thread = std::thread([this]() {
				try {
					ioc.run();
				} catch (const std::exception& e) {
					spdlog::error("MCP Server thread error: {}", e.what());
				}
			});

			spdlog::info("MCP Server started on 127.0.0.1:{}", port);
			return true;
		} catch (const std::exception& e) {
			spdlog::error("Failed to start MCP server on port {}: {}", port, e.what());
			running = false;
			return false;
		}
	}

	void stop() {
		if (!running) return;
		running = false;
		ioc.stop();
		if (server_thread.joinable()) {
			server_thread.join();
		}
		ioc.restart();
		spdlog::info("MCP Server stopped.");
	}

	bool isRunning() const {
		return running;
	}

	void setHandler(McpServer::RequestHandler h) {
		std::lock_guard<std::mutex> lock(handler_mutex);
		handler = h;
	}

	bool processPendingRequests() {
		McpServer::RequestHandler currentHandler;
		{
			std::lock_guard<std::mutex> lock(handler_mutex);
			currentHandler = handler;
		}

		if (!currentHandler) {
			return false; // No handler, ignore
		}

		std::vector<std::shared_ptr<PendingRequest>> requests_to_process;

		{
			std::lock_guard<std::mutex> lock(queue_mutex);
			while (!request_queue.empty()) {
				requests_to_process.push_back(request_queue.front());
				request_queue.pop();
			}
		}

		for (auto& req : requests_to_process) {
			std::string response;
			try {
				response = currentHandler(req->payload);
			} catch (const std::exception& e) {
				response = std::string("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32603,\"message\":\"Internal error\"},\"id\":null}");
				spdlog::error("MCP Handler exception: {}", e.what());
			}

			if (req->on_complete) {
				req->on_complete(response);
			}
		}

		return !requests_to_process.empty();
	}

private:
	void do_accept() {
		acceptor.async_accept(net::make_strand(ioc), [this](beast::error_code ec, tcp::socket socket) {
			if (!ec) {
				std::make_shared<Session>(std::move(socket), this)->start();
			}
			if (running) {
				do_accept();
			}
		});
	}

	class Session : public std::enable_shared_from_this<Session> {
	public:
		Session(tcp::socket socket, McpServerImpl* server) :
			stream(std::move(socket)),
			server(server) {}

		void start() {
			read_request();
		}

	private:
		void read_request() {
			auto self = shared_from_this();
			http::async_read(stream, buffer, req, [self](beast::error_code ec, std::size_t bytes_transferred) {
				boost::ignore_unused(bytes_transferred);
				if (!ec) {
					self->process_request();
				}
			});
		}

		void process_request() {
			if (req.method() == http::verb::post) {
				std::string payload = req.body();

				auto pending = std::make_shared<PendingRequest>();
				pending->payload = payload;

				auto self = shared_from_this();
				pending->on_complete = [self, server = this->server](const std::string& response) {
					net::post(server->ioc, [self, response]() {
						self->send_response(http::status::ok, response);
					});
				};

				{
					std::lock_guard<std::mutex> lock(server->queue_mutex);
					server->request_queue.push(pending);
				}

				// Wake up the main thread to process the pending request
				wxWakeUpIdle();
			} else {
				send_response(http::status::bad_request, "Only POST requests are supported for MCP JSON-RPC.");
			}
		}

		void send_response(http::status status, const std::string& body) {
			auto self = shared_from_this();
			res.version(req.version());
			res.result(status);
			res.set(http::field::server, "RME MCP Server");
			res.set(http::field::content_type, "application/json");
			res.body() = body;
			res.prepare_payload();

			http::async_write(stream, res, [self](beast::error_code ec, std::size_t bytes_transferred) {
				boost::ignore_unused(bytes_transferred);
				beast::error_code ignored_ec;
				self->stream.socket().shutdown(tcp::socket::shutdown_send, ignored_ec);
			});
		}

		beast::tcp_stream stream;
		beast::flat_buffer buffer;
		http::request<http::string_body> req;
		http::response<http::string_body> res;
		McpServerImpl* server;
	};

	net::io_context ioc;
	tcp::acceptor acceptor;
	std::thread server_thread;
	bool running;

	std::mutex queue_mutex;
	std::queue<std::shared_ptr<PendingRequest>> request_queue;

	std::mutex handler_mutex;
	McpServer::RequestHandler handler;

	friend class Session;
};

McpServer::McpServer() :
	impl(std::make_unique<McpServerImpl>()) {}

McpServer::~McpServer() {}

McpServer& McpServer::getInstance() {
	static McpServer instance;
	return instance;
}

bool McpServer::start(uint16_t port) {
	return impl->start(port);
}

void McpServer::stop() {
	impl->stop();
}

bool McpServer::isRunning() const {
	return impl->isRunning();
}

void McpServer::setHandler(RequestHandler handler) {
	impl->setHandler(handler);
}

bool McpServer::processPendingRequests() {
	return impl->processPendingRequests();
}
