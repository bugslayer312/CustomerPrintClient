#pragma once

#include <memory>

class IAsyncResult {
public:
    virtual ~IAsyncResult() = default;
    virtual bool IsReady() const = 0;
    virtual void ExecCallback() = 0;
};
typedef std::shared_ptr<IAsyncResult> IAsyncResultPtr;