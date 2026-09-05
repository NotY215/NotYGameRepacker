# Dependency management for NotY Game Repacker

# Print dependency summary
message(STATUS "========================================")
message(STATUS "Dependencies")
message(STATUS "========================================")

# Qt6
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.16)
    message(STATUS "Qt6: ${Qt6_VERSION}")
endif()

# Zstd - check using find_package variables
if(ZSTD_FOUND)
    message(STATUS "Zstd: Found (${ZSTD_VERSION})")
elseif(TARGET zstd::libzstd_shared)
    message(STATUS "Zstd: Found (zstd::libzstd_shared)")
elseif(TARGET zstd::libzstd_static)
    message(STATUS "Zstd: Found (zstd::libzstd_static)")
elseif(TARGET zstd::libzstd)
    message(STATUS "Zstd: Found (zstd::libzstd)")
else()
    message(WARNING "Zstd: Not found - check vcpkg installation")
endif()

# nlohmann_json
if(nlohmann_json_FOUND)
    message(STATUS "nlohmann_json: Found (${nlohmann_json_VERSION})")
elseif(TARGET nlohmann_json::nlohmann_json)
    message(STATUS "nlohmann_json: Found")
else()
    message(WARNING "nlohmann_json: Not found - check vcpkg installation")
endif()

# BLAKE3
if(blake3_FOUND)
    message(STATUS "BLAKE3: Found")
elseif(TARGET BLAKE3::blake3)
    message(STATUS "BLAKE3: Found")
else()
    message(WARNING "BLAKE3: Not found - check vcpkg installation")
endif()

message(STATUS "========================================")