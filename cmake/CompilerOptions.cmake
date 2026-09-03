if(MSVC)
    # Enable warnings and standards compliance
    add_compile_options(/W4 /WX- /permissive- /Zc:__cplusplus)
    add_compile_options(/std:c++20)
    
    # Enable multi-processor compilation for faster builds
    add_compile_options(/MP)
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        # Release optimizations
        add_compile_options(/O2 /Oi /GL)
        add_link_options(/LTCG /OPT:REF /OPT:ICF)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        # Debug settings
        add_compile_options(/Od /Zi)
        add_link_options(/DEBUG)
    else()
        # RelWithDebInfo - Release with debug info
        add_compile_options(/O2 /Zi /GL)
        add_link_options(/LTCG /DEBUG)
    endif()
    
    # Enable fast linking for development
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_link_options(/INCREMENTAL)
    else()
        add_link_options(/INCREMENTAL:NO)
    endif()
    
else()
    message(FATAL_ERROR "Only MSVC is supported for this project.")
endif()

message(STATUS "Compiler options configured for MSVC (Build: ${CMAKE_BUILD_TYPE})")