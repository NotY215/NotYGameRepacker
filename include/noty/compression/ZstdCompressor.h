================================================================================
FILE: include / noty / compression / ZstdCompressor.h
================================================================================
#pragma once
#include "../common/Common.h"
#include <string>
#include <functional>
#include <atomic>
#include <memory>
#include <cstdint>

struct ZSTD_CCtx_s;

namespace noty {

    /**
     * @brief Streaming Zstandard compressor with bounded buffers
     *
     * This class provides memory-efficient compression for large files
     * using a producer/consumer pattern with configurable buffer sizes.
     * Supports compression levels 1-22 and progress reporting.
     */
    class ZstdCompressor {
    public:
        using ProgressCallback = std::function<void(uint64_t bytesProcessed, uint64_t totalBytes)>;
        using DataCallback = std::function<void(const uint8_t* data, size_t size)>;

        /**
         * @brief Constructor
         * @param compressionLevel Zstd compression level (1-22, default 19)
         * @param bufferSize Internal buffer size for streaming (default 1MB)
         */
        explicit ZstdCompressor(int compressionLevel = 19, size_t bufferSize = 1024 * 1024);
        ~ZstdCompressor();

        // Non-copyable
        ZstdCompressor(const ZstdCompressor&) = delete;
        ZstdCompressor& operator=(const ZstdCompressor&) = delete;

        // Movable
        ZstdCompressor(ZstdCompressor&& other) noexcept;
        ZstdCompressor& operator=(ZstdCompressor&& other) noexcept;

        /**
         * @brief Compress a file to another file with streaming
         * @param inputFile Path to input file
         * @param outputFile Path to output compressed file
         * @param progress Optional progress callback
         * @return true on success, false on failure
         */
        bool compressFile(const std::string& inputFile,
            const std::string& outputFile,
            ProgressCallback progress = nullptr);

        /**
         * @brief Compress data with streaming to callback
         * @param inputFile Path to input file
         * @param dataCallback Called for each compressed chunk
         * @param progress Optional progress callback
         * @return true on success, false on failure
         */
        bool compressToCallback(const std::string& inputFile,
            DataCallback dataCallback,
            ProgressCallback progress = nullptr);

        /**
         * @brief Compress data from memory buffer
         * @param inputData Input data buffer
         * @param inputSize Size of input data
         * @param outputData Output compressed data (will be resized)
         * @return true on success, false on failure
         */
        bool compressBuffer(const uint8_t* inputData, size_t inputSize,
            ByteVector& outputData);

        /**
         * @brief Get the maximum compressed size for a given input size
         * @param inputSize Size of uncompressed data
         * @return Maximum possible compressed size
         */
        size_t getMaxCompressedSize(size_t inputSize) const;

        /**
         * @brief Cancel the current compression operation
         */
        void cancel();

        /**
         * @brief Check if operation has been cancelled
         */
        bool isCancelled() const { return m_cancelled.load(std::memory_order_acquire); }

        /**
         * @brief Get the current compression level
         */
        int getCompressionLevel() const { return m_compressionLevel; }

        /**
         * @brief Set the compression level (must be called before compression starts)
         */
        void setCompressionLevel(int level) { m_compressionLevel = level; }

        /**
         * @brief Get the buffer size used for streaming
         */
        size_t getBufferSize() const { return m_bufferSize; }

        /**
         * @brief Get last error message
         */
        std::string getLastError() const { return m_lastError; }

        /**
         * @brief Check if compression is in progress
         */
        bool isCompressing() const { return m_compressing; }

        /**
         * @brief Get the compressed size of the last operation
         */
        uint64_t getCompressedSize() const { return m_compressedSize; }

        /**
         * @brief Get the uncompressed size of the last operation
         */
        uint64_t getUncompressedSize() const { return m_uncompressedSize; }

    private:
        bool compressStreaming(std::istream& inputStream,
            std::ostream& outputStream,
            ProgressCallback progress);

        bool compressStreamingToCallback(std::istream& inputStream,
            DataCallback dataCallback,
            ProgressCallback progress);

        void cleanup();

        ZSTD_CCtx_s* m_context;
        int m_compressionLevel;
        size_t m_bufferSize;
        std::atomic<bool> m_cancelled;
        std::atomic<bool> m_compressing;
        std::string m_lastError;
        uint64_t m_compressedSize;
        uint64_t m_uncompressedSize;
        std::unique_ptr<uint8_t[]> m_inputBuffer;
        std::unique_ptr<uint8_t[]> m_outputBuffer;
    };

} // namespace noty