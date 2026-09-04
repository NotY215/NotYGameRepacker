#include "noty/repacker/PackageBuilder.h"
#include "noty/common/Logger.h"
#include "noty/common/Constants.h"
#include "noty/filesystem/DirectoryScanner.h"
#include "noty/compression/ZstdCompressor.h"
#include "noty/hashing/Hasher.h"
#include "noty/package/ManifestWriter.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <iomanip>

namespace fs = std::filesystem;

namespace noty {

    PackageBuilder::PackageBuilder()
        : m_cancelled(false)
        , m_totalProcessedSize(0)
        , m_processedFileCount(0)
    {
        m_compressor = std::make_unique<ZstdCompressor>(19, 1024 * 1024);
        m_hasher = std::make_unique<Hasher>(Hasher::Algorithm::BLAKE3);

        m_fileBuffer.resize(1024 * 1024);
        m_compressedBuffer.resize(1024 * 1024 * 2);
    }

    PackageBuilder::~PackageBuilder() = default;

    PackageBuilder::PackageBuilder(PackageBuilder&& other) noexcept
        : m_compressor(std::move(other.m_compressor))
        , m_hasher(std::move(other.m_hasher))
        , m_cancelled(other.m_cancelled.load())
        , m_lastError(std::move(other.m_lastError))
        , m_totalProcessedSize(other.m_totalProcessedSize)
        , m_processedFileCount(other.m_processedFileCount)
        , m_setupPath(std::move(other.m_setupPath))
        , m_chunkPaths(std::move(other.m_chunkPaths))
        , m_fileBuffer(std::move(other.m_fileBuffer))
        , m_compressedBuffer(std::move(other.m_compressedBuffer))
    {
    }

    PackageBuilder& PackageBuilder::operator=(PackageBuilder&& other) noexcept {
        if (this != &other) {
            m_compressor = std::move(other.m_compressor);
            m_hasher = std::move(other.m_hasher);
            m_cancelled.store(other.m_cancelled.load());
            m_lastError = std::move(other.m_lastError);
            m_totalProcessedSize = other.m_totalProcessedSize;
            m_processedFileCount = other.m_processedFileCount;
            m_setupPath = std::move(other.m_setupPath);
            m_chunkPaths = std::move(other.m_chunkPaths);
            m_fileBuffer = std::move(other.m_fileBuffer);
            m_compressedBuffer = std::move(other.m_compressedBuffer);
        }
        return *this;
    }

    bool PackageBuilder::buildPackage(const std::string& sourceDirectory,
        const std::string& outputDirectory,
        const RepackJob::Configuration& config,
        Manifest& manifest,
        ProgressCallback progress,
        FileProgressCallback fileProgress) {
        m_cancelled = false;
        m_lastError.clear();
        m_totalProcessedSize = 0;
        m_processedFileCount = 0;
        m_chunkPaths.clear();
        m_setupPath.clear();

        BuildContext ctx;
        ctx.sourceDirectory = sourceDirectory;
        ctx.outputDirectory = outputDirectory;
        ctx.config = config;
        ctx.manifest = &manifest;
        ctx.progress = progress;
        ctx.fileProgress = fileProgress;

        Logger::instance().info("Starting package build for: " + sourceDirectory);

        try {
            if (!fs::exists(outputDirectory)) {
                fs::create_directories(outputDirectory);
            }

            updateProgress(ctx, 5, "Scanning source directory...");
            if (!scanSourceDirectory(ctx)) {
                return false;
            }

            updateProgress(ctx, 15, "Calculating file hashes...");
            if (!calculateHashes(ctx)) {
                return false;
            }

            updateProgress(ctx, 25, "Building chunks...");
            if (!buildChunks(ctx)) {
                return false;
            }

            updateProgress(ctx, 75, "Writing chunks...");
            if (!writeChunks(ctx)) {
                return false;
            }

            updateProgress(ctx, 85, "Writing manifest...");
            if (!writeManifest(ctx)) {
                return false;
            }

            updateProgress(ctx, 90, "Writing cover image...");
            if (!writeCoverImage(ctx)) {
                return false;
            }

            if (config.generateSetup) {
                updateProgress(ctx, 92, "Generating setup...");
                if (!generateSetup(ctx)) {
                    Logger::instance().warning("Failed to generate setup.exe, continuing...");
                }
            }

            updateProgress(ctx, 100, "Package build complete!");

            manifest.setPackageInfo(ctx.manifest->getPackageInfo());

            Logger::instance().info("Package build complete: " +
                std::to_string(ctx.fileCount) + " files, " +
                std::to_string(ctx.totalSize) + " bytes");

            return true;
        }
        catch (const std::exception& e) {
            m_lastError = "Build failed: " + std::string(e.what());
            Logger::instance().error(m_lastError);
            return false;
        }
    }

