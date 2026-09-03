# Dependency management for NotY Game Repacker

# For VS built-in vcpkg, we need to handle manifest mode
if(DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_TOOLCHAIN_FILE}")
    # Detect vcpkg root
    get_filename_component(VCPKG_SCRIPTS_DIR "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
    get_filename_component(VCPKG_ROOT "${VCPKG_SCRIPTS_DIR}" DIRECTORY)
    get_filename_component(VCPKG_ROOT "${VCPKG_ROOT}" DIRECTORY)
    
    if(EXISTS "${VCPKG_ROOT}")
        set(VCPKG_ROOT "${VCPKG_ROOT}" CACHE PATH "vcpkg root directory" FORCE)
        message(STATUS "vcpkg root: ${VCPKG_ROOT}")
        
        # Check if vcpkg.json exists in project root
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json")
            message(STATUS "vcpkg.json found in project root")
            # Ensure vcpkg will use the manifest
            set(VCPKG_MANIFEST_MODE ON CACHE BOOL "Enable vcpkg manifest mode" FORCE)
        endif()
    else()
        message(WARNING "Could not detect vcpkg root from toolchain file")
    endif()
else()
    message(WARNING "CMAKE_TOOLCHAIN_FILE not defined. vcpkg may not be configured.")
endif()

# Print dependency summary
message(STATUS "========================================")
message(STATUS "Dependencies")
message(STATUS "========================================")

# Qt6
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.16)
    message(STATUS "Qt6: ${Qt6_VERSION}")
endif()

# Zstd
if(TARGET zstd::libzstd)
    message(STATUS "Zstd: Found")
elseif(TARGET zstd::libzstd_static)
    message(STATUS "Zstd: Found (static)")
else()
    message(STATUS "Zstd: Not found as target")
endif()

# nlohmann_json
if(TARGET nlohmann_json::nlohmann_json)
    message(STATUS "nlohmann_json: Found")
else()
    message(STATUS "nlohmann_json: Not found as target")
endif()

# BLAKE3
if(TARGET BLAKE3::blake3)
    message(STATUS "BLAKE3: Found")
else()
    message(STATUS "BLAKE3: Not found as target")
endif()

message(STATUS "========================================")

# Function to check if a vcpkg package is installed
function(check_vcpkg_package PACKAGE_NAME)
    if(DEFINED VCPKG_ROOT)
        set(PACKAGE_INSTALLED_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}")
        if(EXISTS "${PACKAGE_INSTALLED_DIR}")
            # Check for the package
            if(EXISTS "${PACKAGE_INSTALLED_DIR}/include/${PACKAGE_NAME}")
                message(STATUS "Package ${PACKAGE_NAME}: Installed")
            endif()
        endif()
    endif()
endfunction()