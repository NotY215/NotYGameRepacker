#pragma once
#include "FileInfo.h"
#include <vector>
#include <string>
#include <functional>
#include <atomic>

namespace noty {

    class FileEnumerator {
    public:
        using ProgressCallback = std::function<void(int64_t files, int64_t totalSize)>;
        using FileCallback = std::function<void(const FileInfo& file)>;

        FileEnumerator();
        ~FileEnumerator();

        // Main enumeration function
        void enumerateDirectory(const std::string& directory,
            bool recursive = true,
            bool includeHidden = false);

        // Get results
        const std::vector<FileInfo>& getFiles() const { return m_files; }
        int64_t getTotalFiles() const { return m_totalFiles; }
        int64_t getTotalSize() const { return m_totalSize; }
        int64_t getDirectoryCount() const { return m_directoryCount; }

        // Callbacks
        void setProgressCallback(ProgressCallback callback);
        void setFileCallback(FileCallback callback);

        // Cancellation support
        void cancel();
        bool isCancelled() const { return m_cancelled; }

        // Statistics
        struct Statistics {
            int64_t totalFiles = 0;
            int64_t totalSize = 0;
            int64_t totalDirectories = 0;
        };

        Statistics getStatistics() const;

    private:
        void enumerateRecursive(const std::string& directory,
            const std::string& rootPath,
            bool includeHidden);

        bool shouldIncludeFile(const std::string& path, bool includeHidden);

        std::vector<FileInfo> m_files;
        std::atomic<int64_t> m_totalFiles{ 0 };
        std::atomic<int64_t> m_totalSize{ 0 };
        std::atomic<int64_t> m_directoryCount{ 0 };
        std::atomic<bool> m_cancelled{ false };

        ProgressCallback m_progressCallback;
        FileCallback m_fileCallback;

        std::mutex m_mutex;
    };

} // namespace noty