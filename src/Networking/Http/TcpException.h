#pragma once

#include <stdexcept>

class TcpException : public std::runtime_error {
public:
    TcpException(std::string const& whatArg, bool wasCancel);
    TcpException(char const* whatArg, bool wasCancel);
    bool WasCancel() const;

private:
    bool m_wasCancel;
};