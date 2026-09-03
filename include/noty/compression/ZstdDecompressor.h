================================================================================
FILE: include / noty / compression / ZstdDecompressor.h
================================================================================
#pragma once
#include "../common/Common.h"
#include <string>
#include <functional>
#include <atomic>
#include <memory>
#include <cstdint>

struct ZSTD_DCtx_s;

namespace noty {

    /**
     * @brief Streaming Zstandard decompressor with bounded buffers
     *
     * This class provides memory-efficient decompression for large files
     * using a streaming approach with configurable buffer sizes.
     * Supports progress reporting and cancellation.
     */
    class ZstdDecompressor {
    public:
        using ProgressCallback = std::function<void(uint64_t bytesProcessed, uint64_t totalBytes)>;
        using DataCallback = std::function<void(const uint8_t* data, size_t size)>;

        /**
         * @brief Constructor
         * @param bufferSize Internal buffer size for streaming (default 1MB)
         */
        explicit ZstdDecompressor(size_t bufferSize = 1024 * 1024);
        ~ZstdDecompressor();

        // Non-copyable
        ZstdDecompressor(const ZstdDecompressor&) = delete;
        ZstdDecompressor& operator=(const ZstdDecompressor&) = delete;

        // Movable
        ZstdDecompressor(ZstdDecompressor&& other) noexcept;
        ZstdDecompressor& operator=(ZstdDecompressor&& other) noexcept;

        /**
         * @brief Decompress a file to another file with streaming
         * @param inputFile Path to input compressed file
         * @param outputFile Path to output decompressed file
         * @param progress Optional progress callback
         * @return true on success, false on failure
         */
        bool decompressFile(const std::string& inputFile,
            const std::string& outputFile,
            ProgressCallback progress = nullptr);

        /**
         * @brief Decompress with streaming from callback
         * @param inputFile Path to input compressed file
         * @param dataCallback Called for each decompressed chunk
         * @param progress Optional progress callback
         * @return true on success, false on failure
         */
        bool decompressToCallback(const std::string& inputFile,
            DataCallback dataCallback,
            ProgressCallback progress = nullptr);

        /**
         * @brief Decompress from memory buffer
         * @param inputData Input compressed data buffer
         * @param inputSize Size of input data
         * @param outputData Output decompressed data (will be resized)
         * @return true on success, false on failure
         */
        bool decompressBuffer(const uint8_t* inputData, size_t inputSize,
            ByteVector& outputData);

        /**
         * @brief Decompress a frame and get the decompressed size
         * @param inputData Input compressed data
         * @param inputSize Size of input data
         * @return Decompressed size, or 0 on error
         */
        size_t getDecompressedSize(const uint8_t* inputData, size_t inputSize) const;

        /**
         * @brief Cancel the current decompression operation
         */
        void cancel();

        /**
         * @brief Check if operation has been cancelled
         */
        bool isCancelled() const { return m_cancelled.load(std::memory_order_acquire); }

        /**
         * @brief Get the buffer size used for streaming
         */
        size_t getBufferSize() const { return m_bufferSize; }

        /**
         * @brief Get last error message
         */
        std::string getLastError() const { return m_lastError; }

        /**
         * @brief Check if decompression is in progress
         */
        bool isDecompressing() const { return m_decompressing; }

        /**
         * @brief Get the uncompressed size of the last operation
         */
        uint64_t getUncompressedSize() const { return m_uncompressedSize; }

        /**
         * @brief Get the compressed size of the last operation
         */
        uint64_t getCompressedSize() const { return m_compressedSize; }

    private:
        bool decompressStreaming(std::istream& inputStream,
            std::ostream& outputStream,
            ProgressCallback progress);

        bool decompressStreamingToCallback(std::istream& inputStream,
            DataCallback dataCallback,
            ProgressCallback progress);

        void cleanup();

        ZSTD_DCtx_s* m_context;
        size_t m_bufferSize;
        std::atomic<bool> m_cancelled;
        std::atomic<bool> m_decompressing;
        std::string m_lastError;
        uint64_t m_compressedSize;
        uint64_t m_uncompressedSize;
        std::unique_ptr<uint8_t[]> m_inputBuffer;
        std::unique_ptr<uint8_t[]> m_outputBuffer;
    };

} // namespace noty