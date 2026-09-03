================================================================================
FILE: include / noty / crypto / Encryptor.h
================================================================================
#pragma once
#include "../common/Common.h"
#include <string>
#include <functional>
#include <atomic>
#include <memory>
#include <cstdint>
#include <vector>

struct BCRYPT_ALG_HANDLE__;
struct BCRYPT_KEY_HANDLE__;

namespace noty {

    /**
     * @brief Streaming AES-256-GCM encryptor using Windows CNG
     *
     * Provides authenticated encryption with tamper detection.
     * Supports streaming encryption with bounded buffers.
     */
    class Encryptor {
    public:
        using ProgressCallback = std::function<void(uint64_t bytesProcessed, uint64_t totalBytes)>;
        using DataCallback = std::function<void(const uint8_t* data, size_t size)>;

        /**
         * @brief Constructor
         * @param bufferSize Internal buffer size for streaming (default 1MB)
         */
        explicit Encryptor(size_t bufferSize = 1024 * 1024);
        ~Encryptor();

        // Non-copyable
        Encryptor(const Encryptor&) = delete;
        Encryptor& operator=(const Encryptor&) = delete;

        // Movable
        Encryptor(Encryptor&& other) noexcept;
        Encryptor& operator=(Encryptor&& other) noexcept;

        /**
         * @brief Initialize the encryptor with a key
         * @param key 32-byte AES-256 key
         * @param nonce 12-byte nonce (should be unique per encryption)
         * @param additionalData Optional authenticated data (not encrypted)
         * @return true on success
         */
        bool initialize(const std::vector<uint8_t>& key,
            const std::vector<uint8_t>& nonce,
            const std::vector<uint8_t>& additionalData = {});

        /**
         * @brief Encrypt a file to another file with streaming
         * @param inputFile Path to input file
         * @param outputFile Path to output encrypted file
         * @param progress Optional progress callback
         * @return true on success
         */
        bool encryptFile(const std::string& inputFile,
            const std::string& outputFile,
            ProgressCallback progress = nullptr);

        /**
         * @brief Encrypt with streaming to callback
         * @param inputFile Path to input file
         * @param dataCallback Called for each encrypted chunk
         * @param progress Optional progress callback
         * @return true on success
         */
        bool encryptToCallback(const std::string& inputFile,
            DataCallback dataCallback,
            ProgressCallback progress = nullptr);

        /**
         * @brief Encrypt data from memory buffer
         * @param inputData Input data buffer
         * @param inputSize Size of input data
         * @param outputData Output encrypted data (will be resized)
         * @param authTag Output authentication tag (16 bytes)
         * @return true on success
         */
        bool encryptBuffer(const uint8_t* inputData, size_t inputSize,
            ByteVector& outputData, ByteVector& authTag);

        /**
         * @brief Get the authentication tag from the last operation
         * @return Authentication tag (16 bytes), empty if not available
         */
        std::vector<uint8_t> getAuthenticationTag() const;

        /**
         * @brief Get the nonce used
         * @return Nonce (12 bytes)
         */
        std::vector<uint8_t> getNonce() const { return m_nonce; }

        /**
         * @brief Cancel the current encryption operation
         */
        void cancel();

        /**
         * @brief Check if operation has been cancelled
         */
        bool isCancelled() const { return m_cancelled.load(std::memory_order_acquire); }

        /**
         * @brief Get last error message
         */
        std::string getLastError() const { return m_lastError; }

        /**
         * @brief Check if encryption is in progress
         */
        bool isEncrypting() const { return m_encrypting; }

        /**
         * @brief Get the encrypted size of the last operation
         */
        uint64_t getEncryptedSize() const { return m_encryptedSize; }

        /**
         * @brief Get the plaintext size of the last operation
         */
        uint64_t getPlaintextSize() const { return m_plaintextSize; }

        /**
         * @brief Get the recommended nonce size
         */
        static size_t getNonceSize() { return 12; }

        /**
         * @brief Get the recommended key size
         */
        static size_t getKeySize() { return 32; }

        /**
         * @brief Get the authentication tag size
         */
        static size_t getAuthTagSize() { return 16; }

    private:
        bool encryptStreaming(std::istream& inputStream,
            std::ostream& outputStream,
            ProgressCallback progress);

        bool encryptStreamingToCallback(std::istream& inputStream,
            DataCallback dataCallback,
            ProgressCallback progress);

        void cleanup();

        BCRYPT_ALG_HANDLE__* m_algorithmHandle;
        BCRYPT_KEY_HANDLE__* m_keyHandle;
        size_t m_bufferSize;
        std::atomic<bool> m_cancelled;
        std::atomic<bool> m_encrypting;
        std::string m_lastError;
        uint64_t m_encryptedSize;
        uint64_t m_plaintextSize;
        std::vector<uint8_t> m_nonce;
        std::vector<uint8_t> m_authTag;
        std::vector<uint8_t> m_additionalData;
        std::unique_ptr<uint8_t[]> m_inputBuffer;
        std::unique_ptr<uint8_t[]> m_outputBuffer;
        bool m_initialized;
    };

} // namespace noty