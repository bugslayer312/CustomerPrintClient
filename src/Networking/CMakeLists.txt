add_library(TPNetworking STATIC
    Http/HttpClient.cpp
    Http/HttpClient.h
    Http/HttpClientMT.cpp
    Http/HttpClientMT.h
    Http/HttpContent.cpp
    Http/HttpContent.h
    Http/HttpDefinitions.cpp
    Http/HttpDefinitions.h
    Http/HttpException.cpp
    Http/HttpException.h
    Http/HttpRequest.cpp
    Http/HttpRequest.h
    Http/HttpResponse.cpp
    Http/HttpResponse.h
    Http/TcpException.cpp
    Http/TcpException.h
    StringResources.cpp
    StringResources.h
)
add_library(RemotePrint::Networking ALIAS TPNetworking)

target_link_libraries(TPNetworking
    PRIVATE RemotePrint::Core
    ${OPENSSL_LIBS}
)

if (WIN32)
    target_include_directories(TPNetworking PUBLIC ${OPENSSL_INCLUDE_DIR})
endif()