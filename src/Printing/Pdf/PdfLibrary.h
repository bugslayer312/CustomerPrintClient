#pragma once

#include "IPdfPasswordProvider.h"

#include <functional>
#include <memory>
#include <atomic>
#include <string>

namespace Pdf {

struct UnsupportInfo;
class Document;
typedef std::unique_ptr<Document> DocumentUPtr;
typedef std::unique_ptr<IPasswordProvider> IPasswordProviderUPtr;

class Library {
public:
    Library(std::function<void(int)>&& unsupportedFeatureCallback, IPasswordProviderUPtr passwordProvider);
    ~Library();
    DocumentUPtr OpenDocument(std::string const& path);

private:
    std::function<void(int)> m_unsupportedFeatureCallback;
    IPasswordProviderUPtr m_passwordProvider;
    std::unique_ptr<UnsupportInfo> m_unsupportInfo;
    static std::atomic<int> s_refCounter;
};

} // namespace Pdf