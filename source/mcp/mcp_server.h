#ifndef RME_MCP_SERVER_H
#define RME_MCP_SERVER_H

#include <string>
#include <memory>
#include <functional>

class McpServerImpl;

class McpServer {
public:
	static McpServer& getInstance();

	// Starts the background HTTP/TCP server on the specified port
	bool start(uint16_t port);

	// Stops the background server
	void stop();

	// Checks if the server is currently running
	bool isRunning() const;

	// Typedef for the callback: accepts a JSON string request, returns a JSON string response
	using RequestHandler = std::function<std::string(const std::string&)>;

	// Sets the callback that will be executed on the main thread
	void setHandler(RequestHandler handler);

	// To be called from the main thread (e.g., in a wxTimer or OnIdle)
	// This will process pending requests and call the handler
	// Returns true if at least one request was processed
	bool processPendingRequests();

private:
	McpServer();
	~McpServer();

	McpServer(const McpServer&) = delete;
	McpServer& operator=(const McpServer&) = delete;

	std::unique_ptr<McpServerImpl> impl;
};

#define g_mcpServer McpServer::getInstance()

#endif // RME_MCP_SERVER_H
