#pragma once

#include <string>

namespace Pdf {

class IPasswordProvider {
public:
    virtual ~IPasswordProvider() = default;
    virtual std::string const& GetPassword() const = 0;
    virtual bool RequestPassword() = 0;
};

} // namespace Pdf