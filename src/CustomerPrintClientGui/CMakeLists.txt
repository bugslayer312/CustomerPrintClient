file(READ "Version.h" APP_VER)
string(REGEX MATCH "APP_MAJOR_VERSION[ \t]+([0-9]*)" _ ${APP_VER})
set(APP_VER_MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "APP_MINOR_VERSION[ \t]+([0-9]*)" _ ${APP_VER})
set(APP_VER_MINOR ${CMAKE_MATCH_1})
string(REGEX MATCH "APP_RELEASE_NUMBER[ \t]+([0-9]*)" _ ${APP_VER})
set(APP_VER_RELEASE ${CMAKE_MATCH_1})
string(REGEX MATCH "APP_SUBRELEASE_NUMBER[ \t]+([0-9]*)" _ ${APP_VER})
set(APP_VER_SUBRELEASE ${CMAKE_MATCH_1})
set(BUILD_VERSION ${APP_VER_MAJOR}.${APP_VER_MINOR}.${APP_VER_RELEASE}.${APP_VER_SUBRELEASE})
message("BUILD VERSION: ${BUILD_VERSION}")
message(STATUS "Generating file Version.rc")
configure_file("Version.rc.template" "${CMAKE_CURRENT_SOURCE_DIR}/Generated/Version.rc" @ONLY)

set(GUI_RES_FILES
    Gui.rc
)

add_executable(RemotePrint WIN32
    AsyncResultQueue.cpp
    AsyncResultQueue.h
    CtrlWithToolBar.cpp
    CtrlWithToolBar.h
    DlgLogin.cpp
    DlgLogin.h
    JobsPage/CtrlJobDetails.cpp
    JobsPage/CtrlJobDetails.h
    JobsPage/CtrlPendingJob.cpp
    JobsPage/CtrlPendingJob.h
    JobsPage/DlgJobCard.cpp
    JobsPage/DlgJobCard.h
    JobsPage/PageJobsList.cpp
    JobsPage/PageJobsList.h
    JobsPage/Styles.cpp
    JobsPage/Styles.h
    main.cpp
    MainWindow.cpp
    MainWindow.h
    MapPage/CtrlRating.cpp
    MapPage/CtrlRating.h
    MapPage/DlgOfficeCard.cpp
    MapPage/DlgOfficeCard.h
    MapPage/MapView.cpp
    MapPage/MapView.h
    PrintPage/CtrlCloseHeader.cpp
    PrintPage/CtrlCloseHeader.h
    PrintPage/CtrlPageAlignPopup.cpp
    PrintPage/CtrlPageAlignPopup.h
    PrintPage/CtrlPrintPreviewCanvas.cpp
    PrintPage/CtrlPrintPreviewCanvas.h
    PrintPage/CtrlPrintProfiles.cpp
    PrintPage/CtrlPrintProfiles.h
    PrintPage/PagePrintContent.cpp
    PrintPage/PagePrintContent.h
    PrintPage/PanelBrowseFile.cpp
    PrintPage/PanelBrowseFile.h
    PrintPage/PanelPrintPreview.cpp
    PrintPage/PanelPrintPreview.h
    StringResources.cpp
    StringResources.h
    Version.h
    wxUtilities.cpp
    wxUtilities.h
    ${GUI_RES_FILES}
)

target_link_libraries(RemotePrint
    PRIVATE RemotePrint::ClientCore
    # PRIVATE RemotePrint::Printing
    PRIVATE RemotePrint::Resources
    PRIVATE ${WXLIB_WEBVIEW}
    PRIVATE ${WXLIB_CORE}
    PRIVATE ${WXLIB_BASE}
    PRIVATE ${WXLIB_PNG}
    PRIVATE ${WXLIB_ZLIB}
    PRIVATE ${WXLIB_REGEX}
    PRIVATE ${WX_DEPENDENCIES_LIBS}
)

add_custom_command(TARGET RemotePrint POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${PDFIUM_BIN} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/.
)