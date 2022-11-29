#include "Crypto.h"

#include <cstring>

#include <algorithm>

namespace Crypto {
    bool EncryptString(std::string const& value, ByteStream& result) {
        result.resize(value.size());
        memcpy(result.data(), value.data(), value.size());
        return true;
    }

    bool DecryptString(ByteStream const& value, std::string& result) {
        result.resize(value.size());
        for (std::size_t i(0); i < value.size(); ++i) {
            result[i] = static_cast<char>(value[i]);
        }
        return true;
    }
} // namespace Crypto