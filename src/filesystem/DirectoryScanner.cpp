#include "noty/filesystem/DirectoryScanner.h"
#include "noty/common/Logger.h"

namespace noty {

    DirectoryScanner::DirectoryScanner() = default;
    DirectoryScanner::~DirectoryScanner() = default;

    DirectoryScanner::ScanResult DirectoryScanner::scanDirectory(
        const std::string& directory,
        bool recursive,
        bool includeHidden) {

        Logger::instance().info("Starting directory scan: " + directory);

        ScanResult result;
        m_cancelled = false;
        m_totalSize = 0;
        m_fileCount = 0;
        m_directoryCount = 0;

        try {
            // Use FileEnumerator for the actual enumeration
            FileEnumerator enumerator;

            // Set up callbacks
            if (m_fileFoundCallback) {
                enumerator.setFileCallback([this](const FileInfo& file) {
                    if (m_fileFoundCallback) {
                        m_fileFoundCallback(file);
                    }
                    m_fileCount++;
                    m_totalSize += file.size;

                    if (m_scanCallback && m_fileCount % 10 == 0) {
                        m_scanCallback(static_cast<int>(m_fileCount), 0,
                            "Found " + std::to_string(m_fileCount) + " files...");
                    }
                    });
            }

            // Also set progress callback
            if (m_scanCallback) {
                enumerator.setProgressCallback([this](int64_t files, int64_t size) {
                    m_scanCallback(static_cast<int>(files), 0,
                        "Found " + std::to_string(files) + " files (" +
                        std::to_string(size / (1024 * 1024)) + " MB)");
                    });
            }

            // Perform enumeration
            enumerator.enumerateDirectory(directory, recursive, includeHidden);

            // Collect results
            if (!m_cancelled) {
                auto stats = enumerator.getStatistics();
                result.files = enumerator.getFiles();
                result.totalSize = stats.totalSize;
                result.fileCount = stats.totalFiles;
                result.directoryCount = stats.totalDirectories;
                result.success = true;

                m_totalSize = stats.totalSize;
                m_fileCount = stats.totalFiles;
                m_directoryCount = stats.totalDirectories;

                Logger::instance().info("Scan complete: " +
                    std::to_string(m_fileCount) + " files, " +
                    std::to_string(m_totalSize) + " bytes");
            }
            else {
                result.success = false;
                result.errorMessage = "Scan cancelled by user";
                Logger::instance().info("Scan cancelled.");
            }

        }
        catch (const std::exception& e) {
            result.success = false;
            result.errorMessage = e.what();
            Logger::instance().error("Scan failed: " + std::string(e.what()));
        }

        return result;
    }

    void DirectoryScanner::setScanCallback(ScanCallback callback) {
        m_scanCallback = std::move(callback);
    }

    void DirectoryScanner::setFileFoundCallback(FileFoundCallback callback) {
        m_fileFoundCallback = std::move(callback);
    }

    void DirectoryScanner::cancel() {
        m_cancelled = true;
        Logger::instance().info("Directory scanner cancellation requested.");
    }

} // namespace noty