cmake_minimum_required(VERSION 3.31)

project(rme)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Compiler-specific flags
if(MSVC)
	set(CMAKE_CXX_FLAGS                "/W3 /wd4996 /wd4800 /wd4100 /wd4706 /EHsc")
	set(CMAKE_CXX_FLAGS_DEBUG          "/Od /Zi /D__DEBUG__")
	set(CMAKE_CXX_FLAGS_MINSIZEREL     "/O1 /DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE        "/O2 /DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /Zi")
else()
	set(CMAKE_CXX_FLAGS                "-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-deprecated-declarations -Wno-overloaded-virtual -Wno-strict-aliasing -Wno-sign-compare -Wno-unused-function -Wunused-result")
	set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g -D__DEBUG__")
	set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -DNDEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
endif()

find_package(OpenGL REQUIRED)

if(APPLE)
    set(CMAKE_PREFIX_PATH /usr/local/opt/libarchive)
endif()
find_package(LibArchive REQUIRED)

if(WIN32)
    set(Boost_THREADAPI win32)
endif()
find_package(Boost 1.34.0 COMPONENTS thread system REQUIRED)

find_package(wxWidgets COMPONENTS html aui gl adv core net base REQUIRED)

find_package(glad CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# wxWidgets_USE_FILE is set by vcpkg/system wxWidgets, but empty with Conan
if(wxWidgets_USE_FILE)
	include(${wxWidgets_USE_FILE})
endif()

include(source/CMakeLists.txt)

# Option to force console window for debugging (even in Release)
option(RME_DEBUG_CONSOLE "Force console window for debugging output" ON)

if(WIN32 AND NOT RME_DEBUG_CONSOLE)
	add_executable(rme WIN32 ${rme_H} ${rme_SRC})
else()
	add_executable(rme ${rme_H} ${rme_SRC})
endif()

if(RME_DEBUG_CONSOLE)
	message(STATUS "Console window enabled for debugging")
	# For MSVC: Keep console subsystem but use WinMain entry point via linker options
	# This allows both console output AND wxWidgets to work together
	if(MSVC)
		target_link_options(rme PRIVATE /SUBSYSTEM:CONSOLE /ENTRY:WinMainCRTStartup)
	endif()
endif()

set_target_properties(rme PROPERTIES CXX_STANDARD 20)
set_target_properties(rme PROPERTIES CXX_STANDARD_REQUIRED ON)


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
	${CMAKE_CURRENT_SOURCE_DIR}/source
	${Boost_INCLUDE_DIRS}
	${LibArchive_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
)

# Link other libraries (works with both vcpkg targets and Conan targets)
target_link_libraries(rme PRIVATE 
    ${OPENGL_LIBRARIES}
    glad::glad
    fmt::fmt-header-only
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

if(TARGET Boost::thread AND TARGET Boost::system)
    target_link_libraries(rme PRIVATE Boost::thread Boost::system)
else()
    target_link_libraries(rme PRIVATE ${Boost_LIBRARIES})
endif()

if(TARGET ZLIB::ZLIB)
    target_link_libraries(rme PRIVATE ZLIB::ZLIB)
else()
    target_link_libraries(rme PRIVATE ${ZLIB_LIBRARIES})
endif()

if(TARGET LibArchive::LibArchive)
    target_link_libraries(rme PRIVATE LibArchive::LibArchive)
else()
    target_link_libraries(rme PRIVATE ${LibArchive_LIBRARIES})
endif()