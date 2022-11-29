include("${CMAKE_SOURCE_DIR}/cmake/CMakeRC.cmake")

cmrc_add_resource_library(Resources
    ALIAS RemotePrint::Resources
    NAMESPACE Res

    web/MapView.html
    web/office-marker-mail.png
    web/office-marker-normal.png
    web/office-marker-selected.png
)