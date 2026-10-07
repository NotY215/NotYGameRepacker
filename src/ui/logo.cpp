#include "logo.hpp"
#include "noty_resource.h"

#include <windows.h>
#include <objbase.h>

namespace noty::ui {

GdiPlusSession::GdiPlusSession() {
    Gdiplus::GdiplusStartupInput input;
    Gdiplus::Status status = Gdiplus::GdiplusStartup(&token_, &input, nullptr);
    ok_ = (status == Gdiplus::Ok);
}

GdiPlusSession::~GdiPlusSession() {
    if (ok_) {
        Gdiplus::GdiplusShutdown(token_);
    }
    token_ = 0;
    ok_    = false;
}

BitmapHandle::~BitmapHandle() {
    /* Destroy the bitmap first: it may hold a reference to the stream. */
    bitmap.reset();

    if (stream) {
        stream->Release();
        stream = nullptr;
    }
    if (mem) {
        GlobalFree(mem);
        mem = nullptr;
    }
}

bool BitmapHandle::valid() const noexcept {
    return bitmap != nullptr &&
           bitmap->GetLastStatus() == Gdiplus::Ok;
}

std::unique_ptr<BitmapHandle> load_embedded_logo(HMODULE module) {
#ifdef NOTY_HAS_LOGO
    if (!module) {
        return nullptr;
    }

    HRSRC res = FindResourceW(module,
                              MAKEINTRESOURCEW(IDR_LOGO_PNG),
                              RT_RCDATA);
    if (!res) {
        return nullptr;
    }

    const DWORD size = SizeofResource(module, res);
    if (size == 0) {
        return nullptr;
    }

    HGLOBAL loaded = LoadResource(module, res);
    if (!loaded) {
        return nullptr;
    }

    const void* src = LockResource(loaded);
    if (!src) {
        return nullptr;
    }

    /* GDI+ needs an IStream. Copy the resource into a moveable HGLOBAL and
     * wrap it with a stream. We keep ownership of both. */
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!mem) {
        return nullptr;
    }

    void* dst = GlobalLock(mem);
    if (!dst) {
        GlobalFree(mem);
        return nullptr;
    }
    memcpy(dst, src, size);
    GlobalUnlock(mem);

    IStream* stream = nullptr;
    HRESULT hr = CreateStreamOnHGlobal(mem, FALSE, &stream);
    if (FAILED(hr) || !stream) {
        GlobalFree(mem);
        return nullptr;
    }

    auto handle    = std::make_unique<BitmapHandle>();
    handle->mem    = mem;
    handle->stream = stream;
    handle->bitmap = std::make_unique<Gdiplus::Bitmap>(stream, FALSE);

    if (!handle->valid()) {
        /* Unique_ptr destructor releases stream and memory via BitmapHandle. */
        return nullptr;
    }

    return handle;
#else
    (void)module;
    return nullptr;
#endif
}

} // namespace noty::ui