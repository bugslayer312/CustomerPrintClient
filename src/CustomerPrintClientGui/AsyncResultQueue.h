#pragma once

#include "../CustomerPrintClientCore/IAsyncResult.h"
#include "../CustomerPrintClientCore/TitledException.h"

#include <mutex>
#include <list>
#include <functional>

class AsyncResultQueue {
protected:
    void StoreAsyncResult(IAsyncResultPtr asyncResult);
    void RetrieveAsyncResult(std::function<void(const char*, const char*, ExceptionCategory)> errorHandler);

private:
    std::mutex m_asyncResultMutex;
    std::list<IAsyncResultPtr> m_asyncResults;
};