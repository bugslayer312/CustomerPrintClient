add_executable(TestPrint WIN32
    main.cpp
    MainWindow.cpp
    MainWindow.h
    wxUtilities.cpp
    wxUtilities.h
)

target_link_libraries(TestPrint
    PRIVATE RemotePrint::Core
    PRIVATE RemotePrint::Printing
    PRIVATE ${WXLIB_CORE}
    PRIVATE ${WXLIB_BASE}
    PRIVATE ${WXLIB_PNG}
    PRIVATE ${WXLIB_JPEG}
    PRIVATE ${WXLIB_ZLIB}
)
