#include "PdfLibrary.h"

#include "PdfDocument.h"

#include <fpdfview.h>
#include <fpdf_ext.h>

#include <map>

namespace Pdf {

std::map<UNSUPPORT_INFO*, std::function<void(int)>> g_unsupportedFeautureHandlers;

void UnsupportedHandler(UNSUPPORT_INFO* info, int type) {
    auto found = g_unsupportedFeautureHandlers.find(info);
    if (found != g_unsupportedFeautureHandlers.end()) {
        found->second(type);
    }
}

struct UnsupportInfo {
    UnsupportInfo(std::function<void(int)>&& unsupportedFeatureCallback)
        : Info{1, UnsupportedHandler}
    {
        g_unsupportedFeautureHandlers.emplace(&Info, std::move(unsupportedFeatureCallback));
    }
    ~UnsupportInfo() {
        g_unsupportedFeautureHandlers.erase(&Info);
    }

    UNSUPPORT_INFO Info;
};

std::atomic<int> Library::s_refCounter{0};

Library::Library(std::function<void(int)>&& unsupportedFeatureCallback, IPasswordProviderUPtr passwordProvider)
    : m_unsupportInfo(new UnsupportInfo(std::move(unsupportedFeatureCallback)))
    , m_passwordProvider(std::move(passwordProvider))
{
    if (s_refCounter == 0) {
        FPDF_LIBRARY_CONFIG config {2, nullptr, nullptr, 0, nullptr};
        FPDF_InitLibraryWithConfig(&config);
        FSDK_SetUnSpObjProcessHandler(&m_unsupportInfo->Info);
    }
    s_refCounter++;
}

Library::~Library() {
    s_refCounter--;
    if (s_refCounter == 0) {
        FPDF_DestroyLibrary();
    }
}

DocumentUPtr Library::OpenDocument(std::string const& path) {
    DocumentUPtr document(new Document(path, *m_passwordProvider));
    if (*document) {
        return document;
    }
    return nullptr;
}

} // namespace Pdf