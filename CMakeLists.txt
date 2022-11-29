cmake_minimum_required(VERSION 3.10.0)
project(CustomerPrintClient)

# use Unicode
add_definitions(-DUNICODE -D_UNICODE)

set(BUILD_TYPE ${CMAKE_BUILD_TYPE})
if (NOT BUILD_TYPE)
    set(BUILD_TYPE "Release")
endif()

if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    set(PLATFORM "x64")
else()
    set(PLATFORM "x86")
endif()

if (${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
    set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/output/${PLATFORM}/${BUILD_TYPE}")
else()
    set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/output/${PLATFORM}/$<CONFIG>")
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin/$<0:>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin/$<0:>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}/lib/$<0:>")

set(CMAKE_CXX_STANDART 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (USE_WX_DEBUG)
    set(WX_D "d")
endif()

if (WIN32 AND CMAKE_SYSTEM_VERSION)
    set(ver ${CMAKE_SYSTEM_VERSION})
    string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
    string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
    # Check for Windows 10, b/c we'll need to convert to hex 'A'.
    if ("${verMajor}" MATCHES "10")
        set(verMajor "A")
        string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
    endif ("${verMajor}" MATCHES "10")
    # Remove all remaining '.' characters.
    string(REPLACE "." "" ver ${ver})
    # Prepend each digit with a zero.
    string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
    # set(${version} "0x${ver}")
    add_definitions(-D_WIN32_WINNT=0x${ver})
endif()

set(BOOST_ROOT "$ENV{BOOST_1_72_0}")
set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_USE_STATIC_LIBS ON)

add_definitions(-DBOOST_DATE_TIME_NO_LIB
                -DBOOST_ASIO_ENABLE_CANCELIO
                -DBOOST_ASIO_DISABLE_IOCP
                -DBOOST_REGEX_NO_LIB)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # wxWidgets
    set(WXWIDGETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/wxWidgets/3.1.3/wxMSW")
    if (${PLATFORM} STREQUAL "x64")
        set(WXWIDGETS_LIB_DIR "${WXWIDGETS_DIR}/lib/vc_x64_lib")
    else()
        set(WXWIDGETS_LIB_DIR "${WXWIDGETS_DIR}/lib/vc_lib")
    endif()

    include_directories("${WXWIDGETS_DIR}/include")
    include_directories("${WXWIDGETS_DIR}/include/msvc")

    set(WXLIB_BASE "${WXWIDGETS_LIB_DIR}/wxbase31u$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(WXLIB_NET "${WXWIDGETS_LIB_DIR}/wxbase31u$<$<CONFIG:Debug>:d>_net${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(WXLIB_CORE "${WXWIDGETS_LIB_DIR}/wxmsw31u$<$<CONFIG:Debug>:d>_core${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(WXLIB_PNG "${WXWIDGETS_LIB_DIR}/wxpng$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(WXLIB_JPEG "${WXWIDGETS_LIB_DIR}/wxjpeg$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(WXLIB_TIFF "${WXWIDGETS_LIB_DIR}/wxtiff$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(WXLIB_ZLIB "${WXWIDGETS_LIB_DIR}/wxzlib$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(WXLIB_WEBVIEW "${WXWIDGETS_LIB_DIR}/wxmsw31u$<$<CONFIG:Debug>:d>_webview${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(WXLIB_REGEX "${WXWIDGETS_LIB_DIR}/wxregexu$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}")

    add_definitions(
        -DwxNO_NET_LIB
        -DwxNO_XML_LIB
        -DwxNO_REGEX_LIB
    #    -DwxNO_ZLIB_LIB
        -DwxNO_JPEG_LIB
    #    -DwxNO_PNG_LIB
        -DwxNO_TIFF_LIB
        -DwxNO_STC_LIB
        -DwxNO_HTML_LIB
        -DwxNO_QA_LIB
        -DwxNO_XRC_LIB
        -DwxNO_AUI_LIB
        -DwxNO_PROPGRID_LIB
        -DwxNO_RIBBON_LIB
        -DwxNO_RICHTEXT_LIB
        -DwxNO_MEDIA_LIB
        -DwxNO_WEBVIEW_LIB
        -DwxNO_GL_LIB
    )

    # boost
    if (${PLATFORM} STREQUAL "x64")
        set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib64-msvc-14.2")
    else()
        set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib32-msvc-14.2")
    endif()

    # openssl
    set(OPENSSL_LIB_DIR "$ENV{OPENSSL_1_1_1D}/${PLATFORM}/lib")
    set(OPENSSL_LIBS
        "${OPENSSL_LIB_DIR}/libcrypto${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${OPENSSL_LIB_DIR}/libssl${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )
    set(OPENSSL_INCLUDE_DIR "$ENV{OPENSSL_1_1_1D}/${PLATFORM}/include")

    # pdfium
    set(PDFIUM_LIB "$ENV{PDFIUM}/${PLATFORM}/lib/pdfium${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(PDFIUM_BIN "$ENV{PDFIUM}/${PLATFORM}/bin/pdfium${CMAKE_SHARED_LIBRARY_SUFFIX}")
    include_directories("$ENV{PDFIUM}/include")


elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    # wxWidgets
    set(WXWIDGETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/wxWidgets/3.1.3/wxGTK")
    set(WXWIDGETS_LIB_DIR "${WXWIDGETS_DIR}/lib${WX_D}")

    include_directories("${WXWIDGETS_DIR}/include/wx-3.1")
    include_directories("${WXWIDGETS_LIB_DIR}/wx/include/gtk3-unicode-static-3.1")

    set(WXLIB_BASE "${WXWIDGETS_LIB_DIR}/libwx_baseu-3.1${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(WXLIB_NET "${WXWIDGETS_LIB_DIR}/libwx_baseu_net-3.1${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(WXLIB_CORE "${WXWIDGETS_LIB_DIR}/libwx_gtk3u_core-3.1${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(WXLIB_WEBVIEW "${WXWIDGETS_LIB_DIR}/libwx_gtk3u_webview-3.1${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(WXLIB_REGEX "${WXWIDGETS_LIB_DIR}/libwxregexu-3.1${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(WXLIB_PNG png)
    add_definitions(-D_FILE_OFFSET_BITS=64 -D__WXGTK__)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    set(WX_DEPENDENCIES_LIBS
        X11
        Xxf86vm
        SM
        gtk-3
        gdk-3
        pangocairo-1.0
        cairo-gobject
        cairo
        gdk_pixbuf-2.0
        gio-2.0
        pangoft2-1.0
        pango-1.0
        gobject-2.0
        glib-2.0
        fontconfig
        dl
        webkit2gtk-4.0
        javascriptcoregtk-4.0
    # uncomment if it will be required
        # gthread-2.0
        # atk-1.0
        # Xtst
        # freetype
        # expat
        # jpeg
        # tiff
        # jbig
        # z
        # lzma
        # m
    )

    # boost
    set(BOOST_LIBRARYDIR "$ENV{BOOST_LIBS_1_72_0}")

    # openssl
    set(OPENSSL_LIBS
        PRIVATE crypto
        PRIVATE ssl
    )
else()
    message(SEND_ERROR "Unsupported OS: ${CMAKE_SYSTEM_NAME}")
    return()
endif()

find_package(Boost 1.72.0 REQUIRED filesystem)
include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory(src/Resources)
add_subdirectory(src/Core)
add_subdirectory(src/Networking)
add_subdirectory(src/Printing)
add_subdirectory(src/CustomerPrintClientCore)
add_subdirectory(src/CustomerPrintClientGui)

if (BUILD_TEST_APPS)
    add_subdirectory(src/Test/TestPrintApp)
    add_subdirectory(src/Test/PdfRenderApp)
    add_subdirectory(src/Test/TestConsole)
endif()
