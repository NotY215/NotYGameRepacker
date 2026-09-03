================================================================================
FILE: src / compression / ZstdDecompressor.cpp
================================================================================
#include "noty/compression/ZstdDecompressor.h"
#include "noty/common/Logger.h"
#include <zstd.h>
#include <fstream>
#include <algorithm>
#include <cstring>

namespace noty {

    ZstdDecompressor::ZstdDecompressor(size_t bufferSize)
        : m_context(nullptr)
        , m_bufferSize(bufferSize)
        , m_cancelled(false)
        , m_decompressing(false)
        , m_compressedSize(0)
        , m_uncompressedSize(0)
    {
        m_context = ZSTD_createDCtx();
        if (!m_context) {
            m_lastError = "Failed to create ZSTD decompression context";
            Logger::instance().error(m_lastError);
        }

        m_inputBuffer = std::make_unique<uint8_t[]>(m_bufferSize);
        m_outputBuffer = std::make_unique<uint8_t[]>(m_bufferSize);
    }

    ZstdDecompressor::~ZstdDecompressor() {
        cleanup();
    }

    ZstdDecompressor::ZstdDecompressor(ZstdDecompressor&& other) noexcept
        : m_context(std::exchange(other.m_context, nullptr))
        , m_bufferSize(other.m_bufferSize)
        , m_cancelled(other.m_cancelled.load())
        , m_decompressing(other.m_decompressing.load())
        , m_lastError(std::move(other.m_lastError))
        , m_compressedSize(other.m_compressedSize)
        , m_uncompressedSize(other.m_uncompressedSize)
        , m_inputBuffer(std::move(other.m_inputBuffer))
        , m_outputBuffer(std::move(other.m_outputBuffer))
    {
    }

    ZstdDecompressor& ZstdDecompressor::operator=(ZstdDecompressor&& other) noexcept {
        if (this != &other) {
            cleanup();
            m_context = std::exchange(other.m_context, nullptr);
            m_bufferSize = other.m_bufferSize;
            m_cancelled.store(other.m_cancelled.load());
            m_decompressing.store(other.m_decompressing.load());
            m_lastError = std::move(other.m_lastError);
            m_compressedSize = other.m_compressedSize;
            m_uncompressedSize = other.m_uncompressedSize;
            m_inputBuffer = std::move(other.m_inputBuffer);
            m_outputBuffer = std::move(other.m_outputBuffer);
        }
        return *this;
    }

