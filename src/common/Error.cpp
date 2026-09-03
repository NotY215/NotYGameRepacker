#include "noty/common/Error.h"

namespace noty {

    Error::Error(const std::string& message)
        : std::runtime_error(message) {
    }

    Error::Error(const std::string& message, int code)
        : std::runtime_error(message), m_code(code) {
    }

    int Error::code() const {
        return m_code;
    }

} // namespace noty