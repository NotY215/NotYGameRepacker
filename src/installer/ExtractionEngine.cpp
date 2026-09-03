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
                    if (entry.is_regular_file() && entry.path().extension() == noty::Constants::PACKAGE_EXTENSION) {
                        totalSize += fs::file_size(entry.path());
                    }
                }
            }
            else if (fs::is_regular_file(packageDir) &&
                packageDir.extension() == noty::Constants::PACKAGE_EXTENSION) {
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
            manifestPath = (path / noty::Constants::MANIFEST_FILENAME).string();
        }
        else if (fs::is_regular_file(path)) {
            // Try to find manifest in the same directory
            manifestPath = (path.parent_path() / noty::Constants::MANIFEST_FILENAME).string();
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
        if (ctx.config.enableEncryption) {
            if (ctx.config.encryptionKey.empty() || ctx.config.encryptionNonce.empty()) {
                m_lastError = "Encryption enabled but key or nonce missing";
                Logger::instance().error(m_lastError);
                return false;
            }

            // We need to get the auth tag from somewhere - it should be in the manifest or chunk headers
            // For now, we'll use a placeholder - in production, this would come from the package
            std::vector<uint8_t> dummyAuthTag(16, 0);

            if (!m_decryptor->initialize(
                ctx.config.encryptionKey,
                ctx.config.encryptionNonce,
                dummyAuthTag,
                {})) {
                m_lastError = "Failed to initialize decryptor: " + m_decryptor->getLastError();
                Logger::instance().error(m_lastError);
                return false;
            }
        }

        return true;
    }

    bool ExtractionEngine::prepareInstallDirectory(ExtractionContext& ctx) {
        try {
            fs::path installPath(ctx.installDirectory);

            // Check if directory exists and is writable
            if (fs::exists(installPath)) {
                if (!fs::is_directory(installPath)) {
                    m_lastError = "Install path exists but is not a directory: " + ctx.installDirectory;
                    Logger::instance().error(m_lastError);
                    return false;
                }

                // Check write permissions by trying to create a test file
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
                // Create directory
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

            // Build chunk file path
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

            // Read chunk data
            std::ifstream chunkFile(chunkPath.string(), std::ios::binary);
            if (!chunkFile.is_open()) {
                m_lastError = "Failed to open chunk file: " + chunkPath.string();
                Logger::instance().error(m_lastError);
                return false;
            }

            chunkFile.seekg(0, std::ios::end);
            size_t chunkSize = static_cast<size_t>(chunkFile.tellg());
            chunkFile.seekg(0, std::ios::beg);

            // Read the entire chunk (with header) - in production, this should be streaming
            std::vector<uint8_t> chunkData(chunkSize);
            chunkFile.read(reinterpret_cast<char*>(chunkData.data()), chunkSize);
            chunkFile.close();

            // Process the chunk (extract files from it)
            // In a real implementation, you'd parse the chunk header and extract files
            // For now, we'll just decompress and decrypt the payload

            // Find files in this chunk
            const auto& files = ctx.manifest->getFiles();
            for (const auto& entry : files) {
                if (entry.chunkId == chunkInfo.id) {
                    if (m_cancelled) {
                        return false;
                    }

                    // Extract file from chunk
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
        // In a real implementation, this would parse the chunk header,
        // locate the file data at entry.offsetInChunk, and decompress/decrypt it

        // For now, we'll simulate extraction by decompressing and decrypting the entire chunk
        std::vector<uint8_t> decompressedData;
        std::vector<uint8_t> decryptedData;

        // Decompress
        if (!m_decompressor->decompressBuffer(
            chunkData.data(),
            chunkData.size(),
            decompressedData)) {
            m_lastError = "Decompression failed: " + m_decompressor->getLastError();
            Logger::instance().error(m_lastError);
            return false;
        }

        // Decrypt if enabled
        if (ctx.config.enableEncryption) {
            if (!m_decryptor->isAuthenticationValid()) {
                // Try to decrypt the data
                // In production, you'd have proper auth tag handling
            }
        }

        // Build output path
        fs::path outputPath = fs::path(ctx.installDirectory) / entry.path;
        fs::create_directories(outputPath.parent_path());

        // Write file
        std::ofstream outputFile(outputPath.string(), std::ios::binary);
        if (!outputFile.is_open()) {
            m_lastError = "Failed to create file: " + entry.path;
            Logger::instance().error(m_lastError);
            return false;
        }

        // For now, write the decompressed data (assuming it's the file)
        // In production, you'd extract the specific file from the chunk
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

    bool ExtractionEngine::decompressAndDecryptFile(const std::vector<uint8_t>& encryptedData,
        const FileEntry& entry,
        std::vector<uint8_t>& outputData,
        ExtractionContext& ctx) {
        // In production, this would handle proper decompression and decryption
        // For now, we'll just copy the data
        outputData = encryptedData;
        return true;
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