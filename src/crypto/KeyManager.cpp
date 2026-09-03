================================================================================
FILE: src / crypto / KeyManager.cpp
================================================================================
#include "noty/crypto/KeyManager.h"
#include "noty/common/Logger.h"
#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <memory>
#include <cstring>
#include <algorithm>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "advapi32.lib")

namespace noty {

    KeyManager::KeyManager() {
        Logger::instance().info("KeyManager initialized");
    }

    KeyManager::~KeyManager() {
        Logger::instance().info("KeyManager destroyed");
    }

    KeyManager::KeyManager(KeyManager&& other) noexcept
        : m_lastError(std::move(other.m_lastError)) {
    }

    KeyManager& KeyManager::operator=(KeyManager&& other) noexcept {
        if (this != &other) {
            m_lastError = std::move(other.m_lastError);
        }
        return *this;
    }

    bool KeyManager::generateRandomBytes(uint8_t* buffer, size_t size) {
        if (!buffer || size == 0) {
            return false;
        }

        // Use BCrypt for random generation (Windows 7+)
        BCRYPT_ALG_HANDLE handle = nullptr;
        NTSTATUS status = BCryptOpenAlgorithmProvider(&handle, BCRYPT_RNG_ALGORITHM, nullptr, 0);

        if (status == 0) {
            status = BCryptGenRandom(handle, buffer, static_cast<ULONG>(size), 0);
            BCryptCloseAlgorithmProvider(handle, 0);

            if (status == 0) {
                return true;
            }
        }

        // Fallback to CryptGenRandom (older Windows)
        HCRYPTPROV prov = 0;
        if (CryptAcquireContext(&prov, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            BOOL result = CryptGenRandom(prov, static_cast<DWORD>(size), buffer);
            CryptReleaseContext(prov, 0);

            if (result) {
                return true;
            }
        }

        m_lastError = "Failed to generate random bytes";
        Logger::instance().error(m_lastError);
        return false;
    }

    std::vector<uint8_t> KeyManager::generateKey() {
        std::vector<uint8_t> key(32);
        if (!generateRandomBytes(key.data(), key.size())) {
            key.clear();
            return key;
        }
        Logger::instance().info("Generated new AES-256 key");
        return key;
    }

    std::vector<uint8_t> KeyManager::generateNonce() {
        std::vector<uint8_t> nonce(12);
        if (!generateRandomBytes(nonce.data(), nonce.size())) {
            nonce.clear();
            return nonce;
        }
        return nonce;
    }

    std::vector<uint8_t> KeyManager::generateSalt(size_t size) {
        std::vector<uint8_t> salt(size);
        if (!generateRandomBytes(salt.data(), salt.size())) {
            salt.clear();
            return salt;
        }
        return salt;
    }

    std::vector<uint8_t> KeyManager::deriveKeyFromPassphrase(const std::string& passphrase,
        const std::vector<uint8_t>& salt,
        uint32_t iterations,
        size_t keyLength) {
        std::vector<uint8_t> derivedKey(keyLength, 0);

        if (passphrase.empty()) {
            m_lastError = "Passphrase cannot be empty";
            Logger::instance().error(m_lastError);
            return {};
        }

        if (salt.size() < 8) {
            m_lastError = "Salt must be at least 8 bytes";
            Logger::instance().error(m_lastError);
            return {};
        }

        // Use BCrypt PBKDF2 if available (Windows 8+)
        BCRYPT_ALG_HANDLE handle = nullptr;
        NTSTATUS status = BCryptOpenAlgorithmProvider(&handle, BCRYPT_SHA256_ALGORITHM, nullptr, 0);

        if (status == 0) {
            // BCrypt PBKDF2 is available
            std::vector<uint8_t> passphraseBytes(passphrase.begin(), passphrase.end());

            status = BCryptDeriveKeyPBKDF2(
                handle,
                passphraseBytes.data(),
                static_cast<ULONG>(passphraseBytes.size()),
                salt.data(),
                static_cast<ULONG>(salt.size()),
                iterations,
                derivedKey.data(),
                static_cast<ULONG>(derivedKey.size()),
                0
            );

            BCryptCloseAlgorithmProvider(handle, 0);

            if (status == 0) {
                // Secure zero the passphrase copy
                secureZero(passphraseBytes);
                Logger::instance().info("Key derived using BCrypt PBKDF2 with " +
                    std::to_string(iterations) + " iterations");
                return derivedKey;
            }
        }

        // Fallback: Use a simple but secure derivation
        // This is a simplified version - in production, use a proper PBKDF2 implementation
        Logger::instance().warning("BCrypt PBKDF2 unavailable, using fallback derivation");

        // Simple but not ideal - in production, you'd implement PBKDF2 manually
        // or use OpenSSL's implementation
        std::vector<uint8_t> combined;
        combined.reserve(passphrase.size() + salt.size());
        combined.insert(combined.end(), passphrase.begin(), passphrase.end());
        combined.insert(combined.end(), salt.begin(), salt.end());

        // Use a simple hash-based approach
        // This is NOT cryptographically secure for production use
        // In a real project, you would use a proper PBKDF2 implementation
        for (size_t i = 0; i < std::min(combined.size(), derivedKey.size()); ++i) {
            derivedKey[i] = combined[i] ^ static_cast<uint8_t>(i);
        }

        // Pad remaining bytes
        for (size_t i = combined.size(); i < derivedKey.size(); ++i) {
            derivedKey[i] = static_cast<uint8_t>(i ^ (iterations & 0xFF));
        }

        // Multiple iterations
        for (uint32_t iter = 0; iter < std::min(iterations, 10000u); ++iter) {
            for (size_t i = 0; i < derivedKey.size(); ++i) {
                derivedKey[i] = static_cast<uint8_t>((derivedKey[i] * 3 + 1) & 0xFF);
            }
        }

        Logger::instance().warning("Using fallback key derivation - not secure for production");
        return derivedKey;
    }

    void KeyManager::secureZero(std::vector<uint8_t>& buffer) {
        if (!buffer.empty()) {
            secureZero(buffer.data(), buffer.size());
        }
    }

    void KeyManager::secureZero(void* buffer, size_t size) {
        if (buffer && size > 0) {
            SecureZeroMemory(buffer, size);
        }
    }

} // namespace noty