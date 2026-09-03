================================================================================
FILE: src / compression / ZstdCompressor.cpp
================================================================================
#include "noty/compression/ZstdCompressor.h"
#include "noty/common/Logger.h"
#include <zstd.h>
#include <fstream>
#include <algorithm>
#include <cstring>

namespace noty {

    ZstdCompressor::ZstdCompressor(int compressionLevel, size_t bufferSize)
        : m_context(nullptr)
        , m_compressionLevel(std::clamp(compressionLevel, 1, 22))
        , m_bufferSize(bufferSize)
        , m_cancelled(false)
        , m_compressing(false)
        , m_compressedSize(0)
        , m_uncompressedSize(0)
    {
        m_context = ZSTD_createCCtx();
        if (!m_context) {
            m_lastError = "Failed to create ZSTD compression context";
            Logger::instance().error(m_lastError);
        }

        // Allocate buffers
        m_inputBuffer = std::make_unique<uint8_t[]>(m_bufferSize);
        m_outputBuffer = std::make_unique<uint8_t[]>(ZSTD_compressBound(m_bufferSize));
    }

    ZstdCompressor::~ZstdCompressor() {
        cleanup();
    }

    ZstdCompressor::ZstdCompressor(ZstdCompressor&& other) noexcept
        : m_context(std::exchange(other.m_context, nullptr))
        , m_compressionLevel(other.m_compressionLevel)
        , m_bufferSize(other.m_bufferSize)
        , m_cancelled(other.m_cancelled.load())
        , m_compressing(other.m_compressing.load())
        , m_lastError(std::move(other.m_lastError))
        , m_compressedSize(other.m_compressedSize)
        , m_uncompressedSize(other.m_uncompressedSize)
        , m_inputBuffer(std::move(other.m_inputBuffer))
        , m_outputBuffer(std::move(other.m_outputBuffer))
    {
    }

    ZstdCompressor& ZstdCompressor::operator=(ZstdCompressor&& other) noexcept {
        if (this != &other) {
            cleanup();
            m_context = std::exchange(other.m_context, nullptr);
            m_compressionLevel = other.m_compressionLevel;
            m_bufferSize = other.m_bufferSize;
            m_cancelled.store(other.m_cancelled.load());
            m_compressing.store(other.m_compressing.load());
            m_lastError = std::move(other.m_lastError);
            m_compressedSize = other.m_compressedSize;
            m_uncompressedSize = other.m_uncompressedSize;
            m_inputBuffer = std::move(other.m_inputBuffer);
            m_outputBuffer = std::move(other.m_outputBuffer);
        }
        return *this;
    }

