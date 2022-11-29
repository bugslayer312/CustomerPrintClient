add_executable(PdfPrint WIN32
    main.cpp
    MainWindow.cpp
    MainWindow.h
    wxUtilities.cpp
    wxUtilities.h
)

target_link_libraries(PdfPrint
    PRIVATE RemotePrint::Core
    PRIVATE RemotePrint::Printing
    PRIVATE ${WXLIB_CORE}
    PRIVATE ${WXLIB_BASE}
    PRIVATE ${WXLIB_PNG}
    PRIVATE ${WXLIB_ZLIB}
    PRIVATE ${PDFIUM_LIB}
)
