#pragma once

#include <windows.h>

namespace noty::ui {

struct RootWindowSpec {
    const wchar_t* title;       /* e.g. L"NotY Game Repacker" */
    const wchar_t* subtitle;    /* e.g. L"Powered by NotY215" */
    int            width_px;    /* logical client width at 96 DPI */
    int            height_px;   /* logical client height at 96 DPI */
};

/* Registers the shared window class (idempotent), creates, shows, and
 * pumps messages for the root window. Returns the process exit code. */
int run_root_window(HINSTANCE instance, const RootWindowSpec& spec);

} // namespace noty::ui