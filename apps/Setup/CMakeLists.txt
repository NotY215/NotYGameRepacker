add_executable(NotYSetup
    main.cpp
    SetupApplication.cpp
    SetupApplication.h
    SetupWindow.cpp
    SetupWindow.h
)

target_link_libraries(NotYSetup
    PRIVATE
        NotYCore
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
)

# Include the resource file (defined in root)
target_sources(NotYSetup PRIVATE ${RESOURCES})

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

# Package the application
package_application(NotYSetup)

# Create a custom target for the full setup package
add_custom_target(package_setup
    DEPENDS NotYSetup package_NotYSetup
    COMMENT "Packaging NotYSetup..."
)

message(STATUS "NotYSetup configured")