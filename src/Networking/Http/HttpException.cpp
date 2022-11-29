#include "HttpException.h"

HttpException::HttpException(std::uint32_t code, char const* error)
    : TcpException(error, false)
    , m_code(code)
{
}

HttpException::HttpException(std::uint32_t code, std::string const& error)
    : TcpException(error, false)
    , m_code(code)
{
}

std::uint32_t HttpException::GetCode() const {
    return m_code;
}