#include "noty/crypto/Decryptor.h"
#include "noty/common/Logger.h"
#include <windows.h>
#include <bcrypt.h>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

namespace noty {

    static void initAuthInfo(BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO* info) {
        memset(info, 0, sizeof(BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO));
        info->cbSize = sizeof(BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO);
        info->dwInfoVersion = BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO_VERSION;
    }

    Decryptor::Decryptor(size_t bufferSize)
        : m_algorithmHandle(nullptr)
        , m_keyHandle(nullptr)
        , m_bufferSize(bufferSize)
        , m_cancelled(false)
        , m_decrypting(false)
        , m_encryptedSize(0)
        , m_plaintextSize(0)
        , m_initialized(false)
        , m_authValid(false)
    {
        NTSTATUS status = BCryptOpenAlgorithmProvider(
            &m_algorithmHandle,
            BCRYPT_AES_ALGORITHM,
            nullptr,
            0
        );

        if (status != 0) {
            m_lastError = "Failed to open AES algorithm provider";
            Logger::instance().error(m_lastError);
            return;
        }

        status = BCryptSetProperty(
            m_algorithmHandle,
            BCRYPT_CHAINING_MODE,
            (PUCHAR)BCRYPT_CHAIN_MODE_GCM,
            sizeof(BCRYPT_CHAIN_MODE_GCM),
            0
        );

        if (status != 0) {
            m_lastError = "Failed to set GCM chaining mode";
            Logger::instance().error(m_lastError);
            BCryptCloseAlgorithmProvider(m_algorithmHandle, 0);
            m_algorithmHandle = nullptr;
            return;
        }

        m_inputBuffer = std::make_unique<uint8_t[]>(m_bufferSize + 16);
        m_outputBuffer = std::make_unique<uint8_t[]>(m_bufferSize);
    }

    Decryptor::~Decryptor() {
        cleanup();
    }

    Decryptor::Decryptor(Decryptor&& other) noexcept
        : m_algorithmHandle(std::exchange(other.m_algorithmHandle, nullptr))
        , m_keyHandle(std::exchange(other.m_keyHandle, nullptr))
        , m_bufferSize(other.m_bufferSize)
        , m_cancelled(other.m_cancelled.load())
        , m_decrypting(other.m_decrypting.load())
        , m_lastError(std::move(other.m_lastError))
        , m_encryptedSize(other.m_encryptedSize)
        , m_plaintextSize(other.m_plaintextSize)
        , m_nonce(std::move(other.m_nonce))
        , m_authTag(std::move(other.m_authTag))
        , m_additionalData(std::move(other.m_additionalData))
        , m_inputBuffer(std::move(other.m_inputBuffer))
        , m_outputBuffer(std::move(other.m_outputBuffer))
        , m_initialized(other.m_initialized)
        , m_authValid(other.m_authValid)
    {
    }

    Decryptor& Decryptor::operator=(Decryptor&& other) noexcept {
        if (this != &other) {
            cleanup();
            m_algorithmHandle = std::exchange(other.m_algorithmHandle, nullptr);
            m_keyHandle = std::exchange(other.m_keyHandle, nullptr);
            m_bufferSize = other.m_bufferSize;
            m_cancelled.store(other.m_cancelled.load());
            m_decrypting.store(other.m_decrypting.load());
            m_lastError = std::move(other.m_lastError);
            m_encryptedSize = other.m_encryptedSize;
            m_plaintextSize = other.m_plaintextSize;
            m_nonce = std::move(other.m_nonce);
            m_authTag = std::move(other.m_authTag);
            m_additionalData = std::move(other.m_additionalData);
            m_inputBuffer = std::move(other.m_inputBuffer);
            m_outputBuffer = std::move(other.m_outputBuffer);
            m_initialized = other.m_initialized;
            m_authValid = other.m_authValid;
        }
        return *this;
    }

