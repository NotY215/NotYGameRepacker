#include "noty/filesystem/FileInfo.h"
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace noty {

    FileInfo::FileInfo(const std::string& path, uint64_t size, bool isDir)
        : path(path)
        , size(size)
        , isDirectory(isDir)
    {
        fs::path p(path);
        filename = p.filename().string();

        if (fs::exists(p)) {
            auto ftime = fs::last_write_time(p);
            lastModified = std::chrono::duration_cast<std::chrono::seconds>(
                ftime.time_since_epoch()).count();
        }
    }

    std::string FileInfo::getExtension() const {
        fs::path p(path);
        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }

    bool FileInfo::isExecutable() const {
        std::string ext = getExtension();
        return ext == ".exe" || ext == ".com" || ext == ".bat" ||
            ext == ".cmd" || ext == ".msi";
    }

    bool FileInfo::isGameExecutable() const {
        // Common game executable patterns
        std::string name = filename;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        return name == "game.exe" || name == "launcher.exe" ||
            name.find("game") != std::string::npos ||
            name.find("launcher") != std::string::npos;
    }

} // namespace noty