    void PackageBuilder::cancel() {
        m_cancelled.store(true, std::memory_order_release);
        if (m_compressor) {
            m_compressor->cancel();
        }
        Logger::instance().info("Package build cancellation requested");
    }

    bool PackageBuilder::scanSourceDirectory(BuildContext& ctx) {
        DirectoryScanner scanner;

        scanner.setScanCallback([&](int progress, int total, const std::string& status) {
            int percent = 5 + (progress * 10 / std::max(total, 1));
            updateProgress(ctx, percent, "Scanning: " + status);
            });

        auto result = scanner.scanDirectory(ctx.sourceDirectory, true, ctx.config.includeHiddenFiles);

        if (!result.success) {
            m_lastError = "Scan failed: " + result.errorMessage;
            Logger::instance().error(m_lastError);
            return false;
        }

        ctx.files = std::move(result.files);
        ctx.totalSize = result.totalSize;
        ctx.fileCount = result.fileCount;

        Logger::instance().info("Scan complete: " +
            std::to_string(ctx.fileCount) + " files, " +
            std::to_string(ctx.totalSize) + " bytes");

        return true;
    }

    bool PackageBuilder::calculateHashes(BuildContext& ctx) {
        size_t totalFiles = ctx.files.size();
        size_t processed = 0;

        for (const auto& fileInfo : ctx.files) {
            if (m_cancelled) {
                m_lastError = "Cancelled";
                return false;
            }

            if (fileInfo.isDirectory) {
                continue;
            }

            FileEntry entry;
            entry.path = fileInfo.relativePath;
            entry.size = fileInfo.size;

            std::string hash = m_hasher->hashFile(fileInfo.path);
            if (hash.empty()) {
                m_lastError = "Failed to hash file: " + fileInfo.path;
                Logger::instance().error(m_lastError);
                return false;
            }
            entry.hash = hash;

            ctx.fileEntries.push_back(entry);
            ctx.manifest->addFile(entry);

            processed++;
            int percent = 15 + static_cast<int>((processed * 10) / totalFiles);
            updateFileProgress(ctx, fileInfo.relativePath, percent);
        }

        Logger::instance().info("Hashes calculated for " +
            std::to_string(ctx.fileEntries.size()) + " files");

        return true;
    }

    bool PackageBuilder::buildChunks(BuildContext& ctx) {
        ctx.currentChunkId = 1;
        ctx.currentChunkSize = 0;
        ctx.currentChunkData.clear();
        ctx.currentChunkData.reserve(ctx.config.maxChunkSize);

        size_t totalFiles = ctx.fileEntries.size();
        size_t processed = 0;

        for (auto& entry : ctx.fileEntries) {
            if (m_cancelled) {
                m_lastError = "Cancelled";
                return false;
            }

            auto it = std::find_if(ctx.files.begin(), ctx.files.end(),
                [&entry](const FileInfo& info) {
                    return info.relativePath == entry.path;
                });

            if (it == ctx.files.end()) {
                continue;
            }

            std::vector<uint8_t> chunkData;
            if (!compressAndPackFile(*it, ctx, entry, chunkData)) {
                return false;
            }

            entry.offsetInChunk = ctx.currentChunkData.size();
            ctx.currentChunkData.insert(ctx.currentChunkData.end(),
                chunkData.begin(), chunkData.end());
            ctx.currentChunkSize += chunkData.size();

            if (ctx.currentChunkSize >= ctx.config.maxChunkSize) {
                if (!finalizeChunk(ctx)) {
                    return false;
                }
            }

            processed++;
            int percent = 25 + static_cast<int>((processed * 50) / totalFiles);
            updateProgress(ctx, percent, "Processing: " + entry.path);
            updateFileProgress(ctx, entry.path, percent);
        }

        if (!ctx.currentChunkData.empty()) {
            if (!finalizeChunk(ctx)) {
                return false;
            }
        }

        ctx.manifest->setChunks(ctx.chunks);
        ctx.manifest->m_packageInfo.chunkCount = static_cast<uint32_t>(ctx.chunks.size());
        ctx.manifest->m_packageInfo.fileCount = static_cast<uint32_t>(ctx.fileEntries.size());
        ctx.manifest->m_packageInfo.compressedSize = ctx.manifest->calculateTotalCompressedSize();

        Logger::instance().info("Created " + std::to_string(ctx.chunks.size()) + " chunks");

        return true;
    }

