# src/CMakeLists.txt - Core library for NotY Game Repacker

# Create the core library target
add_library(NotYCore STATIC
    # Common
    common/Version.cpp
    common/Constants.cpp
    common/Logger.cpp
    common/Error.cpp
    
    # Filesystem - Phase 1
    filesystem/FileInfo.cpp
    filesystem/FileEnumerator.cpp
    filesystem/DirectoryScanner.cpp
    
    # Package - Phase 2
    package/Manifest.cpp
    package/ManifestWriter.cpp
    package/ManifestReader.cpp
    
    # Hashing - Phase 2
    hashing/Hasher.cpp
    
    # Compression - Phase 3
    compression/ZstdCompressor.cpp
    compression/ZstdDecompressor.cpp
    
    # Repacker - Phase 5
    repacker/RepackJob.cpp
    repacker/RepackEngine.cpp
    repacker/PackageBuilder.cpp
    
    # Installer - Phase 6
    installer/InstallJob.cpp
    installer/InstallEngine.cpp
    installer/ExtractionEngine.cpp
    
    # Core - Phase 10
    core/ResourceManager.cpp
    core/ThreadPool.cpp
    core/PerformanceMonitor.cpp
)

# Set include directories for the library
target_include_directories(NotYCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/generated
)

# Link dependencies
target_link_libraries(NotYCore
    PUBLIC
        Qt6::Core
        Qt6::Widgets
    PRIVATE
        nlohmann_json::nlohmann_json
        BLAKE3::blake3
        ${ZSTD_LIBRARIES}
        advapi32
        psapi
)

# Set C++ standard
target_compile_features(NotYCore PUBLIC cxx_std_20)

# Include directories for vcpkg dependencies
target_include_directories(NotYCore
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${ZSTD_INCLUDE_DIRS}
        ${BLAKE3_INCLUDE_DIRS}
)

# Add compiler options for the library
if(MSVC)
    target_compile_options(NotYCore PRIVATE /W4 /permissive- /MP)
endif()

# Generate a pdb file for debugging
if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(NotYCore PRIVATE /Zi)
    target_link_options(NotYCore PRIVATE /DEBUG)
endif()

message(STATUS "NotYCore configured with ${CMAKE_CXX_STANDARD} standard")