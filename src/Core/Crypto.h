#pragma once

#include <string>
#include <vector>

namespace Crypto {
    typedef std::vector<std::uint8_t> ByteStream;
    bool EncryptString(std::string const& value, ByteStream& result);
    bool DecryptString(ByteStream const& value, std::string& result);
} // namespace Crypto