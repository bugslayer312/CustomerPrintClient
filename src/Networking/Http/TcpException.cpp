#include "TcpException.h"

TcpException::TcpException(std::string const& whatArg, bool wasCancel)
    : runtime_error(whatArg)
    , m_wasCancel(wasCancel)
{
}

TcpException::TcpException(char const* whatArg, bool wasCancel)
    : runtime_error(whatArg)
    , m_wasCancel(wasCancel)
{
}

bool TcpException::WasCancel() const {
    return m_wasCancel;
}