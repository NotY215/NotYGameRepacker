function(deploy_qt TARGET_NAME)
    if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt6)
        
        if(NOT WINDEPLOYQT_EXECUTABLE)
            find_program(WINDEPLOYQT_EXECUTABLE windeployqt)
        endif()
        
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE}
                    --no-compiler-runtime
                    --no-system-d3d-compiler
                    --no-opengl-sw
                    --verbose 0
                    $<TARGET_FILE:${TARGET_NAME}>
                COMMENT "Deploying Qt dependencies for ${TARGET_NAME}"
                VERBATIM
            )
            message(STATUS "Qt deployment configured for ${TARGET_NAME}")
        else()
            message(WARNING "windeployqt not found; Qt DLLs will not be automatically deployed.")
        endif()
    endif()
endfunction()