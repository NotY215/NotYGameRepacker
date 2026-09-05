cmake_minimum_required(VERSION 3.20)
project(NotYGameRepacker VERSION 1.0.0 LANGUAGES CXX)

# Enable C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Include our custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CompilerOptions)
include(Dependencies)
include(QtDeployment)

# Find Qt6 - includes Core, Widgets, Gui
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# Find Zstd
find_package(zstd CONFIG REQUIRED)

# Find nlohmann_json
find_package(nlohmann_json CONFIG REQUIRED)

# Find BLAKE3
find_package(blake3 CONFIG REQUIRED)

# Resource file (includes fonts and icons)
# qt6_add_resources will use the rcc tool from Qt6
qt6_add_resources(RESOURCES resources/resources.qrc)

# Generate version header
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_BINARY_DIR}/generated/version.h
    @ONLY
)

# Include src directory for libraries
add_subdirectory(src)

# Include app directories
add_subdirectory(apps/Repacker)
add_subdirectory(apps/Setup)

# Install targets
include(GNUInstallDirs)

# Print build configuration
message(STATUS "========================================")
message(STATUS "NotY Game Repacker Build Configuration")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "========================================")

# Performance optimization flags for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
    endif()
endif()