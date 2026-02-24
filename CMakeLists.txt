cmake_minimum_required(VERSION 3.28)

project(rme)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Explicitly set configuration types for multi-config generators
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel" CACHE STRING "" FORCE)

# Compiler-specific flags
if(MSVC)
	set(CMAKE_CXX_FLAGS                "/W3 /wd4996 /wd4800 /wd4100 /wd4706 /EHsc")
	set(CMAKE_CXX_FLAGS_DEBUG          "/Od /Zi /D__DEBUG__")
	set(CMAKE_CXX_FLAGS_MINSIZEREL     "/O1 /DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE        "/O2 /DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /Zi /DNDEBUG")
else()
	set(CMAKE_CXX_FLAGS                "-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-deprecated-declarations -Wno-overloaded-virtual -Wno-strict-aliasing -Wno-sign-compare -Wno-unused-function -Wunused-result")
	set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g -D__DEBUG__")
	set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
endif()

find_package(OpenGL REQUIRED)

if(WIN32)
    set(Boost_THREADAPI win32)
endif()
find_package(Boost 1.34.0 COMPONENTS thread system REQUIRED)

find_package(wxWidgets 3.2 COMPONENTS html aui gl adv core net base propgrid REQUIRED)

# On Linux/GTK, find GTK3 for direct GTK API access (e.g. g_log_set_handler)
if(UNIX AND NOT APPLE)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
endif()


find_package(ZLIB REQUIRED)


# wxWidgets_USE_FILE is set by vcpkg/system wxWidgets, but empty with Conan
if(wxWidgets_USE_FILE)
	include(${wxWidgets_USE_FILE})
endif()

find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)

# Lua dependencies
find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)

# nanovg: try vcpkg/Conan first, fallback to ext/nanovg
find_package(nanovg CONFIG QUIET)
if(NOT nanovg_FOUND)
	message(STATUS "nanovg not found via package manager, using ext/nanovg...")
	
	# Build nanovg from ext/nanovg source
	add_library(nanovg_lib STATIC
		${CMAKE_CURRENT_SOURCE_DIR}/ext/nanovg/src/nanovg.c
	)
	target_include_directories(nanovg_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ext/nanovg/src)
	add_library(nanovg::nanovg ALIAS nanovg_lib)
endif()

# Add Lua includes to ensure sol2 finds 'lua.hpp' if needed
# (Conan targets usually handle this via transitive dependencies)

# Define SOL_USING_CXX_LUA_HPP for sol2
add_compile_definitions(SOL_USING_CXX_LUA_HPP=1)
add_compile_definitions(SOL_ALL_SAFETIES_ON=1)

include(source/CMakeLists.txt)
# Removed WIN32 to enable Console subsystem for persistent debugging logs
add_executable(rme ${rme_H} ${rme_SRC})

set_target_properties(rme PROPERTIES CXX_STANDARD 20)
set_target_properties(rme PROPERTIES CXX_STANDARD_REQUIRED ON)
target_compile_definitions(rme PRIVATE WXWIN_COMPATIBILITY_3_3 NANOVG_GL3)
 
# Copy assets to build directory
add_custom_command(TARGET rme POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${CMAKE_CURRENT_SOURCE_DIR}/source/assets"
	"$<TARGET_FILE_DIR:rme>/assets"
	COMMENT "Copying assets to output directory..."
)


# Link against wxWidgets (works with both vcpkg and Conan)
if(TARGET wxWidgets::wxWidgets)
	# Conan provides a single wxWidgets::wxWidgets target
	target_link_libraries(rme PRIVATE wxWidgets::wxWidgets)
else()
	# vcpkg/system provides wxWidgets_LIBRARIES variable
	target_link_libraries(rme PRIVATE ${wxWidgets_LIBRARIES})
endif()

