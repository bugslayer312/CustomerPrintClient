#include "../../Core/OS.h"
#include "../../Core/Log.h"
#include "../../Core/BinaryVersion.h"
#include "../../CustomerPrintClientCore/CoreManager.h"
// #include "../../Printing/GdiplusUtilities.h"

#include <iostream>
#include <fstream>


int main(char const* argv[], int argc) {
    std::cout << "TestConsole start..." << std::endl;
    OS::Version osVer = OS::GetVersion();
    std::cout << "OS: " << osVer.Major << '.' << osVer.Minor << std::endl;
    std::wcout << "Downloads folder: " << OS::GetDownloadsDir() << std::endl;
    BinaryVersion curVer{1, 0, 0, 0};
    /*IAsyncResultPtr result = GetCoreManager().AsyncAskForUpdate(curVer, [](std::shared_ptr<std::wstring> result){
        std::wcout << *result << std::endl;
    });
    result->ExecCallback(); */
    
    return 0;
}