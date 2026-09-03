#pragma once
#include "../common/Common.h"
#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace noty {

    /**
     * @brief Manages cryptographic keys for encryption/decryption
     *
     * Provides key derivation and secure key handling.
     * Keys are stored securely and never exposed as plain text.
     */
    class KeyManager {
    public:
        KeyManager();
        ~KeyManager();

        // Non-copyable
        KeyManager(const KeyManager&) = delete;
        KeyManager& operator=(const KeyManager&) = delete;

        // Movable
        KeyManager(KeyManager&& other) noexcept;
        KeyManager& operator=(KeyManager&& other) noexcept;

        /**
         * @brief Generate a random AES-256 key
         * @return 32-byte random key
         */
        std::vector<uint8_t> generateKey();

        /**
         * @brief Generate a random nonce for AES-GCM
         * @return 12-byte random nonce
         */
        std::vector<uint8_t> generateNonce();

        /**
         * @brief Derive a key from a passphrase using PBKDF2
         * @param passphrase User-provided passphrase
         * @param salt Salt for key derivation (16-32 bytes recommended)
         * @param iterations Number of PBKDF2 iterations (100000+ recommended)
         * @param keyLength Desired key length (32 for AES-256)
         * @return Derived key
         */
        std::vector<uint8_t> deriveKeyFromPassphrase(const std::string& passphrase,
            const std::vector<uint8_t>& salt,
            uint32_t iterations = 100000,
            size_t keyLength = 32);

        /**
         * @brief Generate a random salt
         * @param size Salt size in bytes (16-32 recommended)
         * @return Random salt
         */
        std::vector<uint8_t> generateSalt(size_t size = 16);

        /**
         * @brief Securely zero a memory buffer
         * @param buffer Buffer to zero
         */
        static void secureZero(std::vector<uint8_t>& buffer);

        /**
         * @brief Securely zero a memory buffer
         * @param buffer Pointer to buffer
         * @param size Size of buffer in bytes
         */
        static void secureZero(void* buffer, size_t size);

        /**
         * @brief Get last error message
         */
        std::string getLastError() const { return m_lastError; }

        /**
         * @brief Get recommended PBKDF2 iterations
         */
        static uint32_t getRecommendedIterations() { return 600000; }

        /**
         * @brief Get recommended salt size
         */
        static size_t getRecommendedSaltSize() { return 32; }

        /**
         * @brief Get recommended nonce size
         */
        static size_t getRecommendedNonceSize() { return 12; }

        /**
         * @brief Get recommended key size for AES-256
         */
        static size_t getRecommendedKeySize() { return 32; }

    private:
        std::string m_lastError;

        // Internal helper for random generation
        bool generateRandomBytes(uint8_t* buffer, size_t size);
    };

} // namespace noty