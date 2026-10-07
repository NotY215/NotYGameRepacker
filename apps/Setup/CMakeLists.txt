set(NOTY_VERSION_COMMA     "0,1,0,0")
set(NOTY_VERSION_STR       "0.1.0.0")
set(NOTY_FILE_DESCRIPTION  "NotY Setup - Generated Installer")
set(NOTY_INTERNAL_NAME     "NotYSetup")
set(NOTY_ORIGINAL_FILENAME "NotYSetup.exe")

configure_file(
    "${CMAKE_SOURCE_DIR}/resources/NotY.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Setup.rc"
    @ONLY
)

add_executable(NotYSetup WIN32
    main.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/Setup.rc"
)

set_target_properties(NotYSetup PROPERTIES
    OUTPUT_NAME "NotYSetup"
)

target_link_libraries(NotYSetup PRIVATE noty_ui)

if(NOTY_HAS_LOGO)
    target_compile_definitions(NotYSetup PRIVATE NOTY_HAS_LOGO=1)
    set_source_files_properties(
        "${CMAKE_CURRENT_BINARY_DIR}/Setup.rc"
        PROPERTIES OBJECT_DEPENDS "${CMAKE_SOURCE_DIR}/resources/logo.png"
    )
endif()

noty_configure_target(NotYSetup)