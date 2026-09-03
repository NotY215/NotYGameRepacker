#include "noty/crypto/Encryptor.h"
#include "noty/common/Logger.h"
#include <windows.h>
#include <bcrypt.h>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

namespace noty {

    // Helper function to initialize authenticated cipher mode info
    static void initAuthInfo(BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO* info) {
        memset(info, 0, sizeof(BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO));
        info->cbSize = sizeof(BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO);
        info->dwInfoVersion = BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO_VERSION;
    }

    Encryptor::Encryptor(size_t bufferSize)
        : m_algorithmHandle(nullptr)
        , m_keyHandle(nullptr)
        , m_bufferSize(bufferSize)
        , m_cancelled(false)
        , m_encrypting(false)
        , m_encryptedSize(0)
        , m_plaintextSize(0)
        , m_initialized(false)
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

        m_inputBuffer = std::make_unique<uint8_t[]>(m_bufferSize);
        m_outputBuffer = std::make_unique<uint8_t[]>(m_bufferSize + 16);

        Logger::instance().info("Encryptor initialized with buffer size: " + std::to_string(m_bufferSize));
    }

    Encryptor::~Encryptor() {
        cleanup();
    }

    Encryptor::Encryptor(Encryptor&& other) noexcept
        : m_algorithmHandle(std::exchange(other.m_algorithmHandle, nullptr))
        , m_keyHandle(std::exchange(other.m_keyHandle, nullptr))
        , m_bufferSize(other.m_bufferSize)
        , m_cancelled(other.m_cancelled.load())
        , m_encrypting(other.m_encrypting.load())
        , m_lastError(std::move(other.m_lastError))
        , m_encryptedSize(other.m_encryptedSize)
        , m_plaintextSize(other.m_plaintextSize)
        , m_nonce(std::move(other.m_nonce))
        , m_authTag(std::move(other.m_authTag))
        , m_additionalData(std::move(other.m_additionalData))
        , m_inputBuffer(std::move(other.m_inputBuffer))
        , m_outputBuffer(std::move(other.m_outputBuffer))
        , m_initialized(other.m_initialized)
    {
    }

    Encryptor& Encryptor::operator=(Encryptor&& other) noexcept {
        if (this != &other) {
            cleanup();
            m_algorithmHandle = std::exchange(other.m_algorithmHandle, nullptr);
            m_keyHandle = std::exchange(other.m_keyHandle, nullptr);
            m_bufferSize = other.m_bufferSize;
            m_cancelled.store(other.m_cancelled.load());
            m_encrypting.store(other.m_encrypting.load());
            m_lastError = std::move(other.m_lastError);
            m_encryptedSize = other.m_encryptedSize;
            m_plaintextSize = other.m_plaintextSize;
            m_nonce = std::move(other.m_nonce);
            m_authTag = std::move(other.m_authTag);
            m_additionalData = std::move(other.m_additionalData);
            m_inputBuffer = std::move(other.m_inputBuffer);
            m_outputBuffer = std::move(other.m_outputBuffer);
            m_initialized = other.m_initialized;
        }
        return *this;
    }

    bool Encryptor::initialize(const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& nonce,
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

        m_nonce = nonce;
        m_additionalData = additionalData;
        m_authTag.clear();

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
        Logger::instance().info("Encryptor initialized with key and nonce");
        return true;
    }

    bool Encryptor::encryptFile(const std::string& inputFile,
        const std::string& outputFile,
        ProgressCallback progress) {
        if (!m_initialized || !m_algorithmHandle || !m_keyHandle) {
            m_lastError = "Encryptor not properly initialized";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (m_encrypting) {
            m_lastError = "Encryption already in progress";
            Logger::instance().warning(m_lastError);
            return false;
        }

        m_cancelled = false;
        m_encrypting = true;
        m_encryptedSize = 0;
        m_plaintextSize = 0;
        m_authTag.clear();
        m_lastError.clear();

        try {
            std::ifstream inputFileStream(inputFile, std::ios::binary);
            if (!inputFileStream.is_open()) {
                m_lastError = "Failed to open input file: " + inputFile;
                Logger::instance().error(m_lastError);
                m_encrypting = false;
                return false;
            }

            std::ofstream outputFileStream(outputFile, std::ios::binary);
            if (!outputFileStream.is_open()) {
                m_lastError = "Failed to open output file: " + outputFile;
                Logger::instance().error(m_lastError);
                m_encrypting = false;
                return false;
            }

            inputFileStream.seekg(0, std::ios::end);
            m_plaintextSize = static_cast<uint64_t>(inputFileStream.tellg());
            inputFileStream.seekg(0, std::ios::beg);

            outputFileStream.write(reinterpret_cast<const char*>(m_nonce.data()), m_nonce.size());

            bool result = encryptStreaming(inputFileStream, outputFileStream, progress);
            m_encrypting = false;
            return result;
        }
        catch (const std::exception& e) {
            m_lastError = "Encryption exception: " + std::string(e.what());
            Logger::instance().error(m_lastError);
            m_encrypting = false;
            return false;
        }
    }

    bool Encryptor::encryptToCallback(const std::string& inputFile,
        DataCallback dataCallback,
        ProgressCallback progress) {
        if (!m_initialized || !m_algorithmHandle || !m_keyHandle) {
            m_lastError = "Encryptor not properly initialized";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (m_encrypting) {
            m_lastError = "Encryption already in progress";
            Logger::instance().warning(m_lastError);
            return false;
        }

        if (!dataCallback) {
            m_lastError = "Data callback cannot be null";
            Logger::instance().error(m_lastError);
            return false;
        }

        m_cancelled = false;
        m_encrypting = true;
        m_encryptedSize = 0;
        m_plaintextSize = 0;
        m_authTag.clear();
        m_lastError.clear();

        try {
            std::ifstream inputFileStream(inputFile, std::ios::binary);
            if (!inputFileStream.is_open()) {
                m_lastError = "Failed to open input file: " + inputFile;
                Logger::instance().error(m_lastError);
                m_encrypting = false;
                return false;
            }

            inputFileStream.seekg(0, std::ios::end);
            m_plaintextSize = static_cast<uint64_t>(inputFileStream.tellg());
            inputFileStream.seekg(0, std::ios::beg);

            dataCallback(m_nonce.data(), m_nonce.size());

            bool result = encryptStreamingToCallback(inputFileStream, dataCallback, progress);
            m_encrypting = false;
            return result;
        }
        catch (const std::exception& e) {
            m_lastError = "Encryption exception: " + std::string(e.what());
            Logger::instance().error(m_lastError);
            m_encrypting = false;
            return false;
        }
    }

    bool Encryptor::encryptBuffer(const uint8_t* inputData, size_t inputSize,
        ByteVector& outputData, ByteVector& authTag) {
        if (!m_initialized || !m_algorithmHandle || !m_keyHandle) {
            m_lastError = "Encryptor not properly initialized";
            Logger::instance().error(m_lastError);
            return false;
        }

        if (m_encrypting) {
            m_lastError = "Encryption already in progress";
            Logger::instance().warning(m_lastError);
            return false;
        }

        if (!inputData || inputSize == 0) {
            m_lastError = "Invalid input data";
            Logger::instance().error(m_lastError);
            return false;
        }

        m_cancelled = false;
        m_encrypting = true;
        m_encryptedSize = 0;
        m_plaintextSize = inputSize;
        m_authTag.clear();
        m_lastError.clear();

        try {
            BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
            initAuthInfo(&authInfo);
            authInfo.pbNonce = const_cast<PUCHAR>(m_nonce.data());
            authInfo.cbNonce = static_cast<ULONG>(m_nonce.size());
            authInfo.pbAuthData = m_additionalData.empty() ? nullptr : const_cast<PUCHAR>(m_additionalData.data());
            authInfo.cbAuthData = static_cast<ULONG>(m_additionalData.size());

            ULONG encryptedSize = 0;
            NTSTATUS status = BCryptEncrypt(
                m_keyHandle,
                const_cast<PUCHAR>(inputData),
                static_cast<ULONG>(inputSize),
                &authInfo,
                nullptr,
                0,
                nullptr,
                0,
                &encryptedSize,
                0
            );

            if (status != 0) {
                m_lastError = "BCryptEncrypt size query failed";
                Logger::instance().error(m_lastError);
                m_encrypting = false;
                return false;
            }

            outputData.resize(encryptedSize);
            authTag.resize(16);
            authInfo.pbTag = authTag.data();
            authInfo.cbTag = 16;

            ULONG bytesWritten = 0;
            status = BCryptEncrypt(
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
                m_lastError = "BCryptEncrypt failed";
                Logger::instance().error(m_lastError);
                m_encrypting = false;
                return false;
            }

            outputData.resize(bytesWritten);
            m_encryptedSize = bytesWritten;
            m_authTag = authTag;

            m_encrypting = false;
            Logger::instance().info("Buffer encryption complete: " +
                std::to_string(inputSize) + " -> " + std::to_string(bytesWritten) + " bytes");
            return true;
        }
        catch (const std::exception& e) {
            m_lastError = "Encryption exception: " + std::string(e.what());
            Logger::instance().error(m_lastError);
            m_encrypting = false;
            return false;
        }
    }

    std::vector<uint8_t> Encryptor::getAuthenticationTag() const {
        return m_authTag;
    }

    void Encryptor::cancel() {
        m_cancelled.store(true, std::memory_order_release);
        Logger::instance().info("Encryption cancellation requested");
    }

    bool Encryptor::encryptStreaming(std::istream& inputStream,
        std::ostream& outputStream,
        ProgressCallback progress) {
        size_t totalRead = 0;
        size_t lastProgressUpdate = 0;
        const size_t progressUpdateInterval = m_bufferSize * 10;

        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
        initAuthInfo(&authInfo);
        authInfo.pbNonce = const_cast<PUCHAR>(m_nonce.data());
        authInfo.cbNonce = static_cast<ULONG>(m_nonce.size());
        authInfo.pbAuthData = m_additionalData.empty() ? nullptr : const_cast<PUCHAR>(m_additionalData.data());
        authInfo.cbAuthData = static_cast<ULONG>(m_additionalData.size());

        std::vector<uint8_t> authTag(16);
        authInfo.pbTag = authTag.data();
        authInfo.cbTag = 16;

        while (!inputStream.eof() && !m_cancelled) {
            inputStream.read(reinterpret_cast<char*>(m_inputBuffer.get()), m_bufferSize);
            size_t bytesRead = static_cast<size_t>(inputStream.gcount());

            if (bytesRead == 0) {
                break;
            }

            ULONG encryptedSize = 0;
            NTSTATUS status = BCryptEncrypt(
                m_keyHandle,
                const_cast<PUCHAR>(m_inputBuffer.get()),
                static_cast<ULONG>(bytesRead),
                &authInfo,
                nullptr,
                0,
                nullptr,
                0,
                &encryptedSize,
                0
            );

            if (status != 0) {
                m_lastError = "BCryptEncrypt size query failed during streaming";
                Logger::instance().error(m_lastError);
                return false;
            }

            if (encryptedSize > 0) {
                std::vector<uint8_t> encryptedBuffer(encryptedSize);
                ULONG bytesWritten = 0;
                status = BCryptEncrypt(
                    m_keyHandle,
                    const_cast<PUCHAR>(m_inputBuffer.get()),
                    static_cast<ULONG>(bytesRead),
                    &authInfo,
                    nullptr,
                    0,
                    encryptedBuffer.data(),
                    static_cast<ULONG>(encryptedBuffer.size()),
                    &bytesWritten,
                    0
                );

                if (status != 0) {
                    m_lastError = "BCryptEncrypt failed during streaming";
                    Logger::instance().error(m_lastError);
                    return false;
                }

                outputStream.write(reinterpret_cast<const char*>(encryptedBuffer.data()), bytesWritten);
                m_encryptedSize += bytesWritten;
            }

            totalRead += bytesRead;

            if (progress && (totalRead - lastProgressUpdate) >= progressUpdateInterval) {
                progress(totalRead, m_plaintextSize);
                lastProgressUpdate = totalRead;
            }
        }

        if (m_cancelled) {
            m_lastError = "Encryption cancelled by user";
            Logger::instance().info(m_lastError);
            return false;
        }

        outputStream.write(reinterpret_cast<const char*>(authTag.data()), authTag.size());
        m_authTag = authTag;
        m_encryptedSize += authTag.size();

        if (progress) {
            progress(m_plaintextSize, m_plaintextSize);
        }

        Logger::instance().info("Encryption complete: " +
            std::to_string(m_plaintextSize) + " -> " +
            std::to_string(m_encryptedSize) + " bytes");

        return true;
    }

    bool Encryptor::encryptStreamingToCallback(std::istream& inputStream,
        DataCallback dataCallback,
        ProgressCallback progress) {
        size_t totalRead = 0;
        size_t lastProgressUpdate = 0;
        const size_t progressUpdateInterval = m_bufferSize * 10;

        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
        initAuthInfo(&authInfo);
        authInfo.pbNonce = const_cast<PUCHAR>(m_nonce.data());
        authInfo.cbNonce = static_cast<ULONG>(m_nonce.size());
        authInfo.pbAuthData = m_additionalData.empty() ? nullptr : const_cast<PUCHAR>(m_additionalData.data());
        authInfo.cbAuthData = static_cast<ULONG>(m_additionalData.size());

        std::vector<uint8_t> authTag(16);
        authInfo.pbTag = authTag.data();
        authInfo.cbTag = 16;

        while (!inputStream.eof() && !m_cancelled) {
            inputStream.read(reinterpret_cast<char*>(m_inputBuffer.get()), m_bufferSize);
            size_t bytesRead = static_cast<size_t>(inputStream.gcount());

            if (bytesRead == 0) {
                break;
            }

            ULONG encryptedSize = 0;
            NTSTATUS status = BCryptEncrypt(
                m_keyHandle,
                const_cast<PUCHAR>(m_inputBuffer.get()),
                static_cast<ULONG>(bytesRead),
                &authInfo,
                nullptr,
                0,
                nullptr,
                0,
                &encryptedSize,
                0
            );

            if (status != 0) {
                m_lastError = "BCryptEncrypt size query failed during streaming";
                Logger::instance().error(m_lastError);
                return false;
            }

            if (encryptedSize > 0) {
                std::vector<uint8_t> encryptedBuffer(encryptedSize);
                ULONG bytesWritten = 0;
                status = BCryptEncrypt(
                    m_keyHandle,
                    const_cast<PUCHAR>(m_inputBuffer.get()),
                    static_cast<ULONG>(bytesRead),
                    &authInfo,
                    nullptr,
                    0,
                    encryptedBuffer.data(),
                    static_cast<ULONG>(encryptedBuffer.size()),
                    &bytesWritten,
                    0
                );

                if (status != 0) {
                    m_lastError = "BCryptEncrypt failed during streaming";
                    Logger::instance().error(m_lastError);
                    return false;
                }

                dataCallback(encryptedBuffer.data(), bytesWritten);
                m_encryptedSize += bytesWritten;
            }

            totalRead += bytesRead;

            if (progress && (totalRead - lastProgressUpdate) >= progressUpdateInterval) {
                progress(totalRead, m_plaintextSize);
                lastProgressUpdate = totalRead;
            }
        }

        if (m_cancelled) {
            m_lastError = "Encryption cancelled by user";
            Logger::instance().info(m_lastError);
            return false;
        }

        dataCallback(authTag.data(), authTag.size());
        m_authTag = authTag;
        m_encryptedSize += authTag.size();

        if (progress) {
            progress(m_plaintextSize, m_plaintextSize);
        }

        return true;
    }

    void Encryptor::cleanup() {
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
    }

} // namespace noty