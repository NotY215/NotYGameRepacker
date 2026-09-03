#include "noty/common/Version.h"

namespace noty {

    constexpr int MAJOR_VERSION = 1;
    constexpr int MINOR_VERSION = 0;
    constexpr int PATCH_VERSION = 0;

    std::string getVersionString() {
        return std::to_string(MAJOR_VERSION) + "." +
            std::to_string(MINOR_VERSION) + "." +
            std::to_string(PATCH_VERSION);
    }

    int getMajorVersion() { return MAJOR_VERSION; }
    int getMinorVersion() { return MINOR_VERSION; }
    int getPatchVersion() { return PATCH_VERSION; }

} // namespace noty