================================================================================
FILE: src / installer / ExtractionEngine.cpp
================================================================================
#include "noty/installer/ExtractionEngine.h"
#include "noty/common/Logger.h"
#include "noty/common/Constants.h"
#include "noty/compression/ZstdDecompressor.h"
#include "noty/crypto/Decryptor.h"
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
        m_decryptor = std::make_unique<Decryptor>(1024 * 1024);
        m_hasher = std::make_unique<Hasher>(Hasher::Algorithm::BLAKE3);

        // Allocate buffers
        m_fileBuffer.resize(1024 * 1024);
        m_decompressedBuffer.resize(1024 * 1024 * 2);
        m_decryptedBuffer.resize(1024 * 1024 * 2);
    }

    ExtractionEngine::~ExtractionEngine() = default;

    ExtractionEngine::ExtractionEngine(ExtractionEngine&& other) noexcept
        : m_decompressor(std::move(other.m_decompressor))
        , m_decryptor(std::move(other.m_decryptor))
        , m_hasher(std::move(other.m_hasher))
        , m_cancelled(other.m_cancelled.load())
        , m_lastError(std::move(other.m_lastError))
        , m_extractedSize(other.m_extractedSize)
        , m_extractedFileCount(other.m_extractedFileCount)
        , m_fileBuffer(std::move(other.m_fileBuffer))
        , m_decompressedBuffer(std::move(other.m_decompressedBuffer))
        , m_decryptedBuffer(std::move(other.m_decryptedBuffer))
    {
    }

    ExtractionEngine& ExtractionEngine::operator=(ExtractionEngine&& other) noexcept {
        if (this != &other) {
            m_decompressor = std::move(other.m_decompressor);
            m_decryptor = std::move(other.m_decryptor);
            m_hasher = std::move(other.m_hasher);
            m_cancelled.store(other.m_cancelled.load());
            m_lastError = std::move(other.m_lastError);
            m_extractedSize = other.m_extractedSize;
            m_extractedFileCount = other.m_extractedFileCount;
            m_fileBuffer = std::move(other.m_fileBuffer);
            m_decompressedBuffer = std::move(other.m_decompressedBuffer);
            m_decryptedBuffer = std::move(other.m_decryptedBuffer);
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
            // Create install directory if it doesn't exist
            if (!fs::exists(installDirectory)) {
                fs::create_directories(installDirectory);
            }

            // Phase 1: Read manifest
            updateProgress(ctx, 5, "Reading package manifest...");
            if (!readManifest(ctx)) {
                return false;
            }

            // Phase 2: Prepare install directory
            updateProgress(ctx, 10, "Preparing installation directory...");
            if (!prepareInstallDirectory(ctx)) {
                return false;
            }

            // Phase 3: Extract chunks
            updateProgress(ctx, 15, "Extracting files...");
            if (!extractChunks(ctx)) {
                return false;
            }

            // Phase 4: Verify if requested
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
        if (m_decryptor) {
            m_decryptor->cancel();
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

            // Check if file exists
            if (!fs::exists(filePath)) {
                Logger::instance().error("File missing: " + entry.path);
                allValid = false;
                continue;
            }

            // Check file size
            auto fileSize = fs::file_size(filePath);
            if (fileSize != entry.size) {
                Logger::instance().error("File size mismatch: " + entry.path +
                    " (expected " + std::to_string(entry.size) +
                    ", got " + std::to_string(fileSize) + ")");
                allValid = false;
                continue;
            }

            // Check hash
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

            // Check if it's a directory containing chunks
            if (fs::is_directory(packageDir)) {
                for (const auto& entry : fs::directory_iterator(packageDir)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".noty") {
                        totalSize += fs::file_size(entry.path());
                    }
                }
            }
            else if (fs::is_regular_file(packageDir) &&
                packageDir.extension() == ".noty") {
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

        // Check if packagePath is a directory or file
        fs::path path(ctx.packagePath);

        if (fs::is_directory(path)) {
            manifestPath = (path / Constants::MANIFEST_FILENAME).string();
        }
        else if (fs::is_regular_file(path)) {
            // Try to find manifest in the same directory
            manifestPath = (path.parent_path() / Constants::MANIFEST_FILENAME).string();
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

        // Calculate total size from manifest
        ctx.totalSize = ctx.manifest->calculateTotalFileSize();
        ctx.fileCount = ctx.manifest->getFiles().size();

        Logger::instance().info("Manifest loaded: " + std::to_string(ctx.fileCount) +
            " files, " + std::to_string(ctx.totalSize) + " bytes");

        // Initialize decryptor if encryption is enabled
        if (ctx.config.enableEncryption)