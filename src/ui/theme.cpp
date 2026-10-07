#include "theme.hpp"

#include <windows.h>
#include <dwmapi.h>

namespace noty::ui {

void enable_dpi_awareness() {
    using SetProcessDpiAwarenessContextFn = BOOL (WINAPI*)(DPI_AWARENESS_CONTEXT);

    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        auto set_ctx = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
            reinterpret_cast<void*>(
                GetProcAddress(user32, "SetProcessDpiAwarenessContext")));

        if (set_ctx &&
            set_ctx(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
            return;
        }
    }

    /* Fallback for older builds. */
    SetProcessDPIAware();
}

void apply_dark_titlebar(HWND hwnd) {
    if (!hwnd) {
        return;
    }

    BOOL dark = TRUE;

    /* DWMWA_USE_IMMERSIVE_DARK_MODE is 20 on Windows 10 2004+ and 19 on
     * earlier Windows 10 builds. Try the new value first. */
    constexpr DWORD kAttrNew = 20;
    constexpr DWORD kAttrOld = 19;

    if (FAILED(DwmSetWindowAttribute(hwnd, kAttrNew, &dark, sizeof(dark)))) {
        DwmSetWindowAttribute(hwnd, kAttrOld, &dark, sizeof(dark));
    }

    /* Windows 11 rounded corners: DWMWA_WINDOW_CORNER_PREFERENCE = 33.
     * DWMWCP_ROUND = 2. Fails silently on Windows 10. */
    constexpr DWORD kAttrCorner = 33;
    constexpr DWORD kCornerRound = 2;
    DWORD corner = kCornerRound;
    DwmSetWindowAttribute(hwnd, kAttrCorner, &corner, sizeof(corner));
}

} // namespace noty::ui