#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <memory>

namespace noty::ui {

/* RAII wrapper for a GDI+ process-lifetime session. Construct exactly one
 * of these in wWinMain, before creating any window, and keep it alive. */
class GdiPlusSession {
public:
    GdiPlusSession();
    ~GdiPlusSession();

    GdiPlusSession(const GdiPlusSession&)            = delete;
    GdiPlusSession& operator=(const GdiPlusSession&) = delete;

    bool ok() const noexcept { return ok_; }

private:
    ULONG_PTR token_ = 0;
    bool      ok_    = false;
};

/* Owns a decoded GDI+ bitmap together with the memory and stream it was
 * created from, because GDI+ may lazily reference the source stream. */
class BitmapHandle {
public:
    BitmapHandle() = default;
    ~BitmapHandle();

    BitmapHandle(const BitmapHandle&)            = delete;
    BitmapHandle& operator=(const BitmapHandle&) = delete;

    bool valid() const noexcept;

    /* Owned state - treat as read-only from outside this module. */
    std::unique_ptr<Gdiplus::Bitmap> bitmap;
    HGLOBAL                          mem    = nullptr;
    IStream*                         stream = nullptr;
};

/* Loads the embedded logo from the given module. Returns nullptr when the
 * resource is not embedded (build-time NOTY_HAS_LOGO undefined) or when
 * decoding fails. */
std::unique_ptr<BitmapHandle> load_embedded_logo(HMODULE module);

} // namespace noty::ui