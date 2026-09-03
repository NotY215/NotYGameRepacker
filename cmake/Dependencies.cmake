# Dependency management for NotY Game Repacker

# Print dependency summary
message(STATUS "========================================")
message(STATUS "Dependencies")
message(STATUS "========================================")

# Qt6
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.16)
    message(STATUS "Qt6: ${Qt6_VERSION}")
endif()

# Zstd
if(DEFINED ZSTD_LIBRARIES)
    message(STATUS "Zstd: Found")
elseif(TARGET zstd::libzstd)
    message(STATUS "Zstd: Found (target)")
elseif(TARGET zstd::libzstd_static)
    message(STATUS "Zstd: Found (static target)")
else()
    message(STATUS "Zstd: Not found")
endif()

# nlohmann_json
if(TARGET nlohmann_json::nlohmann_json)
    message(STATUS "nlohmann_json: Found")
else()
    message(STATUS "nlohmann_json: Not found as target")
endif()

# BLAKE3
if(DEFINED BLAKE3_LIBRARIES)
    message(STATUS "BLAKE3: Found")
elseif(TARGET BLAKE3::blake3)
    message(STATUS "BLAKE3: Found (target)")
else()
    message(STATUS "BLAKE3: Not found")
endif()

message(STATUS "========================================")