#include "ui/app_window.hpp"
#include "ui/theme.hpp"
#include "ui/logo.hpp"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int) {
    noty::ui::enable_dpi_awareness();

    noty::ui::GdiPlusSession gdip;
    if (!gdip.ok()) {
        MessageBoxW(nullptr,
                    L"Failed to initialise GDI+.",
                    L"NotY Game Repacker",
                    MB_ICONERROR | MB_OK);
        return 1;
    }

    noty::ui::RootWindowSpec spec{
        L"NotY Game Repacker",
        L"Powered by NotY215",
        960, 640
    };

    return noty::ui::run_root_window(instance, spec);
}