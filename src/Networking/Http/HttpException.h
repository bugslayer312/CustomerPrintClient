#pragma once

#include "TcpException.h"

class HttpException : public TcpException {
public:
    HttpException(std::uint32_t code, char const* error);
    HttpException(std::uint32_t code, std::string const& error);
    std::uint32_t GetCode() const;
private:
    std::uint32_t const m_code;
};