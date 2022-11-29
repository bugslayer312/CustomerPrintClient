set(CORE_SOURCES
    BinaryVersion.cpp
    BinaryVersion.h
    Config.cpp
    Config.h
    CredentialsCache.cpp
    CredentialsCache.h
    Crypto.h
    Format.cpp
    Format.h
    GoogleApiKey.h
    jsonxx.cc
    jsonxx.h
    Log.h
    OS.h
    StringUtilities.cpp
    StringUtilities.h
)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    list(APPEND CORE_SOURCES
        Crypto_Windows.cpp
        Log_Windows.cpp
        OS_Windows.cpp
    )
    set(CORE_DEP_LIBS
        crypt32.lib
        LocationAPI.lib
    )
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    list(APPEND CORE_SOURCES
        Crypto_Linux.cpp
        Log_Linux.cpp
        OS_Linux.cpp
    )
endif()

add_library(Core
    ${CORE_SOURCES}
)
add_library(RemotePrint::Core ALIAS Core)

target_link_libraries(Core
    PRIVATE ${Boost_FILESYSTEM_LIBRARY}
    ${CORE_DEP_LIBS}
)