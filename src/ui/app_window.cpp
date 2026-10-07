#include "app_window.hpp"
#include "theme.hpp"
#include "logo.hpp"

#include <windows.h>
#include <memory>

namespace noty::ui {

namespace {

UINT system_dpi() {
    using GetDpiForSystemFn = UINT (WINAPI*)();

    static GetDpiForSystemFn cached = []() -> GetDpiForSystemFn {
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        if (!user32) {
            return nullptr;
        }
        return reinterpret_cast<GetDpiForSystemFn>(
            reinterpret_cast<void*>(GetProcAddress(user32, "GetDpiForSystem")));
    }();

    if (cached) {
        return cached();
    }

    HDC dc = GetDC(nullptr);
    UINT dpi = 96;
    if (dc) {
        dpi = static_cast<UINT>(GetDeviceCaps(dc, LOGPIXELSY));
        ReleaseDC(nullptr, dc);
    }
    return dpi ? dpi : 96;
}

BOOL adjust_window_rect_for_dpi(LPRECT rc, DWORD style, BOOL menu,
                                DWORD exstyle, UINT dpi) {
    using AdjustFn = BOOL (WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);

    static AdjustFn cached = []() -> AdjustFn {
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        if (!user32) {
            return nullptr;
        }
        return reinterpret_cast<AdjustFn>(
            reinterpret_cast<void*>(
                GetProcAddress(user32, "AdjustWindowRectExForDpi")));
    }();

    if (cached) {
        return cached(rc, style, menu, exstyle, dpi);
    }
    return AdjustWindowRectEx(rc, style, menu, exstyle);
}

struct WindowState {
    RootWindowSpec                  spec{};
    HMODULE                         instance = nullptr;
    std::unique_ptr<BitmapHandle>   logo;
    HFONT                           title_font    = nullptr;
    HFONT                           subtitle_font = nullptr;
    HFONT                           body_font     = nullptr;
};

void paint_content(HWND hwnd, WindowState& st) {
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(hwnd, &ps);

    RECT rc{};
    GetClientRect(hwnd, &rc);

    /* Background */
    HBRUSH bg = CreateSolidBrush(palette::Background.to_colorref());
    FillRect(dc, &rc, bg);
    DeleteObject(bg);

    SetBkMode(dc, TRANSPARENT);

    const UINT dpi = system_dpi();
    const int  pad = MulDiv(32, dpi, 96);
    int        y   = pad;

    /* Logo (optional) */
    if (st.logo && st.logo->valid()) {
        Gdiplus::Graphics gfx(dc);
        gfx.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        gfx.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);

        const UINT lw = st.logo->bitmap->GetWidth();
        const UINT lh = st.logo->bitmap->GetHeight();

        const int target_h = MulDiv(96, dpi, 96);
        const int target_w = lh
            ? static_cast<int>((static_cast<double>(lw) * target_h) / lh)
            : target_h;

        gfx.DrawImage(st.logo->bitmap.get(), pad, y, target_w, target_h);
        y += target_h + MulDiv(24, dpi, 96);
    }

