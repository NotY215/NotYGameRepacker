cmake_minimum_required(VERSION 3.24)

project(NotYGameRepacker
    VERSION 0.1.0
    DESCRIPTION "NotY Game Repacker - Powered by NotY215"
    LANGUAGES C CXX RC
)

# ---------------------------------------------------------------------------
# Platform guard
# ---------------------------------------------------------------------------
if(NOT WIN32)
    message(FATAL_ERROR "NotY Game Repacker targets Windows only.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "NotY Game Repacker is x64 only. Configure with -A x64.")
endif()

# ---------------------------------------------------------------------------
# Language standards
# ---------------------------------------------------------------------------
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Default build type (single-config generators). Only Debug and Release
# are supported by this project.
# ---------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)
endif()

# ---------------------------------------------------------------------------
# Static MSVC runtime for zero-dependency distribution
# ---------------------------------------------------------------------------
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ---------------------------------------------------------------------------
# Common target configuration helper
# ---------------------------------------------------------------------------
function(noty_configure_target target)
    target_compile_definitions(${target} PRIVATE
        UNICODE
        _UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /Zc:__cplusplus
            /Zc:preprocessor
            /utf-8
            /MP
            $<$<CONFIG:Release>:/O2>
            $<$<CONFIG:Release>:/GL>
        )
        target_link_options(${target} PRIVATE
            $<$<CONFIG:Release>:/LTCG>
        )
    endif()
endfunction()

# ---------------------------------------------------------------------------
# Resource detection
# ---------------------------------------------------------------------------
set(NOTY_RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")
file(TO_CMAKE_PATH "${NOTY_RESOURCE_DIR}" NOTY_RESOURCE_DIR_CMAKE)

if(EXISTS "${NOTY_RESOURCE_DIR}/logo.png")
    set(NOTY_HAS_LOGO ON)
    message(STATUS "NotY: embedding resources/logo.png")
else()
    set(NOTY_HAS_LOGO OFF)
    message(WARNING "NotY: resources/logo.png not found - building without embedded logo.")
endif()

# ---------------------------------------------------------------------------
# Subdirectories
# ---------------------------------------------------------------------------
add_subdirectory(src)
add_subdirectory(apps)