add_library(TPPrinting STATIC
    IDocumentRenderer.h
    ImageBundleRenderer.cpp
    ImageBundleRenderer.h
    ImageFormat.cpp
    ImageFormat.h
    IPreviewRenderer.h
    GdiplusUtilities.cpp
    GdiplusUtilities.h
    HBitmapPtr.cpp
    HBitmapPtr.h
    PdfDocumentRenderer.cpp
    PdfDocumentRenderer.h
    Pdf/IPdfPasswordProvider.h
    Pdf/PdfDocument.cpp
    Pdf/PdfDocument.h
    Pdf/PdfError.cpp
    Pdf/PdfError.h
    Pdf/PdfLibrary.cpp
    Pdf/PdfLibrary.h
    Primitives.cpp
    Primitives.h
    PrintSettings.cpp
    PrintSettings.h
)
add_library(RemotePrint::Printing ALIAS TPPrinting)

target_link_libraries(TPPrinting
    PRIVATE RemotePrint::Core
    PRIVATE ${PDFIUM_LIB}
#    PRIVATE ${WXLIB_CORE}
#    PRIVATE ${WXLIB_BASE}
#    PRIVATE ${WXLIB_PNG}
#    PRIVATE ${WXLIB_JPEG}
#    PRIVATE ${WXLIB_TIFF}
#    PRIVATE ${WXLIB_ZLIB}
#    PRIVATE ${Boost_FILESYSTEM_LIBRARY}
)