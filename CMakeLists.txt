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

# Auto-detect vcpkg toolchain if not set
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    # Try Visual Studio vcpkg locations
    set(VCPKG_PATHS
        "C:/Program Files/Microsoft Visual Studio/18/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake"
        "C:/Program Files/Microsoft Visual Studio/2026/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake"
        "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake"
    )
    
    foreach(VCPKG_PATH ${VCPKG_PATHS})
        if(EXISTS "${VCPKG_PATH}")
            set(CMAKE_TOOLCHAIN_FILE "${VCPKG_PATH}" CACHE FILEPATH "vcpkg toolchain file")
            message(STATUS "Auto-detected vcpkg: ${VCPKG_PATH}")
            break()
        endif()
    endforeach()
    
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        message(WARNING "CMAKE_TOOLCHAIN_FILE not set. Please set it to vcpkg.cmake")
        message(WARNING "Expected VS vcpkg path: C:/Program Files/Microsoft Visual Studio/18/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake")
    endif()
endif()

# For VS built-in vcpkg, we need to handle manifest mode correctly
if(DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_TOOLCHAIN_FILE}")
    # Ensure vcpkg manifest mode is enabled
    set(VCPKG_MANIFEST_MODE ON CACHE BOOL "Enable vcpkg manifest mode" FORCE)
    set(VCPKG_MANIFEST_INSTALL ON CACHE BOOL "Install vcpkg dependencies" FORCE)
endif()

# Include our custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CompilerOptions)
include(Dependencies)
include(QtDeployment)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# Find other libraries
find_package(zstd REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(blake3 CONFIG REQUIRED)

# Resource file (includes fonts and icons)
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
message(STATUS "VCPKG_ROOT: ${VCPKG_ROOT}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "========================================")

# Performance optimization flags for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        # Enable whole program optimization
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
    endif()
endif()