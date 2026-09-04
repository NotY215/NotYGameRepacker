#include "noty/installer/ExtractionEngine.h"
#include "noty/common/Logger.h"
#include "noty/common/Constants.h"
#include "noty/compression/ZstdDecompressor.h"
#include "noty/hashing/Hasher.h"
#include "noty/package/ManifestReader.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstring>

namespace fs = std::filesystem;

namespace noty {

    ExtractionEngine::ExtractionEngine()
        : m_cancelled(false)
        , m_extractedSize(0)
        , m_extractedFileCount(0)
    {
        m_decompressor = std::make_unique<ZstdDecompressor>(1024 * 1024);
        m_hasher = std::make_unique<Hasher>(Hasher::Algorithm::BLAKE3);

        m_fileBuffer.resize(1024 * 1024);
        m_decompressedBuffer.resize(1024 * 1024 * 2);
    }

    ExtractionEngine::~ExtractionEngine() = default;

    ExtractionEngine::ExtractionEngine(ExtractionEngine&& other) noexcept
        : m_decompressor(std::move(other.m_decompressor))
        , m_hasher(std::move(other.m_hasher))
        , m_cancelled(other.m_cancelled.load())
        , m_lastError(std::move(other.m_lastError))
        , m_extractedSize(other.m_extractedSize)
        , m_extractedFileCount(other.m_extractedFileCount)
        , m_fileBuffer(std::move(other.m_fileBuffer))
        , m_decompressedBuffer(std::move(other.m_decompressedBuffer))
    {
    }

    ExtractionEngine& ExtractionEngine::operator=(ExtractionEngine&& other) noexcept {
        if (this != &other) {
            m_decompressor = std::move(other.m_decompressor);
            m_hasher = std::move(other.m_hasher);
            m_cancelled.store(other.m_cancelled.load());
            m_lastError = std::move(other.m_lastError);
            m_extractedSize = other.m_extractedSize;
            m_extractedFileCount = other.m_extractedFileCount;
            m_fileBuffer = std::move(other.m_fileBuffer);
            m_decompressedBuffer = std::move(other.m_decompressedBuffer);
        }
        return *this;
    }

    bool ExtractionEngine::extractPackage(const std::string& packagePath,
        const std::string& installDirectory,
        Manifest& manifest,
        const InstallJob::Configuration& config,
        ProgressCallback progress,
        FileProgressCallback fileProgress) {
        m_cancelled = false;
        m_lastError.clear();
        m_extractedSize = 0;
        m_extractedFileCount = 0;

        ExtractionContext ctx;
        ctx.packagePath = packagePath;
        ctx.installDirectory = installDirectory;
        ctx.config = config;
        ctx.manifest = &manifest;
        ctx.progress = progress;
        ctx.fileProgress = fileProgress;

        Logger::instance().info("Starting extraction from: " + packagePath);
        Logger::instance().info("Install directory: " + installDirectory);

        try {
            if (!fs::exists(installDirectory)) {
                fs::create_directories(installDirectory);
            }

            updateProgress(ctx, 5, "Reading package manifest...");
            if (!readManifest(ctx)) {
                return false;
            }

            updateProgress(ctx, 10, "Preparing installation directory...");
            if (!prepareInstallDirectory(ctx)) {
                return false;
            }

            updateProgress(ctx, 15, "Extracting files...");
            if (!extractChunks(ctx)) {
                return false;
            }

            if (ctx.config.verifyFiles) {
                updateProgress(ctx, 95, "Verifying installation...");
                if (!verifyExtractedFiles(ctx)) {
                    return false;
                }
            }

            updateProgress(ctx, 100, "Extraction complete!");

            Logger::instance().info("Extraction complete: " +
                std::to_string(ctx.extractedFileCount) + " files, " +
                std::to_string(ctx.extractedSize) + " bytes");

            return true;
        }
        catch (const std::exception& e) {
            m_lastError = "Extraction failed: " + std::string(e.what());
            Logger::instance().error(m_lastError);
            return false;
        }
    }

    void ExtractionEngine::cancel() {
        m_cancelled.store(true, std::memory_order_release);
        if (m_decompressor) {
            m_decompressor->cancel();
        }
        Logger::instance().info("Extraction cancellation requested");
    }

