#pragma once
#include "FileInfo.h"
#include "FileEnumerator.h"
#include <string>
#include <vector>
#include <functional>

namespace noty {

    class DirectoryScanner {
    public:
        using ScanCallback = std::function<void(int progress, int total, const std::string& status)>;
        using FileFoundCallback = std::function<void(const FileInfo& file)>;

        DirectoryScanner();
        ~DirectoryScanner();

        // Scan a directory and return file information
        ScanResult scanDirectory(const std::string& directory,
            bool recursive = true,
            bool includeHidden = false);

        // Get total size of files (returns 0 if not scanned)
        uint64_t getTotalSize() const { return m_totalSize; }
        uint64_t getFileCount() const { return m_fileCount; }
        uint64_t getDirectoryCount() const { return m_directoryCount; }

        // Callbacks for progress
        void setScanCallback(ScanCallback callback);
        void setFileFoundCallback(FileFoundCallback callback);

        // Cancellation
        void cancel();
        bool isCancelled() const { return m_cancelled; }

        // Statistics structure
        struct ScanResult {
            std::vector<FileInfo> files;
            uint64_t totalSize = 0;
            uint64_t fileCount = 0;
            uint64_t directoryCount = 0;
            bool success = false;
            std::string errorMessage;
        };

    private:
        void processFiles(const std::vector<FileInfo>& files);

        uint64_t m_totalSize = 0;
        uint64_t m_fileCount = 0;
        uint64_t m_directoryCount = 0;
        std::atomic<bool> m_cancelled{ false };

        ScanCallback m_scanCallback;
        FileFoundCallback m_fileFoundCallback;
    };

} // namespace noty