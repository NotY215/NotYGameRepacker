#include "noty/filesystem/FileEnumerator.h"
#include "noty/common/Logger.h"
#include <filesystem>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

namespace noty {

    FileEnumerator::FileEnumerator() = default;
    FileEnumerator::~FileEnumerator() = default;

    void FileEnumerator::enumerateDirectory(const std::string& directory,
        bool recursive,
        bool includeHidden) {
        m_cancelled = false;
        m_files.clear();
        m_totalFiles = 0;
        m_totalSize = 0;
        m_directoryCount = 0;

        Logger::instance().info("Enumerating directory: " + directory);

        if (!fs::exists(directory)) {
            Logger::instance().error("Directory does not exist: " + directory);
            throw Error("Directory does not exist: " + directory);
        }

        if (!fs::is_directory(directory)) {
            Logger::instance().error("Path is not a directory: " + directory);
            throw Error("Path is not a directory: " + directory);
        }

        // Start enumeration
        auto startTime = std::chrono::steady_clock::now();

        if (recursive) {
            enumerateRecursive(directory, directory, includeHidden);
        }
        else {
            // Non-recursive enumeration
            for (const auto& entry : fs::directory_iterator(directory)) {
                if (m_cancelled) break;

                try {
                    if (entry.is_directory()) {
                        m_directoryCount++;
                        if (shouldIncludeFile(entry.path().string(), includeHidden)) {
                            FileInfo info(entry.path().string(), 0, true);
                            m_files.push_back(info);
                        }
                    }
                    else if (entry.is_regular_file()) {
                        auto size = entry.file_size();
                        if (shouldIncludeFile(entry.path().string(), includeHidden)) {
                            FileInfo info(entry.path().string(), size, false);
                            m_files.push_back(info);
                            m_totalFiles++;
                            m_totalSize += size;

                            if (m_fileCallback) {
                                m_fileCallback(info);
                            }
                        }
                    }
                }
                catch (const std::exception& e) {
                    Logger::instance().warning("Error processing entry: " + std::string(e.what()));
                }
            }
        }

        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        Logger::instance().info("Enumeration complete: " +
            std::to_string(m_totalFiles) + " files, " +
            std::to_string(m_totalSize) + " bytes, " +
            std::to_string(m_directoryCount) + " directories in " +
            std::to_string(duration.count()) + "ms");

        if (m_progressCallback) {
            m_progressCallback(m_totalFiles, m_totalSize);
        }
    }

    void FileEnumerator::enumerateRecursive(const std::string& directory,
        const std::string& rootPath,
        bool includeHidden) {
        try {
            for (const auto& entry : fs::recursive_directory_iterator(
                directory, fs::directory_options::skip_permission_denied)) {

                if (m_cancelled) break;

                try {
                    auto path = entry.path().string();

                    if (entry.is_directory()) {
                        m_directoryCount++;

                        if (shouldIncludeFile(path, includeHidden)) {
                            FileInfo info(path, 0, true);
                            // Calculate relative path
                            info.relativePath = fs::relative(path, rootPath).string();
                            m_files.push_back(info);
                        }
                    }
                    else if (entry.is_regular_file()) {
                        auto size = entry.file_size();

                        if (shouldIncludeFile(path, includeHidden)) {
                            FileInfo info(path, size, false);
                            info.relativePath = fs::relative(path, rootPath).string();
                            m_files.push_back(info);
                            m_totalFiles++;
                            m_totalSize += size;

                            if (m_fileCallback) {
                                m_fileCallback(info);
                            }

                            // Update progress periodically
                            if (m_totalFiles % 100 == 0 && m_progressCallback) {
                                m_progressCallback(m_totalFiles, m_totalSize);
                            }
                        }
                    }
                }
                catch (const std::exception& e) {
                    Logger::instance().warning("Error processing file: " + std::string(e.what()));
                }
            }
        }
        catch (const std::exception& e) {
            Logger::instance().error("Enumeration error: " + std::string(e.what()));
            throw Error("Failed to enumerate directory: " + std::string(e.what()));
        }
    }

    bool FileEnumerator::shouldIncludeFile(const std::string& path, bool includeHidden) {
        if (!includeHidden) {
            // Check if file/directory is hidden
            fs::path p(path);
            auto filename = p.filename().string();
            if (!filename.empty() && filename[0] == '.') {
                return false;
            }
            // Check Windows hidden attribute
            DWORD attributes = GetFileAttributesA(path.c_str());
            if (attributes != INVALID_FILE_ATTRIBUTES &&
                (attributes & FILE_ATTRIBUTE_HIDDEN)) {
                return false;
            }
        }
        return true;
    }

    void FileEnumerator::setProgressCallback(ProgressCallback callback) {
        m_progressCallback = std::move(callback);
    }

    void FileEnumerator::setFileCallback(FileCallback callback) {
        m_fileCallback = std::move(callback);
    }

    void FileEnumerator::cancel() {
        m_cancelled = true;
        Logger::instance().info("Enumeration cancelled.");
    }

    FileEnumerator::Statistics FileEnumerator::getStatistics() const {
        Statistics stats;
        stats.totalFiles = m_totalFiles;
        stats.totalSize = m_totalSize;
        stats.totalDirectories = m_directoryCount;
        return stats;
    }

} // namespace noty