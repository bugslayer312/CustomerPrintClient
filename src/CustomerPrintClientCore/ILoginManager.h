#pragma once

#include "Types.h"
#include "../CustomerPrintClientCore/IAsyncResult.h"

#include <string>

class ILoginManager {
public:
    virtual ~ILoginManager() = default;
    virtual IAsyncResultPtr AsyncLogin(std::string const& email, std::string const& pass, VoidCallback callback) = 0;
    virtual IAsyncResultPtr AsyncSignUp(std::string const& email, std::string const& pass, std::string const& name,
                                        VoidCallback callback) = 0;
    virtual IAsyncResultPtr AsyncResetPassword(std::string const& email, VoidCallback callback) = 0;
};