#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h> // For logging custom types if needed

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
#define LOG_TRACE(...) ::Logger::getCoreLogger()->trace(__VA_ARGS__)
#define LOG_INFO(...) ::Logger::getCoreLogger()->info(__VA_ARGS__)
#define LOG_WARN(...) ::Logger::getCoreLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...) ::Logger::getCoreLogger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::Logger::getCoreLogger()->critical(__VA_ARGS__)

// Rendering specific macros
#define LOG_RENDER_TRACE(...) ::Logger::getRenderLogger()->trace(__VA_ARGS__)
#define LOG_RENDER_INFO(...) ::Logger::getRenderLogger()->info(__VA_ARGS__)
#define LOG_RENDER_WARN(...) ::Logger::getRenderLogger()->warn(__VA_ARGS__)
#define LOG_RENDER_ERROR(...) ::Logger::getRenderLogger()->error(__VA_ARGS__)

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
