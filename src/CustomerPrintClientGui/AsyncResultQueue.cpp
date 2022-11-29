#include "AsyncResultQueue.h"

void AsyncResultQueue::StoreAsyncResult(IAsyncResultPtr asyncResult) {
    std::lock_guard<std::mutex> lock(m_asyncResultMutex);
    m_asyncResults.push_back(asyncResult);
}
    
void AsyncResultQueue::RetrieveAsyncResult(std::function<void(const char*, const char*, ExceptionCategory)> errorHandler) {
    std::lock_guard<std::mutex> lock(m_asyncResultMutex);
    
    try {
        for (auto it = m_asyncResults.begin(); it != m_asyncResults.end(); ++it) {
            IAsyncResultPtr asyncResult = *it;
            if (asyncResult->IsReady()) {
                m_asyncResults.erase(it);
                asyncResult->ExecCallback();
                return;
            }
        }
    }
    catch (TitledException& ex) {
        errorHandler(ex.GetTitle().c_str(), ex.what(), ex.GetCategory());
    }
    catch (std::exception& ex) {
        errorHandler("Error", ex.what(), ExceptionCategory::Undefined);
    }
}