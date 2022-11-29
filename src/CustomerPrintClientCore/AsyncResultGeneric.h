#pragma once

#include "IAsyncResult.h"

#include <future>
#include <functional>
#include <chrono>

int const AsyncResultWaitTimeoutMs = 50;

template<class T>
class AsyncResultGeneric : public IAsyncResult {
public:
    AsyncResultGeneric(std::future<T>&& fut, std::function<void(T)> callback)
        : m_future(std::move(fut))
        , m_callback(callback)
    {
    }

    virtual ~AsyncResultGeneric() override = default;
    virtual bool IsReady() const override {
        return m_future.wait_for(std::chrono::milliseconds(AsyncResultWaitTimeoutMs)) == std::future_status::ready;
    }

    virtual void ExecCallback() override {
        m_callback(m_future.get());
    }

private:
    std::future<T> m_future;
    std::function<void(T)> m_callback;
};

template<>
void AsyncResultGeneric<void>::ExecCallback() {
    m_future.get();
    m_callback();
}

