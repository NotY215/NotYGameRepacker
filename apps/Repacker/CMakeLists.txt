set(NOTY_VERSION_COMMA     "0,1,0,0")
set(NOTY_VERSION_STR       "0.1.0.0")
set(NOTY_FILE_DESCRIPTION  "NotY Game Repacker - Powered by NotY215")
set(NOTY_INTERNAL_NAME     "NotYRepacker")
set(NOTY_ORIGINAL_FILENAME "NotYRepacker.exe")

configure_file(
    "${CMAKE_SOURCE_DIR}/resources/NotY.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Repacker.rc"
    @ONLY
)

add_executable(NotYRepacker WIN32
    main.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/Repacker.rc"
)

set_target_properties(NotYRepacker PROPERTIES
    OUTPUT_NAME "NotYRepacker"
)

target_link_libraries(NotYRepacker PRIVATE noty_ui)

if(NOTY_HAS_LOGO)
    target_compile_definitions(NotYRepacker PRIVATE NOTY_HAS_LOGO=1)
    set_source_files_properties(
        "${CMAKE_CURRENT_BINARY_DIR}/Repacker.rc"
        PROPERTIES OBJECT_DEPENDS "${CMAKE_SOURCE_DIR}/resources/logo.png"
    )
endif()

noty_configure_target(NotYRepacker)