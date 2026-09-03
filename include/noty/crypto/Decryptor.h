================================================================================
FILE: include / noty / crypto / Decryptor.h
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
     * @brief Streaming AES-256-GCM decryptor using Windows CNG
     *
     * Provides authenticated decryption with tamper detection.
     * Supports streaming decryption with bounded buffers.
     */
    class Decryptor {
    public:
        using ProgressCallback = std::function<void(uint64_t bytesProcessed, uint64_t totalBytes)>;
        using DataCallback = std::function<void(const uint8_t* data, size_t size)>;

        /**
         * @brief Constructor
         * @param bufferSize Internal buffer size for streaming (default 1MB)
         */
        explicit Decryptor(size_t bufferSize = 1024 * 1024);
        ~Decryptor();

        // Non-copyable
        Decryptor(const Decryptor&) = delete;
        Decryptor& operator=(const Decryptor&) = delete;

        // Movable
        Decryptor(Decryptor&& other) noexcept;
        Decryptor& operator=(Decryptor&& other) noexcept;

        /**
         * @brief Initialize the decryptor with a key
         * @param key 32-byte AES-256 key
         * @param nonce 12-byte nonce (must match encryption nonce)
         * @param authTag 16-byte authentication tag
         * @param additionalData Optional authenticated data (must match encryption)
         * @return true on success
         */
        bool initialize(const std::vector<uint8_t>& key,
            const std::vector<uint8_t>& nonce,
            const std::vector<uint8_t>& authTag,
            const std::vector<uint8_t>& additionalData = {});

        /**
         * @brief Decrypt a file to another file with streaming
         * @param inputFile Path to input encrypted file
         * @param outputFile Path to output decrypted file
         * @param progress Optional progress callback
         * @return true on success
         */
        bool decryptFile(const std::string& inputFile,
            const std::string& outputFile,
            ProgressCallback progress = nullptr);

        /**
         * @brief Decrypt with streaming to callback
         * @param inputFile Path to input encrypted file
         * @param dataCallback Called for each decrypted chunk
         * @param progress Optional progress callback
         * @return true on success
         */
        bool decryptToCallback(const std::string& inputFile,
            DataCallback dataCallback,
            ProgressCallback progress = nullptr);

        /**
         * @brief Decrypt data from memory buffer
         * @param inputData Input encrypted data buffer
         * @param inputSize Size of input data
         * @param outputData Output decrypted data (will be resized)
         * @param authTag Authentication tag for verification
         * @return true on success
         */
        bool decryptBuffer(const uint8_t* inputData, size_t inputSize,
            ByteVector& outputData, const std::vector<uint8_t>& authTag);

        /**
         * @brief Cancel the current decryption operation
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
         * @brief Check if decryption is in progress
         */
        bool isDecrypting() const { return m_decrypting; }

        /**
         * @brief Get the encrypted size of the last operation
         */
        uint64_t getEncryptedSize() const { return m_encryptedSize; }

        /**
         * @brief Get the plaintext size of the last operation
         */
        uint64_t getPlaintextSize() const { return m_plaintextSize; }

        /**
         * @brief Check if authentication was successful
         */
        bool isAuthenticationValid() const { return m_authValid; }

    private:
        bool decryptStreaming(std::istream& inputStream,
            std::ostream& outputStream,
            ProgressCallback progress);

        bool decryptStreamingToCallback(std::istream& inputStream,
            DataCallback dataCallback,
            ProgressCallback progress);

        void cleanup();

        BCRYPT_ALG_HANDLE__* m_algorithmHandle;
        BCRYPT_KEY_HANDLE__* m_keyHandle;
        size_t m_bufferSize;
        std::atomic<bool> m_cancelled;
        std::atomic<bool> m_decrypting;
        std::string m_lastError;
        uint64_t m_encryptedSize;
        uint64_t m_plaintextSize;
        std::vector<uint8_t> m_nonce;
        std::vector<uint8_t> m_authTag;
        std::vector<uint8_t> m_additionalData;
        std::unique_ptr<uint8_t[]> m_inputBuffer;
        std::unique_ptr<uint8_t[]> m_outputBuffer;
        bool m_initialized;
        bool m_authValid;
    };

} // namespace noty