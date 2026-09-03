#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>

namespace noty {

    using Byte = uint8_t;
    using ByteVector = std::vector<Byte>;
    using FileSize = uint64_t;

} // namespace noty