    bool ZstdDecompressor::decompressFile(const std::string& inputFile,
        const std::string& outputFile,
        ProgressCallback progress) {
        if (!m_context) {
            m_lastError = "Decompression context not initialized";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (m_decompressing) {
            m_lastError = "Decompression already in progress";
            Logger::instance().warning(m_lastError);
            return false;
        }

        m_cancelled = false;
        m_decompressing = true;
        m_compressedSize = 0;
        m_uncompressedSize = 0;
        m_lastError.clear();

        try {
            std::ifstream inputFileStream(inputFile, std::ios::binary);
            if (!inputFileStream.is_open()) {
                m_lastError = "Failed to open input file: " + inputFile;
                Logger::instance().error(m_lastError);
                m_decompressing = false;
                return false;
            }

            std::ofstream outputFileStream(outputFile, std::ios::binary);
            if (!outputFileStream.is_open()) {
                m_lastError = "Failed to open output file: " + outputFile;
                Logger::instance().error(m_lastError);
                m_decompressing = false;
                return false;
            }

            inputFileStream.seekg(0, std::ios::end);
            m_compressedSize = static_cast<uint64_t>(inputFileStream.tellg());
            inputFileStream.seekg(0, std::ios::beg);

            bool result = decompressStreaming(inputFileStream, outputFileStream, progress);
            m_decompressing = false;
            return result;
        }
        catch (const std::exception& e) {
            m_lastError = "Decompression exception: " + std::string(e.what());
            Logger::instance().error(m_lastError);
            m_decompressing = false;
            return false;
        }
    }

    bool ZstdDecompressor::decompressToCallback(const std::string& inputFile,
        DataCallback dataCallback,
        ProgressCallback progress) {
        if (!m_context) {
            m_lastError = "Decompression context not initialized";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (m_decompressing) {
            m_lastError = "Decompression already in progress";
            Logger::instance().warning(m_lastError);
            return false;
        }

        if (!dataCallback) {
            m_lastError = "Data callback cannot be null";
            Logger::instance().error(m_lastError);
            return false;
        }

        m_cancelled = false;
        m_decompressing = true;
        m_compressedSize = 0;
        m_uncompressedSize = 0;
        m_lastError.clear();

        try {
            std::ifstream inputFileStream(inputFile, std::ios::binary);
            if (!inputFileStream.is_open()) {
                m_lastError = "Failed to open input file: " + inputFile;
                Logger::instance().error(m_lastError);
                m_decompressing = false;
                return false;
            }

            inputFileStream.seekg(0, std::ios::end);
            m_compressedSize = static_cast<uint64_t>(inputFileStream.tellg());
            inputFileStream.seekg(0, std::ios::beg);

            bool result = decompressStreamingToCallback(inputFileStream, dataCallback, progress);
            m_decompressing = false;
            return result;
        }
        catch (const std::exception& e) {
            m_lastError = "Decompression exception: " + std::string(e.what());
            Logger::instance().error(m_lastError);
            m_decompressing = false;
            return false;
        }
    }

    bool ZstdDecompressor::decompressBuffer(const uint8_t* inputData, size_t inputSize,
        ByteVector& outputData) {
        if (!m_context) {
            m_lastError = "Decompression context not initialized";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (m_decompressing) {
            m_lastError = "Decompression already in progress";
            Logger::instance().warning(m_lastError);
            return false;
        }

        if (!inputData || inputSize == 0) {
            m_lastError = "Invalid input data";
            Logger::instance().error(m_lastError);
            return false;
        }

        m_cancelled = false;
        m_decompressing = true;
        m_compressedSize = inputSize;
        m_uncompressedSize = 0;
        m_lastError.clear();

        try {
            // Get decompressed size first
            unsigned long long decompressedSize = ZSTD_getFrameContentSize(inputData, inputSize);
            if (decompressedSize == ZSTD_CONTENTSIZE_ERROR) {
                m_lastError = "Failed to determine decompressed size";
                Logger::instance().error(m_lastError);
                m_decompressing = false;
                return false;
            }
            if (decompressedSize == ZSTD_CONTENTSIZE_UNKNOWN) {
                // Unknown size - we'll need to decompress into a dynamic buffer
                // For now, use a conservative approach with a temporary buffer
                Logger::instance().warning("Decompressed size unknown, using streaming approach");
                m_decompressing = false;
                return false;
            }

            outputData.resize(static_cast<size_t>(decompressedSize));
            m_uncompressedSize = static_cast<size_t>(decompressedSize);

            size_t result = ZSTD_decompressDCtx(
                m_context,
                outputData.data(),
                outputData.size(),
                inputData,
                inputSize
            );

            if (ZSTD_isError(result)) {
                m_lastError = "ZSTD decompression error: " + std::string(ZSTD_getErrorName(result));
                Logger::instance().error(m_lastError);
                m_decompressing = false;
                return false;
            }

            if (result != outputData.size()) {
                outputData.resize(result);
                m_uncompressedSize = result;
            }

            m_decompressing = false;
            return true;
        }
        catch (const std::exception& e) {
            m_lastError = "Decompression exception: " + std::string(e.what());
            Logger::instance().error(m_lastError);
            m_decompressing = false;
            return false;
        }
    }

    size_t ZstdDecompressor::getDecompressedSize(const uint8_t* inputData, size_t inputSize) const {
        if (!inputData || inputSize == 0) {
            return 0;
        }

        unsigned long long size = ZSTD_getFrameContentSize(inputData, inputSize);
        if (size == ZSTD_CONTENTSIZE_ERROR || size == ZSTD_CONTENTSIZE_UNKNOWN) {
            return 0;
        }

        return static_cast<size_t>(size);
    }

    void ZstdDecompressor::cancel() {
        m_cancelled.store(true, std::memory_order_release);
        Logger::instance().info("Decompression cancellation requested");
    }

    bool ZstdDecompressor::decompressStreaming(std::istream& inputStream,
        std::ostream& outputStream,
        ProgressCallback progress) {
        size_t totalRead = 0;
        size_t lastProgressUpdate = 0;
        const size_t progressUpdateInterval = m_bufferSize * 10;

        ZSTD_DCtx_reset(m_context, ZSTD_reset_session_and_parameters);

        ZSTD_inBuffer inBuffer = { m_inputBuffer.get(), 0, 0 };
        ZSTD_outBuffer outBuffer = { m_outputBuffer.get(), m_bufferSize, 0 };

        while (!inputStream.eof() && !m_cancelled) {
            // Read input data
            inputStream.read(reinterpret_cast<char*>(m_inputBuffer.get()), m_bufferSize);
            size_t bytesRead = static_cast<size_t>(inputStream.gcount());

            if (bytesRead == 0) {
                break;
            }

            inBuffer.src = m_inputBuffer.get();
            inBuffer.size = bytesRead;
            inBuffer.pos = 0;

            // Decompress the input data
            while (inBuffer.pos < inBuffer.size && !m_cancelled) {
                outBuffer.pos = 0;
                outBuffer.size = m_bufferSize;

                size_t result = ZSTD_decompressStream(
                    m_context,
                    &outBuffer,
                    &inBuffer
                );

                if (ZSTD_isError(result)) {
                    m_lastError = "ZSTD decompression stream error: " + std::string(ZSTD_getErrorName(result));
                    Logger::instance().error(m_lastError);
                    return false;
                }

                // Write decompressed data
                if (outBuffer.pos > 0) {
                    outputStream.write(reinterpret_cast<const char*>(m_outputBuffer.get()), outBuffer.pos);
                    m_uncompressedSize += outBuffer.pos;
                }

                if (result == 0) {
                    // End of frame
                    break;
                }
            }

            totalRead += bytesRead;

            // Update progress
            if (progress && (totalRead - lastProgressUpdate) >= progressUpdateInterval) {
                progress(totalRead, m_compressedSize);
                lastProgressUpdate = totalRead;
            }
        }

        if (m_cancelled) {
            m_lastError = "Decompression cancelled by user";
            Logger::instance().info(m_lastError);
            return false;
        }

        if (progress) {
            progress(m_compressedSize, m_compressedSize);
        }

        Logger::instance().info("Decompression complete: " +
            std::to_string(m_compressedSize) + " -> " +
            std::to_string(m_uncompressedSize) + " bytes");

        return true;
    }

    bool ZstdDecompressor::decompressStreamingToCallback(std::istream& inputStream,
        DataCallback dataCallback,
        ProgressCallback progress) {
        size_t totalRead = 0;
        size_t lastProgressUpdate = 0;
        const size_t progressUpdateInterval = m_bufferSize * 10;

        ZSTD_DCtx_reset(m_context, ZSTD_reset_session_and_parameters);

        ZSTD_inBuffer inBuffer = { m_inputBuffer.get(), 0, 0 };
        ZSTD_outBuffer outBuffer = { m_outputBuffer.get(), m_bufferSize, 0 };

        while (!inputStream.eof() && !m_cancelled) {
            inputStream.read(reinterpret_cast<char*>(m_inputBuffer.get()), m_bufferSize);
            size_t bytesRead = static_cast<size_t>(inputStream.gcount());

            if (bytesRead == 0) {
                break;
            }

            inBuffer.src = m_inputBuffer.get();
            inBuffer.size = bytesRead;
            inBuffer.pos = 0;

            while (inBuffer.pos < inBuffer.size && !m_cancelled) {
                outBuffer.pos = 0;
                outBuffer.size = m_bufferSize;

                size_t result = ZSTD_decompressStream(
                    m_context,
                    &outBuffer,
                    &inBuffer
                );

                if (ZSTD_isError(result)) {
                    m_lastError = "ZSTD decompression stream error: " + std::string(ZSTD_getErrorName(result));
                    Logger::instance().error(m_lastError);
                    return false;
                }

                if (outBuffer.pos > 0) {
                    dataCallback(m_outputBuffer.get(), outBuffer.pos);
                    m_uncompressedSize += outBuffer.pos;
                }

                if (result == 0) {
                    break;
                }
            }

            totalRead += bytesRead;

            if (progress && (totalRead - lastProgressUpdate) >= progressUpdateInterval) {
                progress(totalRead, m_compressedSize);
                lastProgressUpdate = totalRead;
            }
        }

        if (m_cancelled) {
            m_lastError = "Decompression cancelled by user";
            Logger::instance().info(m_lastError);
            return false;
        }

        if (progress) {
            progress(m_compressedSize, m_compressedSize);
        }

        return true;
    }

    void ZstdDecompressor::cleanup() {
        if (m_context) {
            ZSTD_freeDCtx(m_context);
            m_context = nullptr;
        }
        m_inputBuffer.reset();
        m_outputBuffer.reset();
    }

} // namespace noty