    bool ExtractionEngine::verifyExtraction(const Manifest& manifest,
        const std::string& installDirectory) {
        Logger::instance().info("Starting verification of extracted files");

        const auto& files = manifest.getFiles();
        size_t totalFiles = files.size();
        size_t processed = 0;
        bool allValid = true;

        for (const auto& entry : files) {
            if (m_cancelled) {
                return false;
            }

            std::string filePath = (fs::path(installDirectory) / entry.path).string();

            if (!fs::exists(filePath)) {
                Logger::instance().error("File missing: " + entry.path);
                allValid = false;
                continue;
            }

            auto fileSize = fs::file_size(filePath);
            if (fileSize != entry.size) {
                Logger::instance().error("File size mismatch: " + entry.path +
                    " (expected " + std::to_string(entry.size) +
                    ", got " + std::to_string(fileSize) + ")");
                allValid = false;
                continue;
            }

            std::string actualHash = m_hasher->hashFile(filePath);
            if (actualHash.empty()) {
                Logger::instance().error("Failed to hash file: " + entry.path);
                allValid = false;
                continue;
            }

            if (actualHash != entry.hash) {
                Logger::instance().error("Hash mismatch: " + entry.path);
                allValid = false;
                continue;
            }

            processed++;
            if (processed % 100 == 0) {
                Logger::instance().info("Verified " + std::to_string(processed) +
                    "/" + std::to_string(totalFiles) + " files");
            }
        }

        if (allValid) {
            Logger::instance().info("All files verified successfully");
        }
        else {
            Logger::instance().error("Verification failed for some files");
        }

        return allValid;
    }

    uint64_t ExtractionEngine::getPackageSize(const std::string& packagePath) {
        uint64_t totalSize = 0;

        try {
            fs::path packageDir(packagePath);

            if (fs::is_directory(packageDir)) {
                for (const auto& entry : fs::directory_iterator(packageDir)) {
                    if (entry.is_regular_file() && entry.path().extension() == std::string(noty::Constants::PACKAGE_EXTENSION)) {
                        totalSize += fs::file_size(entry.path());
                    }
                }
            }
            else if (fs::is_regular_file(packageDir) &&
                packageDir.extension() == std::string(noty::Constants::PACKAGE_EXTENSION)) {
                totalSize = fs::file_size(packageDir);
            }
        }
        catch (const std::exception& e) {
            Logger::instance().error("Failed to get package size: " + std::string(e.what()));
        }

        return totalSize;
    }

    bool ExtractionEngine::readManifest(ExtractionContext& ctx) {
        ManifestReader reader;
        std::string manifestPath;

        fs::path path(ctx.packagePath);

        if (fs::is_directory(path)) {
            manifestPath = (path / std::string(noty::Constants::MANIFEST_FILENAME)).string();
        }
        else if (fs::is_regular_file(path)) {
            manifestPath = (path.parent_path() / std::string(noty::Constants::MANIFEST_FILENAME)).string();
        }
        else {
            m_lastError = "Invalid package path: " + ctx.packagePath;
            Logger::instance().error(m_lastError);
            return false;
        }

        if (!fs::exists(manifestPath)) {
            m_lastError = "Manifest not found: " + manifestPath;
            Logger::instance().error(m_lastError);
            return false;
        }

        auto manifestOpt = reader.readFromFile(manifestPath);
        if (!manifestOpt.has_value()) {
            m_lastError = "Failed to read manifest";
            Logger::instance().error(m_lastError);
            return false;
        }

        *ctx.manifest = std::move(manifestOpt.value());

        ctx.totalSize = ctx.manifest->calculateTotalFileSize();
        ctx.fileCount = ctx.manifest->getFiles().size();

        Logger::instance().info("Manifest loaded: " + std::to_string(ctx.fileCount) +
            " files, " + std::to_string(ctx.totalSize) + " bytes");

        return true;
    }

    bool ExtractionEngine::prepareInstallDirectory(ExtractionContext& ctx) {
        try {
            fs::path installPath(ctx.installDirectory);

            if (fs::exists(installPath)) {
                if (!fs::is_directory(installPath)) {
                    m_lastError = "Install path exists but is not a directory: " + ctx.installDirectory;
                    Logger::instance().error(m_lastError);
                    return false;
                }

                std::string testFile = ctx.installDirectory + "/.noty_test";
                std::ofstream test(testFile);
                if (!test.is_open()) {
                    m_lastError = "Install directory is not writable: " + ctx.installDirectory;
                    Logger::instance().error(m_lastError);
                    return false;
                }
                test.close();
                fs::remove(testFile);
            }
            else {
                if (!fs::create_directories(installPath)) {
                    m_lastError = "Failed to create install directory: " + ctx.installDirectory;
                    Logger::instance().error(m_lastError);
                    return false;
                }
            }

            Logger::instance().info("Install directory prepared: " + ctx.installDirectory);
            return true;
        }
        catch (const std::exception& e) {
            m_lastError = "Failed to prepare install directory: " + std::string(e.what());
            Logger::instance().error(m_lastError);
            return false;
        }
    }

