add_executable(NotYRepacker
    main.cpp
    RepackerApplication.cpp
    RepackerApplication.h
    MainWindow.cpp
    MainWindow.h
)

target_link_libraries(NotYRepacker
    PRIVATE
        NotYCore
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
)

# Include the resource file (defined in root)
target_sources(NotYRepacker PRIVATE ${RESOURCES})

# Set properties for Windows
if(WIN32)
    set_target_properties(NotYRepacker PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Install target
install(TARGETS NotYRepacker
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION bin
    LIBRARY DESTINATION bin
)

# Enable parallel compilation
if(MSVC)
    target_compile_options(NotYRepacker PRIVATE /MP)
endif()

# Generate a pdb file for debugging
if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(NotYRepacker PRIVATE /Zi)
    target_link_options(NotYRepacker PRIVATE /DEBUG)
endif()

# Deploy Qt dependencies
include(QtDeployment)
deploy_qt(NotYRepacker)

# Package the application
package_application(NotYRepacker)

# Create a custom target for the full repacker package
add_custom_target(package_repacker
    DEPENDS NotYRepacker package_NotYRepacker
    COMMENT "Packaging NotYRepacker..."
)

message(STATUS "NotYRepacker configured")