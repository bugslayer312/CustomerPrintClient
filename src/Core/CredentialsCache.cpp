#include "CredentialsCache.h"
#include "Crypto.h"

#include <fstream>

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

CredentialsCache::CredentialsCache(std::string const& path)
    : m_path(path)
{
}

bool CredentialsCache::GetCredentials(Credentials& creds) const {
    std::ifstream ifs(m_path, std::ios::in|std::ios::binary);
    if (ifs) {
        std::string login;
        if (std::getline(ifs, login)) {
            Crypto::ByteStream bytes;
            char buf[64];
            std::size_t prevSize(0);
            while (ifs.read(buf, 64)) {
                bytes.resize(prevSize + 64);
                memcpy(bytes.data() + prevSize, buf, 64);
                prevSize = bytes.size();
            }
            if(std::size_t lastReadCnt = ifs.gcount()) {
                bytes.resize(prevSize + lastReadCnt);
                memcpy(bytes.data() + prevSize, buf, lastReadCnt);
            }
            std::string password;
            if (Crypto::DecryptString(bytes, password)) {
                creds.Login = login;
                creds.Password = password;
                return true;
            }
        }
    }
    return false;
}

void CredentialsCache::SetCredentials(Credentials const& creds) {
    Crypto::ByteStream bytes;
    if (Crypto::EncryptString(creds.Password, bytes)) {
        std::ofstream ofs(m_path, std::ios::out|std::ios::binary|std::ios::ate);
        ofs << creds.Login << std::endl;
        ofs.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }
}

void CredentialsCache::ClearCredentials() {
    fs::path path(m_path);
    if (fs::is_regular_file(m_path)) {
        fs::remove(path);
    }
}