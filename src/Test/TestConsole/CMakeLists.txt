add_executable(TestConsole
    main.cpp
)

target_link_libraries(TestConsole
    PRIVATE RemotePrint::Core
    PRIVATE RemotePrint::Printing
    PRIVATE RemotePrint::ClientCore
    gdiplus.lib
)