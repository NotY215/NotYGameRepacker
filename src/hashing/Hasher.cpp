#include "noty/hashing/Hasher.h"
#include "noty/common/Logger.h"
#include <blake3.h>
#include <openssl/sha.h>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace noty {

    Hasher::Hasher(Algorithm algorithm)
        : m_algorithm(algorithm) {
    }

    Hasher::~Hasher() = default;

    std::string Hasher::hashFile(const std::string& filename) const {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            Logger::instance().error("Failed to open file for hashing: " + filename);
            return "";
        }

        // Get file size
        file.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            Logger::instance().error("Failed to read file for hashing: " + filename);
            return "";
        }

        return hashData(buffer.data(), size);
    }

    std::string Hasher::hashData(const std::vector<uint8_t>& data) const {
        return hashData(data.data(), data.size());
    }

    std::string Hasher::hashData(const uint8_t* data, size_t size) const {
        switch (m_algorithm) {
        case Algorithm::BLAKE3:
            return hashBlake3(data, size);
        case Algorithm::SHA256:
            return hashSha256(data, size);
        default:
            Logger::instance().error("Unknown hashing algorithm");
            return "";
        }
    }

    bool Hasher::hashFileStreaming(const std::string& filename,
        std::function<void(size_t, size_t)> progress) const {
        const size_t BUFFER_SIZE = 1024 * 1024; // 1MB buffer

        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            Logger::instance().error("Failed to open file for streaming hash: " + filename);
            return false;
        }

        // Get file size
        file.seekg(0, std::ios::end);
        size_t totalSize = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        size_t processed = 0;
        std::vector<uint8_t> buffer(BUFFER_SIZE);

        // For BLAKE3
        blake3_hasher hasher;
        blake3_hasher_init(&hasher);

        while (!file.eof()) {
            file.read(reinterpret_cast<char*>(buffer.data()), BUFFER_SIZE);
            size_t bytesRead = static_cast<size_t>(file.gcount());

            if (bytesRead > 0) {
                blake3_hasher_update(&hasher, buffer.data(), bytesRead);
                processed += bytesRead;

                if (progress) {
                    progress(processed, totalSize);
                }
            }
        }

        // Finalize (but we don't need the result for streaming)
        // The caller may want to verify the result separately

        file.close();
        return true;
    }

    bool Hasher::verifyFile(const std::string& filename, const std::string& expectedHash) const {
        std::string actualHash = hashFile(filename);
        if (actualHash.empty()) {
            return false;
        }

        // Case-insensitive comparison
        std::transform(actualHash.begin(), actualHash.end(), actualHash.begin(), ::tolower);
        std::string expectedLower = expectedHash;
        std::transform(expectedLower.begin(), expectedLower.end(), expectedLower.begin(), ::tolower);

        return actualHash == expectedLower;
    }

    std::string Hasher::getAlgorithmName() const {
        switch (m_algorithm) {
        case Algorithm::BLAKE3:
            return "BLAKE3";
        case Algorithm::SHA256:
            return "SHA-256";
        default:
            return "Unknown";
        }
    }

    std::string Hasher::hashBlake3(const uint8_t* data, size_t size) const {
        uint8_t hash[BLAKE3_OUT_LEN];
        blake3_hasher hasher;
        blake3_hasher_init(&hasher);
        blake3_hasher_update(&hasher, data, size);
        blake3_hasher_finalize(&hasher, hash, BLAKE3_OUT_LEN);

        return bytesToHex(hash, BLAKE3_OUT_LEN);
    }

    std::string Hasher::hashSha256(const uint8_t* data, size_t size) const {
        uint8_t hash[SHA256_DIGEST_LENGTH];
        SHA256(data, size, hash);
        return bytesToHex(hash, SHA256_DIGEST_LENGTH);
    }

    std::string Hasher::bytesToHex(const uint8_t* bytes, size_t size) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (size_t i = 0; i < size; ++i) {
            ss << std::setw(2) << static_cast<int>(bytes[i]);
        }
        return ss.str();
    }

    std::vector<uint8_t> Hasher::hexToBytes(const std::string& hex) {
        std::vector<uint8_t> bytes;
        bytes.reserve(hex.size() / 2);

        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byteString = hex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
            bytes.push_back(byte);
        }

        return bytes;
    }

} // namespace noty