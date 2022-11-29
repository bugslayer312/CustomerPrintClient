#include "Crypto.h"

#include <windows.h>
#include <wincrypt.h>

#include <stdexcept>
#include <cstring>
#include <memory>

namespace Crypto {
    bool EncryptString(std::string const& value, ByteStream& result) {
        DATA_BLOB dataIn, dataOut;
        dataIn.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(value.c_str()));
        dataIn.cbData = static_cast<DWORD>(value.size());
        bool success = CryptProtectData(&dataIn, NULL, NULL, NULL, NULL, 0, &dataOut) != 0;
        if (success) {
            result.resize(dataOut.cbData);
            memcpy(result.data(), dataOut.pbData, dataOut.cbData);
        }
        LocalFree(dataOut.pbData);
        return success;
    }
    
    bool DecryptString(ByteStream const& value, std::string& result) {
        DATA_BLOB dataIn, dataOut;
        dataIn.pbData = const_cast<BYTE*>(value.data());
        dataIn.cbData = static_cast<DWORD>(value.size());
        bool success = CryptUnprotectData(&dataIn, NULL, NULL, NULL, NULL, 0, &dataOut) != 0;
        if (success) {
            std::unique_ptr<char[]> buf(new char[dataOut.cbData+1]);
            memcpy(buf.get(), dataOut.pbData, dataOut.cbData);
            buf[dataOut.cbData] = '\0';
            result = buf.get();
        }
        LocalFree(dataOut.pbData);
        return success;
    }
} // namespace Crypto