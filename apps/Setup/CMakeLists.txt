add_executable(NotYSetup
    main.cpp
    SetupApplication.cpp
    SetupApplication.h
    SetupWindow.cpp
    SetupWindow.h
    ${RESOURCES}
)

target_link_libraries(NotYSetup
    PRIVATE
        NotYCore
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
)

# Set properties for Windows
if(WIN32)
    set_target_properties(NotYSetup PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Install target
install(TARGETS NotYSetup
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION bin
    LIBRARY DESTINATION bin
)

# Enable parallel compilation
if(MSVC)
    target_compile_options(NotYSetup PRIVATE /MP)
endif()

# Generate a pdb file for debugging
if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(NotYSetup PRIVATE /Zi)
    target_link_options(NotYSetup PRIVATE /DEBUG)
endif()

# Deploy Qt dependencies
include(QtDeployment)
deploy_qt(NotYSetup)

message(STATUS "NotYSetup configured")