#include "logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <vector>
#ifdef _WIN32
	#include <windows.h>
#endif
#include <GL/gl.h> // For GLenum and GL_NO_ERROR

std::shared_ptr<spdlog::logger> Logger::s_CoreLogger;
std::shared_ptr<spdlog::logger> Logger::s_RenderLogger;
std::shared_ptr<spdlog::logger> Logger::s_MapLogger;

void Logger::init() {
	// Create multiple sinks: console and file
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	console_sink->set_level(spdlog::level::trace);
	console_sink->set_pattern("%^[%T] %n: %v%$");

	auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/rme.log", 1024 * 1024 * 5, 3);
	file_sink->set_level(spdlog::level::trace);
	file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");

	std::vector<spdlog::sink_ptr> sinks { console_sink, file_sink };

	// Core Logger
	s_CoreLogger = std::make_shared<spdlog::logger>("CORE", sinks.begin(), sinks.end());
	spdlog::register_logger(s_CoreLogger);
	s_CoreLogger->set_level(spdlog::level::trace);
	s_CoreLogger->flush_on(spdlog::level::trace);

	// Render Logger
	s_RenderLogger = std::make_shared<spdlog::logger>("RENDER", sinks.begin(), sinks.end());
	spdlog::register_logger(s_RenderLogger);
	s_RenderLogger->set_level(spdlog::level::trace);
	s_RenderLogger->flush_on(spdlog::level::trace);

	// Map Logger
	s_MapLogger = std::make_shared<spdlog::logger>("MAP", sinks.begin(), sinks.end());
	spdlog::register_logger(s_MapLogger);
	s_MapLogger->set_level(spdlog::level::trace);
	s_MapLogger->flush_on(spdlog::level::trace);
}

void Logger::shutdown() {
	spdlog::shutdown();
}

std::shared_ptr<spdlog::logger>& Logger::getCoreLogger() {
	return s_CoreLogger;
}

std::shared_ptr<spdlog::logger>& Logger::getRenderLogger() {
	return s_RenderLogger;
}

std::shared_ptr<spdlog::logger>& Logger::getMapLogger() {
	return s_MapLogger;
}
