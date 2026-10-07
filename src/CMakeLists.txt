# ---------------------------------------------------------------------------
# noty_core - C backend primitives
# ---------------------------------------------------------------------------
add_library(noty_core STATIC
    core/core.c
)

target_include_directories(noty_core PUBLIC
    "${CMAKE_SOURCE_DIR}/include"
)

target_compile_features(noty_core PUBLIC c_std_17)

noty_configure_target(noty_core)

# ---------------------------------------------------------------------------
# noty_ui - Win32 native UI foundation
# ---------------------------------------------------------------------------
add_library(noty_ui STATIC
    ui/theme.cpp
    ui/logo.cpp
    ui/app_window.cpp
)

target_include_directories(noty_ui PUBLIC
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/src"
)

target_compile_features(noty_ui PUBLIC cxx_std_20)

target_link_libraries(noty_ui PUBLIC
    noty_core
    Bcrypt
    Gdiplus
    Comctl32
    Shell32
    Shlwapi
    Dwmapi
    UxTheme
    User32
    Gdi32
    Ole32
)

if(NOTY_HAS_LOGO)
    target_compile_definitions(noty_ui PRIVATE NOTY_HAS_LOGO=1)
endif()

noty_configure_target(noty_ui)