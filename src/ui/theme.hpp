#pragma once

#include <windows.h>

namespace noty::ui {

struct Color {
    BYTE r;
    BYTE g;
    BYTE b;

    constexpr COLORREF to_colorref() const noexcept {
        return RGB(r, g, b);
    }
};

namespace palette {
inline constexpr Color Background    { 0x12, 0x12, 0x14 };
inline constexpr Color Surface       { 0x1C, 0x1C, 0x20 };
inline constexpr Color SurfaceRaised { 0x24, 0x24, 0x2A };
inline constexpr Color Border        { 0x2E, 0x2E, 0x36 };
inline constexpr Color TextPrimary   { 0xF2, 0xF2, 0xF5 };
inline constexpr Color TextSecondary { 0xA0, 0xA0, 0xA8 };
inline constexpr Color TextMuted     { 0x6E, 0x6E, 0x78 };
inline constexpr Color Accent        { 0x6C, 0x5C, 0xE7 };
inline constexpr Color AccentHover   { 0x7E, 0x70, 0xEF };
inline constexpr Color AccentPressed { 0x5A, 0x4A, 0xD4 };
inline constexpr Color Success       { 0x4C, 0xC3, 0x8A };
inline constexpr Color Warning       { 0xE7, 0xB4, 0x4C };
inline constexpr Color Error         { 0xE7, 0x5C, 0x5C };
}

inline constexpr wchar_t kNotyWindowClassName[] = L"NotY.RootWindow";

/* Enable per-monitor-v2 DPI awareness. Must be called before any HWND is
 * created (i.e. at the very top of wWinMain). */
void enable_dpi_awareness();

/* Apply a dark immersive title bar and, on Windows 11, rounded corners.
 * Safe to call on any supported Windows version. */
void apply_dark_titlebar(HWND hwnd);

} // namespace noty::ui