    bool PackageBuilder::compressAndPackFile(const FileInfo& fileInfo,
        BuildContext& ctx,
        FileEntry& entry,
        std::vector<uint8_t>& chunkData) {
        std::ifstream file(fileInfo.path, std::ios::binary);
        if (!file.is_open()) {
            m_lastError = "Failed to open file: " + fileInfo.path;
            Logger::instance().error(m_lastError);
            return false;
        }

        file.seekg(0, std::ios::end);
        size_t fileSize = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> fileData(fileSize);
        file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
        file.close();

        ByteVector compressedData;
        m_compressor->setCompressionLevel(ctx.config.compressionLevel);

        if (!m_compressor->compressBuffer(fileData.data(), fileData.size(), compressedData)) {
            m_lastError = "Compression failed: " + m_compressor->getLastError();
            Logger::instance().error(m_lastError);
            return false;
        }

        entry.compressedSize = compressedData.size();
        chunkData = std::move(compressedData);

        m_totalProcessedSize += fileInfo.size;
        m_processedFileCount++;

        return true;
    }

    bool PackageBuilder::finalizeChunk(BuildContext& ctx) {
        if (ctx.currentChunkData.empty()) {
            return true;
        }

        ChunkInfo chunk;
        chunk.id = ctx.currentChunkId++;
        
        std::string chunkNum = std::to_string(chunk.id);
        while (chunkNum.length() < 3) {
            chunkNum = "0" + chunkNum;
        }
        chunk.filename = ctx.config.gameName + "." + chunkNum + noty::PACKAGE_EXTENSION;
        chunk.uncompressedSize = ctx.currentChunkData.size();
        chunk.compressedSize = chunk.uncompressedSize;
        chunk.fileCount = 0;

        std::string checksum = m_hasher->hashData(ctx.currentChunkData);
        chunk.checksum = checksum;

        ctx.chunks.push_back(chunk);
        ctx.currentChunkData.clear();
        ctx.currentChunkSize = 0;

        return true;
    }

    bool PackageBuilder::writeChunks(BuildContext& ctx) {
        size_t chunkIndex = 0;
        for (auto& chunk : ctx.chunks) {
            std::vector<uint8_t> fullChunkData;

            const char* magic = "NOTY";
            fullChunkData.insert(fullChunkData.end(), magic, magic + 4);

            uint32_t version = 1;
            uint8_t* versionBytes = reinterpret_cast<uint8_t*>(&version);
            fullChunkData.insert(fullChunkData.end(), versionBytes, versionBytes + 4);

            uint32_t chunkId = chunk.id;
            uint8_t* chunkIdBytes = reinterpret_cast<uint8_t*>(&chunkId);
            fullChunkData.insert(fullChunkData.end(), chunkIdBytes, chunkIdBytes + 4);

            uint32_t totalChunks = static_cast<uint32_t>(ctx.chunks.size());
            uint8_t* totalBytes = reinterpret_cast<uint8_t*>(&totalChunks);
            fullChunkData.insert(fullChunkData.end(), totalBytes, totalBytes + 4);

            const char* compression = "ZSTD";
            fullChunkData.insert(fullChunkData.end(), compression, compression + 4);

            const char* encryption = "NONE";
            fullChunkData.insert(fullChunkData.end(), encryption, encryption + 4);

            uint64_t uncompressedSize = chunk.uncompressedSize;
            uint8_t* uncompressedBytes = reinterpret_cast<uint8_t*>(&uncompressedSize);
            fullChunkData.insert(fullChunkData.end(), uncompressedBytes, uncompressedBytes + 8);

            uint64_t compressedSize = chunk.compressedSize;
            uint8_t* compressedBytes = reinterpret_cast<uint8_t*>(&compressedSize);
            fullChunkData.insert(fullChunkData.end(), compressedBytes, compressedBytes + 8);

            fullChunkData.insert(fullChunkData.end(),
                chunk.checksum.begin(), chunk.checksum.end());

            fullChunkData.insert(fullChunkData.end(),
                ctx.currentChunkData.begin(), ctx.currentChunkData.end());

            std::string chunkPath = (fs::path(ctx.outputDirectory) / chunk.filename).string();
            std::ofstream chunkFile(chunkPath, std::ios::binary);
            if (!chunkFile.is_open()) {
                m_lastError = "Failed to create chunk file: " + chunkPath;
                Logger::instance().error(m_lastError);
                return false;
            }

            chunkFile.write(reinterpret_cast<const char*>(fullChunkData.data()),
                fullChunkData.size());
            chunkFile.close();

            m_chunkPaths.push_back(chunkPath);
            chunkIndex++;

            int percent = 75 + static_cast<int>((chunkIndex * 10) / ctx.chunks.size());
            updateProgress(ctx, percent, "Writing chunk " +
                std::to_string(chunk.id) + "/" + std::to_string(ctx.chunks.size()));
        }

        return true;
    }

