add_library(TPClientCore STATIC
    AsyncResultGeneric.h
    CloudPrintClient.cpp
    CloudPrintClient.h
    ConvertPrintProfile.cpp
    ConvertPrintProfile.h
    CoreManager.cpp
    CoreManager.h
    IAsyncResult.h
    ILoginManager.h
    JobContentType.h
    JobProcessor.cpp
    JobProcessor.h
    PendingJobProgressCallback.h
    PrintJobInfo.cpp
    PrintJobInfo.h
    PrintOffice.cpp
    PrintOffice.h
    PrintProfile.cpp
    PrintProfile.h
    TitledException.cpp
    TitledException.h
    Types.h
    UpdateClient.cpp
    UpdateClient.h
    UserInfo.h
)
add_library(RemotePrint::ClientCore ALIAS TPClientCore)

target_link_libraries(TPClientCore
    PRIVATE RemotePrint::Networking
    PRIVATE RemotePrint::Printing
)