    bool ExtractionEngine::extractChunks(ExtractionContext& ctx) {
        const auto& chunks = ctx.manifest->getChunks();
        size_t totalChunks = chunks.size();
        size_t processedChunks = 0;

        for (const auto& chunkInfo : chunks) {
            if (m_cancelled) {
                m_lastError = "Extraction cancelled by user";
                Logger::instance().info(m_lastError);
                return false;
            }

            fs::path chunkPath(ctx.packagePath);
            if (fs::is_directory(chunkPath)) {
                chunkPath /= chunkInfo.filename;
            }
            else if (fs::is_regular_file(chunkPath)) {
                chunkPath = chunkPath.parent_path() / chunkInfo.filename;
            }
            else {
                m_lastError = "Invalid chunk path: " + ctx.packagePath;
                Logger::instance().error(m_lastError);
                return false;
            }

            if (!fs::exists(chunkPath)) {
                m_lastError = "Chunk file not found: " + chunkPath.string();
                Logger::instance().error(m_lastError);
                return false;
            }

            std::ifstream chunkFile(chunkPath.string(), std::ios::binary);
            if (!chunkFile.is_open()) {
                m_lastError = "Failed to open chunk file: " + chunkPath.string();
                Logger::instance().error(m_lastError);
                return false;
            }

            chunkFile.seekg(0, std::ios::end);
            size_t chunkSize = static_cast<size_t>(chunkFile.tellg());
            chunkFile.seekg(0, std::ios::beg);

            std::vector<uint8_t> chunkData(chunkSize);
            chunkFile.read(reinterpret_cast<char*>(chunkData.data()), chunkSize);
            chunkFile.close();

            const auto& files = ctx.manifest->getFiles();
            for (const auto& entry : files) {
                if (entry.chunkId == chunkInfo.id) {
                    if (m_cancelled) {
                        return false;
                    }

                    if (!extractFileFromChunk(ctx, entry, chunkData)) {
                        return false;
                    }
                }
            }

            processedChunks++;
            int percent = 15 + (processedChunks * 80 / totalChunks);
            updateProgress(ctx, percent, "Extracting chunk " +
                std::to_string(processedChunks) + "/" + std::to_string(totalChunks));
        }

        return true;
    }

    bool ExtractionEngine::extractFileFromChunk(ExtractionContext& ctx,
        const FileEntry& entry,
        const std::vector<uint8_t>& chunkData) {
        std::vector<uint8_t> decompressedData;

        if (!m_decompressor->decompressBuffer(
            chunkData.data(),
            chunkData.size(),
            decompressedData)) {
            m_lastError = "Decompression failed: " + m_decompressor->getLastError();
            Logger::instance().error(m_lastError);
            return false;
        }

        fs::path outputPath = fs::path(ctx.installDirectory) / entry.path;
        fs::create_directories(outputPath.parent_path());

        std::ofstream outputFile(outputPath.string(), std::ios::binary);
        if (!outputFile.is_open()) {
            m_lastError = "Failed to create file: " + entry.path;
            Logger::instance().error(m_lastError);
            return false;
        }

        outputFile.write(
            reinterpret_cast<const char*>(decompressedData.data()),
            std::min(decompressedData.size(), static_cast<size_t>(entry.size)));
        outputFile.close();

        ctx.extractedFileCount++;
        ctx.extractedSize += entry.size;

        updateFileProgress(ctx, entry.path,
            static_cast<int>(ctx.extractedFileCount * 100 / ctx.fileCount));

        return true;
    }

    bool ExtractionEngine::verifyExtractedFiles(ExtractionContext& ctx) {
        return verifyExtraction(*ctx.manifest, ctx.installDirectory);
    }

    void ExtractionEngine::updateProgress(ExtractionContext& ctx, int percent, const std::string& status) {
        if (ctx.progress) {
            ctx.progress(std::min(percent, 100), status);
        }
    }

    void ExtractionEngine::updateFileProgress(ExtractionContext& ctx, const std::string& filename, int percent) {
        if (ctx.fileProgress) {
            ctx.fileProgress(filename, std::min(percent, 100));
        }
    }

} // namespace noty