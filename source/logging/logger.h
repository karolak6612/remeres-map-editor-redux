#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/fmt/chrono.h>

#include <memory>
#include <string>

class Logger {
public:
	static void init();
	static void shutdown();

	static std::shared_ptr<spdlog::logger>& getCoreLogger();
	static std::shared_ptr<spdlog::logger>& getRenderLogger();
	static std::shared_ptr<spdlog::logger>& getMapLogger();

private:
	static std::shared_ptr<spdlog::logger> s_CoreLogger;
	static std::shared_ptr<spdlog::logger> s_RenderLogger;
	static std::shared_ptr<spdlog::logger> s_MapLogger;
};

// Core logging macros
#define LOG_TRACE(...)             \
	if (::Logger::getCoreLogger()) \
	::Logger::getCoreLogger()->trace(__VA_ARGS__)
#define LOG_INFO(...)              \
	if (::Logger::getCoreLogger()) \
	::Logger::getCoreLogger()->info(__VA_ARGS__)
#define LOG_WARN(...)              \
	if (::Logger::getCoreLogger()) \
	::Logger::getCoreLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)             \
	if (::Logger::getCoreLogger()) \
	::Logger::getCoreLogger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...)          \
	if (::Logger::getCoreLogger()) \
	::Logger::getCoreLogger()->critical(__VA_ARGS__)

// Rendering specific macros
#define LOG_RENDER_TRACE(...)        \
	if (::Logger::getRenderLogger()) \
	::Logger::getRenderLogger()->trace(__VA_ARGS__)
#define LOG_RENDER_DEBUG(...)        \
	if (::Logger::getRenderLogger()) \
	::Logger::getRenderLogger()->debug(__VA_ARGS__)
#define LOG_RENDER_INFO(...)         \
	if (::Logger::getRenderLogger()) \
	::Logger::getRenderLogger()->info(__VA_ARGS__)
#define LOG_RENDER_WARN(...)         \
	if (::Logger::getRenderLogger()) \
	::Logger::getRenderLogger()->warn(__VA_ARGS__)
#define LOG_RENDER_ERROR(...)        \
	if (::Logger::getRenderLogger()) \
	::Logger::getRenderLogger()->error(__VA_ARGS__)

// OpenGL Error Check Macro
#ifdef NDEBUG
	#define GL_CHECK(call) call
#else
	#define GL_CHECK(call)                                                                                \
		do {                                                                                              \
			call;                                                                                         \
			GLenum err = glGetError();                                                                    \
			if (err != GL_NO_ERROR) {                                                                     \
				LOG_RENDER_ERROR("OpenGL Error: {} at {}:{} - Call: {}", err, __FILE__, __LINE__, #call); \
			}                                                                                             \
		} while (0)
#endif
