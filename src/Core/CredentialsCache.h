#pragma once

#include <string>

struct Credentials {
    std::string Login;
    std::string Password;
};

class CredentialsCache {
public:
    CredentialsCache(std::string const& path);
    bool GetCredentials(Credentials& creds) const;
    void SetCredentials(Credentials const& creds);
    void ClearCredentials();
private:
    std::string const m_path;
};