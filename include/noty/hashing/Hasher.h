#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

namespace noty {

    class Hasher {
    public:
        enum class Algorithm {
            BLAKE3
        };

        Hasher(Algorithm algorithm = Algorithm::BLAKE3);
        ~Hasher();

        std::string hashFile(const std::string& filename) const;
        std::string hashData(const std::vector<uint8_t>& data) const;
        std::string hashData(const uint8_t* data, size_t size) const;

        bool hashFileStreaming(const std::string& filename,
            std::function<void(size_t bytesProcessed, size_t totalBytes)> progress = nullptr) const;

        bool verifyFile(const std::string& filename, const std::string& expectedHash) const;

        std::string getAlgorithmName() const;

        static std::string bytesToHex(const uint8_t* bytes, size_t size);
        static std::vector<uint8_t> hexToBytes(const std::string& hex);

    private:
        Algorithm m_algorithm;

        std::string hashBlake3(const uint8_t* data, size_t size) const;
    };

} // namespace noty