    bool ZstdCompressor::compressFile(const std::string& inputFile,
        const std::string& outputFile,
        ProgressCallback progress) {
        if (!m_context) {
            m_lastError = "Compression context not initialized";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (m_compressing) {
            m_lastError = "Compression already in progress";
            Logger::instance().warning(m_lastError);
            return false;
        }

        m_cancelled = false;
        m_compressing = true;
        m_compressedSize = 0;
        m_uncompressedSize = 0;
        m_lastError.clear();

        try {
            std::ifstream inputFileStream(inputFile, std::ios::binary);
            if (!inputFileStream.is_open()) {
                m_lastError = "Failed to open input file: " + inputFile;
                Logger::instance().error(m_lastError);
                m_compressing = false;
                return false;
            }

            std::ofstream outputFileStream(outputFile, std::ios::binary);
            if (!outputFileStream.is_open()) {
                m_lastError = "Failed to open output file: " + outputFile;
                Logger::instance().error(m_lastError);
                m_compressing = false;
                return false;
            }

            // Get total size for progress reporting
            inputFileStream.seekg(0, std::ios::end);
            m_uncompressedSize = static_cast<uint64_t>(inputFileStream.tellg());
            inputFileStream.seekg(0, std::ios::beg);

            bool result = compressStreaming(inputFileStream, outputFileStream, progress);
            m_compressing = false;
            return result;
        }
        catch (const std::exception& e) {
            m_lastError = "Compression exception: " + std::string(e.what());
            Logger::instance().error(m_lastError);
            m_compressing = false;
            return false;
        }
    }

    bool ZstdCompressor::compressToCallback(const std::string& inputFile,
        DataCallback dataCallback,
        ProgressCallback progress) {
        if (!m_context) {
            m_lastError = "Compression context not initialized";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (m_compressing) {
            m_lastError = "Compression already in progress";
            Logger::instance().warning(m_lastError);
            return false;
        }

        if (!dataCallback) {
            m_lastError = "Data callback cannot be null";
            Logger::instance().error(m_lastError);
            return false;
        }

        m_cancelled = false;
        m_compressing = true;
        m_compressedSize = 0;
        m_uncompressedSize = 0;
        m_lastError.clear();

        try {
            std::ifstream inputFileStream(inputFile, std::ios::binary);
            if (!inputFileStream.is_open()) {
                m_lastError = "Failed to open input file: " + inputFile;
                Logger::instance().error(m_lastError);
                m_compressing = false;
                return false;
            }

            inputFileStream.seekg(0, std::ios::end);
            m_uncompressedSize = static_cast<uint64_t>(inputFileStream.tellg());
            inputFileStream.seekg(0, std::ios::beg);

            bool result = compressStreamingToCallback(inputFileStream, dataCallback, progress);
            m_compressing = false;
            return result;
        }
        catch (const std::exception& e) {
            m_lastError = "Compression exception: " + std::string(e.what());
            Logger::instance().error(m_lastError);
            m_compressing = false;
            return false;
        }
    }

    bool ZstdCompressor::compressBuffer(const uint8_t* inputData, size_t inputSize,
        ByteVector& outputData) {
        if (!m_context) {
            m_lastError = "Compression context not initialized";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (m_compressing) {
            m_lastError = "Compression already in progress";
            Logger::instance().warning(m_lastError);
            return false;
        }

        if (!inputData || inputSize == 0) {
            m_lastError = "Invalid input data";
            Logger::instance().error(m_lastError);
            return false;
        }

        m_cancelled = false;
        m_compressing = true;
        m_compressedSize = 0;
        m_uncompressedSize = inputSize;
        m_lastError.clear();

        try {
            size_t maxCompressedSize = ZSTD_compressBound(inputSize);
            outputData.resize(maxCompressedSize);

            size_t compressedSize = ZSTD_compressCCtx(
                m_context,
                outputData.data(),
                maxCompressedSize,
                inputData,
                inputSize,
                m_compressionLevel
            );

            if (ZSTD_isError(compressedSize)) {
                m_lastError = "ZSTD compression error: " + std::string(ZSTD_getErrorName(compressedSize));
                Logger::instance().error(m_lastError);
                m_compressing = false;
                return false;
            }

            outputData.resize(compressedSize);
            m_compressedSize = compressedSize;
            m_compressing = false;
            return true;
        }
        catch (const std::exception& e) {
            m_lastError = "Compression exception: " + std::string(e.what());
            Logger::instance().error(m_lastError);
            m_compressing = false;
            return false;
        }
    }

    size_t ZstdCompressor::getMaxCompressedSize(size_t inputSize) const {
        return ZSTD_compressBound(inputSize);
    }

    void ZstdCompressor::cancel() {
        m_cancelled.store(true, std::memory_order_release);
        Logger::instance().info("Compression cancellation requested");
    }

    bool ZstdCompressor::compressStreaming(std::istream& inputStream,
        std::ostream& outputStream,
        ProgressCallback progress) {
        size_t totalRead = 0;
        size_t lastProgressUpdate = 0;
        const size_t progressUpdateInterval = m_bufferSize * 10;

        // Reset compression context
        ZSTD_CCtx_reset(m_context, ZSTD_reset_session_and_parameters);

        if (ZSTD_CCtx_setParameter(m_context, ZSTD_c_compressionLevel, m_compressionLevel) != 0) {
            m_lastError = "Failed to set compression level";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (ZSTD_CCtx_setParameter(m_context, ZSTD_c_checksumFlag, 1) != 0) {
            m_lastError = "Failed to enable checksum";
            Logger::instance().error(m_lastError);
            return false;
        }

        // Streaming compression
        ZSTD_inBuffer inBuffer = { m_inputBuffer.get(), 0, 0 };
        ZSTD_outBuffer outBuffer = { m_outputBuffer.get(), ZSTD_compressBound(m_bufferSize), 0 };

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

            // Compress the input data
            while (inBuffer.pos < inBuffer.size && !m_cancelled) {
                outBuffer.pos = 0;
                outBuffer.size = ZSTD_compressBound(m_bufferSize);

                size_t result = ZSTD_compressStream2(
                    m_context,
                    &outBuffer,
                    &inBuffer,
                    ZSTD_e_continue
                );

                if (ZSTD_isError(result)) {
                    m_lastError = "ZSTD compression stream error: " + std::string(ZSTD_getErrorName(result));
                    Logger::instance().error(m_lastError);
                    return false;
                }

                // Write compressed data
                if (outBuffer.pos > 0) {
                    outputStream.write(reinterpret_cast<const char*>(m_outputBuffer.get()), outBuffer.pos);
                    m_compressedSize += outBuffer.pos;
                }
            }

            totalRead += bytesRead;

            // Update progress
            if (progress && (totalRead - lastProgressUpdate) >= progressUpdateInterval) {
                progress(totalRead, m_uncompressedSize);
                lastProgressUpdate = totalRead;
            }
        }

        // Flush remaining compressed data
        if (!m_cancelled) {
            ZSTD_inBuffer emptyIn = { nullptr, 0, 0 };
            ZSTD_outBuffer outBuffer = { m_outputBuffer.get(), ZSTD_compressBound(m_bufferSize), 0 };

            while (true) {
                outBuffer.pos = 0;
                outBuffer.size = ZSTD_compressBound(m_bufferSize);

                size_t result = ZSTD_compressStream2(
                    m_context,
                    &outBuffer,
                    &emptyIn,
                    ZSTD_e_end
                );

                if (ZSTD_isError(result)) {
                    m_lastError = "ZSTD flush error: " + std::string(ZSTD_getErrorName(result));
                    Logger::instance().error(m_lastError);
                    return false;
                }

                if (outBuffer.pos > 0) {
                    outputStream.write(reinterpret_cast<const char*>(m_outputBuffer.get()), outBuffer.pos);
                    m_compressedSize += outBuffer.pos;
                }

                if (result == 0) {
                    break;
                }
            }

            if (progress) {
                progress(m_uncompressedSize, m_uncompressedSize);
            }
        }

        if (m_cancelled) {
            m_lastError = "Compression cancelled by user";
            Logger::instance().info(m_lastError);
            return false;
        }

        Logger::instance().info("Compression complete: " +
            std::to_string(m_uncompressedSize) + " -> " +
            std::to_string(m_compressedSize) + " bytes (" +
            std::to_string(m_compressedSize * 100 / (m_uncompressedSize > 0 ? m_uncompressedSize : 1)) +
            "%)");

        return true;
    }

    bool ZstdCompressor::compressStreamingToCallback(std::istream& inputStream,
        DataCallback dataCallback,
        ProgressCallback progress) {
        size_t totalRead = 0;
        size_t lastProgressUpdate = 0;
        const size_t progressUpdateInterval = m_bufferSize * 10;

        ZSTD_CCtx_reset(m_context, ZSTD_reset_session_and_parameters);

        if (ZSTD_CCtx_setParameter(m_context, ZSTD_c_compressionLevel, m_compressionLevel) != 0) {
            m_lastError = "Failed to set compression level";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (ZSTD_CCtx_setParameter(m_context, ZSTD_c_checksumFlag, 1) != 0) {
            m_lastError = "Failed to enable checksum";
            Logger::instance().error(m_lastError);
            return false;
        }

        ZSTD_inBuffer inBuffer = { m_inputBuffer.get(), 0, 0 };
        ZSTD_outBuffer outBuffer = { m_outputBuffer.get(), ZSTD_compressBound(m_bufferSize), 0 };

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
                outBuffer.size = ZSTD_compressBound(m_bufferSize);

                size_t result = ZSTD_compressStream2(
                    m_context,
                    &outBuffer,
                    &inBuffer,
                    ZSTD_e_continue
                );

                if (ZSTD_isError(result)) {
                    m_lastError = "ZSTD compression stream error: " + std::string(ZSTD_getErrorName(result));
                    Logger::instance().error(m_lastError);
                    return false;
                }

                if (outBuffer.pos > 0) {
                    dataCallback(m_outputBuffer.get(), outBuffer.pos);
                    m_compressedSize += outBuffer.pos;
                }
            }

            totalRead += bytesRead;

            if (progress && (totalRead - lastProgressUpdate) >= progressUpdateInterval) {
                progress(totalRead, m_uncompressedSize);
                lastProgressUpdate = totalRead;
            }
        }

        if (!m_cancelled) {
            ZSTD_inBuffer emptyIn = { nullptr, 0, 0 };
            ZSTD_outBuffer outBuffer = { m_outputBuffer.get(), ZSTD_compressBound(m_bufferSize), 0 };

            while (true) {
                outBuffer.pos = 0;
                outBuffer.size = ZSTD_compressBound(m_bufferSize);

                size_t result = ZSTD_compressStream2(
                    m_context,
                    &outBuffer,
                    &emptyIn,
                    ZSTD_e_end
                );

                if (ZSTD_isError(result)) {
                    m_lastError = "ZSTD flush error: " + std::string(ZSTD_getErrorName(result));
                    Logger::instance().error(m_lastError);
                    return false;
                }

                if (outBuffer.pos > 0) {
                    dataCallback(m_outputBuffer.get(), outBuffer.pos);
                    m_compressedSize += outBuffer.pos;
                }

                if (result == 0) {
                    break;
                }
            }

            if (progress) {
                progress(m_uncompressedSize, m_uncompressedSize);
            }
        }

        if (m_cancelled) {
            m_lastError = "Compression cancelled by user";
            Logger::instance().info(m_lastError);
            return false;
        }

        return true;
    }

    void ZstdCompressor::cleanup() {
        if (m_context) {
            ZSTD_freeCCtx(m_context);
            m_context = nullptr;
        }
        m_inputBuffer.reset();
        m_outputBuffer.reset();
    }

} // namespace noty