# Include directories (for legacy find modules)
target_include_directories(rme PRIVATE
	${Boost_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
	${LUA_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/source
)

# GTK3 includes for direct GTK API access on Linux
if(UNIX AND NOT APPLE AND GTK3_FOUND)
	target_include_directories(rme PRIVATE ${GTK3_INCLUDE_DIRS})
	target_link_libraries(rme PRIVATE ${GTK3_LIBRARIES})
endif()

# Link other libraries (works with both vcpkg targets and Conan targets)
if(TARGET Boost::boost)
	target_link_libraries(rme PRIVATE Boost::boost Boost::thread Boost::system)
else()
	target_link_libraries(rme PRIVATE ${Boost_LIBRARIES})
endif()


if(TARGET ZLIB::ZLIB)
	target_link_libraries(rme PRIVATE ZLIB::ZLIB)
else()
	target_link_libraries(rme PRIVATE ${ZLIB_LIBRARIES})
endif()

target_link_libraries(rme PRIVATE ${OPENGL_LIBRARIES})
target_link_libraries(rme PRIVATE glad::glad)
target_link_libraries(rme PRIVATE glm::glm)
target_link_libraries(rme PRIVATE nanovg::nanovg)
target_link_libraries(rme PRIVATE spdlog::spdlog)
target_link_libraries(rme PRIVATE tomlplusplus::tomlplusplus)

# Link Lua dependencies
target_link_libraries(rme PRIVATE ${LUA_LIBRARIES})
target_link_libraries(rme PRIVATE sol2::sol2)
target_link_libraries(rme PRIVATE cpr::cpr)

option(RME_BUILD_TESTS "Build test executables" OFF)

if(RME_BUILD_TESTS)
	# Filter out application.cpp from tests to avoid multiple main() definitions
	set(rme_SRC_TEST ${rme_SRC})
	list(FILTER rme_SRC_TEST EXCLUDE REGEX "app/application\\.cpp$")

	# Test executable for autoborder preview - uses same sources as main target
	add_executable(test_autoborder_preview ${rme_H} ${rme_SRC_TEST} ${CMAKE_CURRENT_SOURCE_DIR}/source/test_autoborder_preview.cpp)
	target_compile_definitions(test_autoborder_preview PRIVATE WXWIN_COMPATIBILITY_3_3 NANOVG_GL3)

	# Include directories
	target_include_directories(test_autoborder_preview PRIVATE
		${Boost_INCLUDE_DIRS}
		${OPENGL_INCLUDE_DIR}
		${ZLIB_INCLUDE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/source
	)

	# Link libraries (same as main rme target)
	if(TARGET wxWidgets::wxWidgets)
		target_link_libraries(test_autoborder_preview PRIVATE wxWidgets::wxWidgets)
	else()
		target_link_libraries(test_autoborder_preview PRIVATE ${wxWidgets_LIBRARIES})
	endif()

	if(TARGET Boost::boost)
		target_link_libraries(test_autoborder_preview PRIVATE Boost::boost Boost::thread Boost::system)
	else()
		target_link_libraries(test_autoborder_preview PRIVATE ${Boost_LIBRARIES})
	endif()


	if(TARGET ZLIB::ZLIB)
		target_link_libraries(test_autoborder_preview PRIVATE ZLIB::ZLIB)
	else()
		target_link_libraries(test_autoborder_preview PRIVATE ${ZLIB_LIBRARIES})
	endif()

	target_link_libraries(test_autoborder_preview PRIVATE ${OPENGL_LIBRARIES})
	target_link_libraries(test_autoborder_preview PRIVATE glad::glad)
	target_link_libraries(test_autoborder_preview PRIVATE glm::glm)
	target_link_libraries(test_autoborder_preview PRIVATE nanovg::nanovg)
	target_link_libraries(test_autoborder_preview PRIVATE spdlog::spdlog)
	target_link_libraries(test_autoborder_preview PRIVATE tomlplusplus::tomlplusplus)

    # Lua for tests
    target_link_libraries(test_autoborder_preview PRIVATE ${LUA_LIBRARIES})
    target_link_libraries(test_autoborder_preview PRIVATE sol2::sol2)
    target_link_libraries(test_autoborder_preview PRIVATE cpr::cpr)

	# Lua API Test
	add_executable(test_lua_api ${rme_H} ${rme_SRC_TEST} ${CMAKE_CURRENT_SOURCE_DIR}/source/test_lua_api.cpp)
	target_compile_definitions(test_lua_api PRIVATE WXWIN_COMPATIBILITY_3_3 NANOVG_GL3)

	target_include_directories(test_lua_api PRIVATE
		${Boost_INCLUDE_DIRS}
		${OPENGL_INCLUDE_DIR}
		${ZLIB_INCLUDE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/source
	)

	if(TARGET wxWidgets::wxWidgets)
		target_link_libraries(test_lua_api PRIVATE wxWidgets::wxWidgets)
	else()
		target_link_libraries(test_lua_api PRIVATE ${wxWidgets_LIBRARIES})
	endif()

	if(TARGET Boost::boost)
		target_link_libraries(test_lua_api PRIVATE Boost::boost Boost::thread Boost::system)
	else()
		target_link_libraries(test_lua_api PRIVATE ${Boost_LIBRARIES})
	endif()

	if(TARGET ZLIB::ZLIB)
		target_link_libraries(test_lua_api PRIVATE ZLIB::ZLIB)
	else()
		target_link_libraries(test_lua_api PRIVATE ${ZLIB_LIBRARIES})
	endif()

	target_link_libraries(test_lua_api PRIVATE ${OPENGL_LIBRARIES})
	target_link_libraries(test_lua_api PRIVATE glad::glad)
	target_link_libraries(test_lua_api PRIVATE glm::glm)
	target_link_libraries(test_lua_api PRIVATE nanovg::nanovg)
	target_link_libraries(test_lua_api PRIVATE spdlog::spdlog)
	target_link_libraries(test_lua_api PRIVATE tomlplusplus::tomlplusplus)
	target_link_libraries(test_lua_api PRIVATE ${LUA_LIBRARIES})
	target_link_libraries(test_lua_api PRIVATE sol2::sol2)
	target_link_libraries(test_lua_api PRIVATE cpr::cpr)

	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
	set_target_properties(test_lua_api PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
		RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
		RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
	)

endif()