    bool Decryptor::initialize(const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& nonce,
        const std::vector<uint8_t>& authTag,
        const std::vector<uint8_t>& additionalData) {
        if (!m_algorithmHandle) {
            m_lastError = "Algorithm provider not initialized";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (key.size() != 32) {
            m_lastError = "Key must be 32 bytes for AES-256";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (nonce.size() != 12) {
            m_lastError = "Nonce must be 12 bytes for AES-GCM";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (authTag.size() != 16) {
            m_lastError = "Authentication tag must be 16 bytes";
            Logger::instance().error(m_lastError);
            return false;
        }

        m_nonce = nonce;
        m_authTag = authTag;
        m_additionalData = additionalData;

        NTSTATUS status = BCryptImportKey(
            m_algorithmHandle,
            nullptr,
            BCRYPT_KEY_DATA_BLOB,
            &m_keyHandle,
            nullptr,
            0,
            (PUCHAR)key.data(),
            static_cast<ULONG>(key.size()),
            0
        );

        if (status != 0) {
            m_lastError = "Failed to import key";
            Logger::instance().error(m_lastError);
            return false;
        }

        m_initialized = true;
        m_authValid = false;
        return true;
    }

    bool Decryptor::decryptFile(const std::string& inputFile,
        const std::string& outputFile,
        ProgressCallback progress) {
        if (!m_initialized || !m_algorithmHandle || !m_keyHandle) {
            m_lastError = "Decryptor not properly initialized";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (m_decrypting) {
            m_lastError = "Decryption already in progress";
            Logger::instance().warning(m_lastError);
            return false;
        }

        m_cancelled = false;
        m_decrypting = true;
        m_encryptedSize = 0;
        m_plaintextSize = 0;
        m_authValid = false;
        m_lastError.clear();

        try {
            std::ifstream inputFileStream(inputFile, std::ios::binary);
            if (!inputFileStream.is_open()) {
                m_lastError = "Failed to open input file: " + inputFile;
                Logger::instance().error(m_lastError);
                m_decrypting = false;
                return false;
            }

            std::ofstream outputFileStream(outputFile, std::ios::binary);
            if (!outputFileStream.is_open()) {
                m_lastError = "Failed to open output file: " + outputFile;
                Logger::instance().error(m_lastError);
                m_decrypting = false;
                return false;
            }

            std::vector<uint8_t> fileNonce(12);
            inputFileStream.read(reinterpret_cast<char*>(fileNonce.data()), 12);
            if (inputFileStream.gcount() != 12) {
                m_lastError = "Failed to read nonce from file";
                Logger::instance().error(m_lastError);
                m_decrypting = false;
                return false;
            }

            if (fileNonce != m_nonce) {
                m_lastError = "Nonce mismatch";
                Logger::instance().error(m_lastError);
                m_decrypting = false;
                return false;
            }

            inputFileStream.seekg(0, std::ios::end);
            m_encryptedSize = static_cast<uint64_t>(inputFileStream.tellg()) - 12;
            inputFileStream.seekg(12, std::ios::beg);

            bool result = decryptStreaming(inputFileStream, outputFileStream, progress);
            m_decrypting = false;
            return result;
        } catch (const std::exception& e) {
            m_lastError = "Decryption exception: " + std::string(e.what());
            Logger::instance().error(m_lastError);
            m_decrypting = false;
            return false;
        }
    }

    bool Decryptor::decryptToCallback(const std::string& inputFile,
        DataCallback dataCallback,
        ProgressCallback progress) {
        if (!m_initialized || !m_algorithmHandle || !m_keyHandle) {
            m_lastError = "Decryptor not properly initialized";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (m_decrypting) {
            m_lastError = "Decryption already in progress";
            Logger::instance().warning(m_lastError);
            return false;
        }

        if (!dataCallback) {
            m_lastError = "Data callback cannot be null";
            Logger::instance().error(m_lastError);
            return false;
        }

        m_cancelled = false;
        m_decrypting = true;
        m_encryptedSize = 0;
        m_plaintextSize = 0;
        m_authValid = false;
        m_lastError.clear();

        try {
            std::ifstream inputFileStream(inputFile, std::ios::binary);
            if (!inputFileStream.is_open()) {
                m_lastError = "Failed to open input file: " + inputFile;
                Logger::instance().error(m_lastError);
                m_decrypting = false;
                return false;
            }

            std::vector<uint8_t> fileNonce(12);
            inputFileStream.read(reinterpret_cast<char*>(fileNonce.data()), 12);
            if (inputFileStream.gcount() != 12) {
                m_lastError = "Failed to read nonce from file";
                Logger::instance().error(m_lastError);
                m_decrypting = false;
                return false;
            }

            if (fileNonce != m_nonce) {
                m_lastError = "Nonce mismatch";
                Logger::instance().error(m_lastError);
                m_decrypting = false;
                return false;
            }

            inputFileStream.seekg(0, std::ios::end);
            m_encryptedSize = static_cast<uint64_t>(inputFileStream.tellg()) - 12;
            inputFileStream.seekg(12, std::ios::beg);

            bool result = decryptStreamingToCallback(inputFileStream, dataCallback, progress);
            m_decrypting = false;
            return result;
        } catch (const std::exception& e) {
            m_lastError = "Decryption exception: " + std::string(e.what());
            Logger::instance().error(m_lastError);
            m_decrypting = false;
            return false;
        }
    }

    bool Decryptor::decryptBuffer(const uint8_t* inputData, size_t inputSize,
        ByteVector& outputData, const std::vector<uint8_t>& authTag) {
        if (!m_initialized || !m_algorithmHandle || !m_keyHandle) {
            m_lastError = "Decryptor not properly initialized";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (m_decrypting) {
            m_lastError = "Decryption already in progress";
            Logger::instance().warning(m_lastError);
            return false;
        }

        if (!inputData || inputSize == 0) {
            m_lastError = "Invalid input data";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (authTag.size() != 16) {
            m_lastError = "Authentication tag must be 16 bytes";
            Logger::instance().error(m_lastError);
            return false;
        }

        m_cancelled = false;
        m_decrypting = true;
        m_encryptedSize = inputSize;
        m_plaintextSize = 0;
        m_authValid = false;
        m_lastError.clear();

        try {
            BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
            initAuthInfo(&authInfo);
            authInfo.pbNonce = const_cast<PUCHAR>(m_nonce.data());
            authInfo.cbNonce = static_cast<ULONG>(m_nonce.size());
            authInfo.pbTag = const_cast<PUCHAR>(authTag.data());
            authInfo.cbTag = static_cast<ULONG>(authTag.size());
            authInfo.pbAuthData = m_additionalData.empty() ? nullptr : const_cast<PUCHAR>(m_additionalData.data());
            authInfo.cbAuthData = static_cast<ULONG>(m_additionalData.size());

            ULONG decryptedSize = 0;
            NTSTATUS status = BCryptDecrypt(
                m_keyHandle,
                const_cast<PUCHAR>(inputData),
                static_cast<ULONG>(inputSize),
                &authInfo,
                nullptr,
                0,
                nullptr,
                0,
                &decryptedSize,
                0
            );

            if (status != 0) {
                m_lastError = "BCryptDecrypt size query failed";
                Logger::instance().error(m_lastError);
                m_decrypting = false;
                return false;
            }

            outputData.resize(decryptedSize);
            ULONG bytesWritten = 0;
            status = BCryptDecrypt(
                m_keyHandle,
                const_cast<PUCHAR>(inputData),
                static_cast<ULONG>(inputSize),
                &authInfo,
                nullptr,
                0,
                outputData.data(),
                static_cast<ULONG>(outputData.size()),
                &bytesWritten,
                0
            );

            if (status != 0) {
                m_lastError = "BCryptDecrypt failed - data may be corrupted or tampered";
                Logger::instance().error(m_lastError);
                m_decrypting = false;
                return false;
            }

            outputData.resize(bytesWritten);
            m_plaintextSize = bytesWritten;
            m_authValid = true;

            m_decrypting = false;
            return true;
        } catch (const std::exception& e) {
            m_lastError = "Decryption exception: " + std::string(e.what());
            Logger::instance().error(m_lastError);
            m_decrypting = false;
            return false;
        }
    }

    void Decryptor::cancel() {
        m_cancelled.store(true, std::memory_order_release);
        Logger::instance().info("Decryption cancellation requested");
    }

    bool Decryptor::decryptStreaming(std::istream& inputStream,
        std::ostream& outputStream,
        ProgressCallback progress) {
        size_t totalRead = 0;
        size_t lastProgressUpdate = 0;
        const size_t progressUpdateInterval = m_bufferSize * 10;

        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
        initAuthInfo(&authInfo);
        authInfo.pbNonce = const_cast<PUCHAR>(m_nonce.data());
        authInfo.cbNonce = static_cast<ULONG>(m_nonce.size());
        authInfo.pbTag = const_cast<PUCHAR>(m_authTag.data());
        authInfo.cbTag = static_cast<ULONG>(m_authTag.size());
        authInfo.pbAuthData = m_additionalData.empty() ? nullptr : const_cast<PUCHAR>(m_additionalData.data());
        authInfo.cbAuthData = static_cast<ULONG>(m_additionalData.size());

        while (!inputStream.eof() && !m_cancelled) {
            inputStream.read(reinterpret_cast<char*>(m_inputBuffer.get()), m_bufferSize);
            size_t bytesRead = static_cast<size_t>(inputStream.gcount());

            if (bytesRead == 0) {
                break;
            }

            ULONG decryptedSize = 0;
            NTSTATUS status = BCryptDecrypt(
                m_keyHandle,
                const_cast<PUCHAR>(m_inputBuffer.get()),
                static_cast<ULONG>(bytesRead),
                &authInfo,
                nullptr,
                0,
                nullptr,
                0,
                &decryptedSize,
                0
            );

            if (status != 0) {
                m_lastError = "BCryptDecrypt size query failed during streaming";
                Logger::instance().error(m_lastError);
                return false;
            }

            if (decryptedSize > 0) {
                std::vector<uint8_t> decryptedBuffer(decryptedSize);
                ULONG bytesWritten = 0;
                status = BCryptDecrypt(
                    m_keyHandle,
                    const_cast<PUCHAR>(m_inputBuffer.get()),
                    static_cast<ULONG>(bytesRead),
                    &authInfo,
                    nullptr,
                    0,
                    decryptedBuffer.data(),
                    static_cast<ULONG>(decryptedBuffer.size()),
                    &bytesWritten,
                    0
                );

                if (status != 0) {
                    m_lastError = "BCryptDecrypt failed during streaming";
                    Logger::instance().error(m_lastError);
                    return false;
                }

                outputStream.write(reinterpret_cast<const char*>(decryptedBuffer.data()), bytesWritten);
                m_plaintextSize += bytesWritten;
            }

            totalRead += bytesRead;

            if (progress && (totalRead - lastProgressUpdate) >= progressUpdateInterval) {
                progress(totalRead, m_encryptedSize);
                lastProgressUpdate = totalRead;
            }
        }

        if (m_cancelled) {
            m_lastError = "Decryption cancelled by user";
            Logger::instance().info(m_lastError);
            return false;
        }

        m_authValid = true;

        if (progress) {
            progress(m_encryptedSize, m_encryptedSize);
        }

        return true;
    }

    bool Decryptor::decryptStreamingToCallback(std::istream& inputStream,
        DataCallback dataCallback,
        ProgressCallback progress) {
        size_t totalRead = 0;
        size_t lastProgressUpdate = 0;
        const size_t progressUpdateInterval = m_bufferSize * 10;

        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
        initAuthInfo(&authInfo);
        authInfo.pbNonce = const_cast<PUCHAR>(m_nonce.data());
        authInfo.cbNonce = static_cast<ULONG>(m_nonce.size());
        authInfo.pbTag = const_cast<PUCHAR>(m_authTag.data());
        authInfo.cbTag = static_cast<ULONG>(m_authTag.size());
        authInfo.pbAuthData = m_additionalData.empty() ? nullptr : const_cast<PUCHAR>(m_additionalData.data());
        authInfo.cbAuthData = static_cast<ULONG>(m_additionalData.size());

        while (!inputStream.eof() && !m_cancelled) {
            inputStream.read(reinterpret_cast<char*>(m_inputBuffer.get()), m_bufferSize);
            size_t bytesRead = static_cast<size_t>(inputStream.gcount());

            if (bytesRead == 0) {
                break;
            }

            ULONG decryptedSize = 0;
            NTSTATUS status = BCryptDecrypt(
                m_keyHandle,
                const_cast<PUCHAR>(m_inputBuffer.get()),
                static_cast<ULONG>(bytesRead),
                &authInfo,
                nullptr,
                0,
                nullptr,
                0,
                &decryptedSize,
                0
            );

            if (status != 0) {
                m_lastError = "BCryptDecrypt size query failed during streaming";
                Logger::instance().error(m_lastError);
                return false;
            }

            if (decryptedSize > 0) {
                std::vector<uint8_t> decryptedBuffer(decryptedSize);
                ULONG bytesWritten = 0;
                status = BCryptDecrypt(
                    m_keyHandle,
                    const_cast<PUCHAR>(m_inputBuffer.get()),
                    static_cast<ULONG>(bytesRead),
                    &authInfo,
                    nullptr,
                    0,
                    decryptedBuffer.data(),
                    static_cast<ULONG>(decryptedBuffer.size()),
                    &bytesWritten,
                    0
                );

                if (status != 0) {
                    m_lastError = "BCryptDecrypt failed during streaming";
                    Logger::instance().error(m_lastError);
                    return false;
                }

                dataCallback(decryptedBuffer.data(), bytesWritten);
                m_plaintextSize += bytesWritten;
            }

            totalRead += bytesRead;

            if (progress && (totalRead - lastProgressUpdate) >= progressUpdateInterval) {
                progress(totalRead, m_encryptedSize);
                lastProgressUpdate = totalRead;
            }
        }

        if (m_cancelled) {
            m_lastError = "Decryption cancelled by user";
            Logger::instance().info(m_lastError);
            return false;
        }

        m_authValid = true;

        if (progress) {
            progress(m_encryptedSize, m_encryptedSize);
        }

        return true;
    }

    void Decryptor::cleanup() {
        if (m_keyHandle) {
            BCryptDestroyKey(m_keyHandle);
            m_keyHandle = nullptr;
        }
        if (m_algorithmHandle) {
            BCryptCloseAlgorithmProvider(m_algorithmHandle, 0);
            m_algorithmHandle = nullptr;
        }
        m_inputBuffer.reset();
        m_outputBuffer.reset();
        m_initialized = false;
        m_authValid = false;
    }

} // namespace noty