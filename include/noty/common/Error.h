#pragma once
#include <stdexcept>
#include <string>

namespace noty {

    class Error : public std::runtime_error {
    public:
        explicit Error(const std::string& message);
        Error(const std::string& message, int code);

        int code() const;

    private:
        int m_code = 0;
    };

} // namespace noty