    bool PackageBuilder::writeManifest(BuildContext& ctx) {
        ManifestWriter writer;

        auto& info = ctx.manifest->m_packageInfo;
        info.gameName = ctx.config.gameName;
        info.gameVersion = ctx.config.gameVersion;
        info.packageId = ctx.config.packageId;
        info.repackerName = ctx.config.repackerName;
        info.setupName = ctx.config.setupName;
        info.originalSize = ctx.totalSize;
        info.compressedSize = ctx.manifest->calculateTotalCompressedSize();
        info.compressionRatio = (info.compressedSize * 100) / std::max(info.originalSize, (uint64_t)1);
        info.compressionMethod = "Zstandard";
        info.encryptionMethod = "None";
        info.hashAlgorithm = "BLAKE3";
        info.formatVersion = 1;
        info.createdBy = "NotY Repacker v1.0";
        info.creationDate = std::chrono::system_clock::now();

        std::string manifestPath = (fs::path(ctx.outputDirectory) / noty::MANIFEST_FILENAME).string();
        if (!writer.writeToFilePretty(*ctx.manifest, manifestPath)) {
            m_lastError = "Failed to write manifest";
            Logger::instance().error(m_lastError);
            return false;
        }

        Logger::instance().info("Manifest written to: " + manifestPath);
        return true;
    }

    bool PackageBuilder::writeCoverImage(BuildContext& ctx) {
        if (ctx.config.coverImagePath.empty()) {
            Logger::instance().info("No cover image specified");
            return true;
        }

        if (!fs::exists(ctx.config.coverImagePath)) {
            Logger::instance().warning("Cover image not found: " + ctx.config.coverImagePath);
            return true;
        }

        try {
            std::string coverPath = (fs::path(ctx.outputDirectory) / noty::COVER_FILENAME).string();
            fs::copy_file(ctx.config.coverImagePath, coverPath, fs::copy_options::overwrite_existing);
            Logger::instance().info("Cover image copied to: " + coverPath);
            return true;
        }
        catch (const std::exception& e) {
            Logger::instance().warning("Failed to copy cover image: " + std::string(e.what()));
            return true;
        }
    }

    bool PackageBuilder::generateSetup(BuildContext& ctx) {
        Logger::instance().info("Setup.exe generation placeholder");
        m_setupPath = (fs::path(ctx.outputDirectory) / ctx.config.setupName).string();
        return true;
    }

    void PackageBuilder::updateProgress(BuildContext& ctx, int percent, const std::string& status) {
        if (ctx.progress) {
            ctx.progress(std::min(percent, 100), status);
        }
    }

    void PackageBuilder::updateFileProgress(BuildContext& ctx, const std::string& filename, int percent) {
        if (ctx.fileProgress) {
            ctx.fileProgress(filename, std::min(percent, 100));
        }
    }

} // namespace noty