    /* Title */
    if (st.title_font) {
        HFONT old = static_cast<HFONT>(SelectObject(dc, st.title_font));
        SetTextColor(dc, palette::TextPrimary.to_colorref());

        RECT title_rc{ pad, y, rc.right - pad, y + MulDiv(48, dpi, 96) };
        DrawTextW(dc, st.spec.title, -1, &title_rc,
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

        SelectObject(dc, old);
        y += MulDiv(52, dpi, 96);
    }

    /* Subtitle */
    if (st.subtitle_font) {
        HFONT old = static_cast<HFONT>(SelectObject(dc, st.subtitle_font));
        SetTextColor(dc, palette::TextSecondary.to_colorref());

        RECT sub_rc{ pad, y, rc.right - pad, y + MulDiv(28, dpi, 96) };
        DrawTextW(dc, st.spec.subtitle, -1, &sub_rc,
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

        SelectObject(dc, old);
    }

    /* Hint line (Phase 1 only) */
    if (st.body_font) {
        HFONT old = static_cast<HFONT>(SelectObject(dc, st.body_font));
        SetTextColor(dc, palette::TextMuted.to_colorref());

        const wchar_t* hint =
            L"Phase 1 - UI foundation. Wizard flows arrive in later phases.";

        RECT hint_rc{
            pad,
            rc.bottom - MulDiv(48, dpi, 96),
            rc.right - pad,
            rc.bottom - MulDiv(16, dpi, 96)
        };
        DrawTextW(dc, hint, -1, &hint_rc,
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

        SelectObject(dc, old);
    }

    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK root_wnd_proc(HWND hwnd, UINT msg,
                               WPARAM wparam, LPARAM lparam) {
    WindowState* st = reinterpret_cast<WindowState*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return 0;
    }
    case WM_ERASEBKGND:
        return 1; /* painted in WM_PAINT */

    case WM_PAINT:
        if (st) {
            paint_content(hwnd, *st);
        } else {
            PAINTSTRUCT ps{};
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
        }
        return 0;

    case WM_DPICHANGED: {
        auto* suggested = reinterpret_cast<RECT*>(lparam);
        if (suggested) {
            SetWindowPos(hwnd, nullptr,
                         suggested->left, suggested->top,
                         suggested->right  - suggested->left,
                         suggested->bottom - suggested->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

ATOM register_window_class(HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = root_wnd_proc;
    wc.hInstance     = instance;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; /* handled in WM_PAINT */
    wc.lpszClassName = kNotyWindowClassName;

    /* Icon resource may or may not be embedded. LoadIconW tolerates the
     * missing case by returning nullptr; Windows then uses its default. */
    wc.hIcon         = LoadIconW(instance, MAKEINTRESOURCEW(1));
    wc.hIconSm       = wc.hIcon;

    return RegisterClassExW(&wc);
}

} // namespace

int run_root_window(HINSTANCE instance, const RootWindowSpec& spec) {
    /* Idempotent class registration. */
    if (!GetClassInfoExW(instance, kNotyWindowClassName, nullptr)) {
        if (register_window_class(instance) == 0) {
            MessageBoxW(nullptr,
                        L"Failed to register the NotY window class.",
                        L"NotY",
                        MB_ICONERROR | MB_OK);
            return 1;
        }
    }

    WindowState state{};
    state.spec     = spec;
    state.instance = instance;
    state.logo     = load_embedded_logo(instance);

    const UINT dpi = system_dpi();

    const int title_px    = MulDiv(30, dpi, 96);
    const int subtitle_px = MulDiv(15, dpi, 96);
    const int body_px     = MulDiv(13, dpi, 96);

    state.title_font = CreateFontW(
        -title_px, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    state.subtitle_font = CreateFontW(
        -subtitle_px, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    state.body_font = CreateFontW(
        -body_px, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    RECT desired{ 0, 0, spec.width_px, spec.height_px };
    adjust_window_rect_for_dpi(&desired, WS_OVERLAPPEDWINDOW, FALSE, 0, dpi);

    HWND hwnd = CreateWindowExW(
        0,
        kNotyWindowClassName,
        spec.title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        desired.right  - desired.left,
        desired.bottom - desired.top,
        nullptr, nullptr, instance,
        &state);

    if (!hwnd) {
        if (state.title_font)    DeleteObject(state.title_font);
        if (state.subtitle_font) DeleteObject(state.subtitle_font);
        if (state.body_font)     DeleteObject(state.body_font);
        return 1;
    }

    apply_dark_titlebar(hwnd);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg{};
    int exit_code = 0;
    for (;;) {
        const BOOL r = GetMessageW(&msg, nullptr, 0, 0);
        if (r == 0) {
            exit_code = static_cast<int>(msg.wParam);
            break;
        }
        if (r == -1) {
            exit_code = 1;
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (state.title_font)    DeleteObject(state.title_font);
    if (state.subtitle_font) DeleteObject(state.subtitle_font);
    if (state.body_font)     DeleteObject(state.body_font);

    return exit_code;
}

} // namespace noty::ui