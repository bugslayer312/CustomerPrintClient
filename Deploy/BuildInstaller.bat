@echo OFF

set CONFIG=Release
set APP_NAME=TeamPrinter
set DO_SIGN=OFF

:: Read command line arguments
for %%x in (%*) do (
  for /F "tokens=1,2 delims=: " %%a in ("%%x") do (
    if "%%a" == "/app" (
      if NOT "%%b" == "/" (
        set APP_NAME=%%~b
      )
    )
    if "%%a" == "/config" (
      if NOT "%%b" == "/" (
        set CONFIG=%%~b
      )
    )
    if "%%a" == "/sign" (
      if NOT "%%b" == "/" (
        set DO_SIGN=%%~b
      )
    )
  )
)

echo -- Building installer. app:%APP_NAME% config:%CONFIG% sign:%DO_SIGN%

:: For code signing
set SIGNTOOL="C:\Program Files (x86)\Windows Kits\10\App Certification Kit\signtool.exe"
set SHA1_THUMBPRINT=56db8c975850740d3dd71a25ceb6f13bd3f71968
set SHA256_THUMBPRINT=27cbeec245010d0c4ac061203b8760cba6a9664e
set TIMESTAMP_SERVER=http://timestamp.digicert.com

:: Check if SSL is compiled as shared
set LIB_CRYPTO_X86_FILENAME=libcrypto-1_1.dll
set LIB_CRYPTO_X64_FILENAME=libcrypto-1_1-x64.dll

if exist ..\output\x86\%CONFIG%\bin\%LIB_CRYPTO_X86_FILENAME% (
  set LIB_CRYPTO_X86=%LIB_CRYPTO_X86_FILENAME%
  echo -- found shared ssl lib ^(x86^)^: %LIB_CRYPTO_X86%
)
if exist ..\output\x64\%CONFIG%\bin\%LIB_CRYPTO_X64_FILENAME% (
  set LIB_CRYPTO_X64=%LIB_CRYPTO_X64_FILENAME%
  echo -- found shared ssl lib ^(x64^)^: %LIB_CRYPTO_X64%
)

:: Sign binaries
if %DO_SIGN% == ON (
  echo -- Signing binaries
  for %%i in (..\output\x86\%CONFIG%\bin\pdfium.dll ^
              ..\output\x86\%CONFIG%\bin\TeamPrinter.exe ^
              ..\output\x64\%CONFIG%\bin\pdfium.dll ^
              ..\output\x64\%CONFIG%\bin\TeamPrinter.exe) do (
    %SIGNTOOL% sign /t %TIMESTAMP_SERVER% /sha1 %SHA1_THUMBPRINT% /sm /v %%i
    %SIGNTOOL% sign /tr %TIMESTAMP_SERVER% /td sha256 /fd sha256 /as /sha1 %SHA256_THUMBPRINT% /sm /v %%i
  )
  if not "%LIB_CRYPTO_X86%" == "" (
    %SIGNTOOL% sign /t %TIMESTAMP_SERVER% /sha1 %SHA1_THUMBPRINT% /sm /v ..\output\x86\%CONFIG%\bin\%LIB_CRYPTO_X86%
    %SIGNTOOL% sign /tr %TIMESTAMP_SERVER% /td sha256 /fd sha256 /as /sha1 %SHA256_THUMBPRINT% /sm /v ..\output\x86\%CONFIG%\bin\%LIB_CRYPTO_X86%
  )
  if not "%LIB_CRYPTO_X64%" == "" (
    %SIGNTOOL% sign /t %TIMESTAMP_SERVER% /sha1 %SHA1_THUMBPRINT% /sm /v ..\output\x64\%CONFIG%\bin\%LIB_CRYPTO_X64%
    %SIGNTOOL% sign /tr %TIMESTAMP_SERVER% /td sha256 /fd sha256 /as /sha1 %SHA256_THUMBPRINT% /sm /v ..\output\x64\%CONFIG%\bin\%LIB_CRYPTO_X64%
  )
)

:: Recreate Output directory
if exist Output (
  del Output\*.* /Q
) else (
  md Output
)
set WIX_OUT=Output\WixOut
set CMAKE_OUT=Output\CMakeOut
md %WIX_OUT%
md %CMAKE_OUT%

:: Generating installer files
echo -- Generating installer files
cmake -S . -B %CMAKE_OUT% -DPRODUCT=%APP_NAME% -DCONFIGURATION=%CONFIG% -DLIB_CRYPTO_X86=%LIB_CRYPTO_X86% -DLIB_CRYPTO_X64=%LIB_CRYPTO_X64%
set /p BOOTSTRAPPER_NAME=<%CMAKE_OUT%\%APP_NAME%.outputfiles

:: Wix project files
set WIX_TARGET_X86=%CMAKE_OUT%\%APP_NAME%32.wxs
set WIX_TARGET_X64=%CMAKE_OUT%\%APP_NAME%64.wxs
set WIX_TARGET_BUNDLE=%CMAKE_OUT%\%APP_NAME%Bundle.wxs
set WIX_UI_WIZARD_SRC=res\WixUI_CustomFeatureTree.wxs
set WIX_UI_WIZARD_X86_OBJ=%WIX_OUT%\WixUI_CustomFeatureTree32.wixobj
set WIX_UI_WIZARD_X64_OBJ=%WIX_OUT%\WixUI_CustomFeatureTree64.wixobj
set WIX_TARGET_X86_OBJ=%WIX_OUT%\%APP_NAME%32.wixobj
set WIX_TARGET_X64_OBJ=%WIX_OUT%\%APP_NAME%64.wixobj

:: Wix toolset
set CANDLE_EXE="%WIX%bin\candle.exe"
set LIGHT_EXE="%WIX%bin\light.exe"
set INSIGNIA="%WIX%bin\insignia.exe"

:: compile wizard ui
echo -- Creating x86 msi
%CANDLE_EXE% %WIX_UI_WIZARD_SRC% -out %WIX_UI_WIZARD_X86_OBJ% -ext WixUIExtension
%CANDLE_EXE% %WIX_TARGET_X86% -out %WIX_TARGET_X86_OBJ% -ext WixUIExtension -ext WixUtilExtension
%LIGHT_EXE% %WIX_TARGET_X86_OBJ% %WIX_UI_WIZARD_X86_OBJ% -out Output\%APP_NAME%32.msi -pdbout %WIX_OUT%\%APP_NAME%32.wixpdb ^
    -ext WixUIExtension -ext WixUtilExtension -cultures:en-us -sice:ICE03

:: create x64 installer
echo -- Creating x64 msi
%CANDLE_EXE% %WIX_UI_WIZARD_SRC% -arch x64 -out %WIX_UI_WIZARD_X64_OBJ% -ext WixUIExtension
%CANDLE_EXE% %WIX_TARGET_X64% -arch x64 -out %WIX_TARGET_X64_OBJ% -ext WixUIExtension -ext WixUtilExtension
%LIGHT_EXE% %WIX_TARGET_X64_OBJ% %WIX_UI_WIZARD_X64_OBJ% -out Output\%APP_NAME%64.msi -pdbout %WIX_OUT%\%APP_NAME%64.wixpdb ^
    -ext WixUIExtension -ext WixUtilExtension -cultures:en-us -sice:ICE03

:: sign msi
if %DO_SIGN% == ON (
  echo -- Signing msi
  %SIGNTOOL% sign /t %TIMESTAMP_SERVER% /sha1 %SHA1_THUMBPRINT% /sm /v Output\%APP_NAME%32.msi
  %SIGNTOOL% sign /tr %TIMESTAMP_SERVER% /td sha256 /fd sha256 /sha1 %SHA256_THUMBPRINT% /sm /v Output\%APP_NAME%64.msi
)

:: create BootStrapper
echo -- Creating bootstrapper
%CANDLE_EXE% %WIX_TARGET_BUNDLE% -out %WIX_OUT%\%APP_NAME%Bundle.wixobj -arch x86 -ext WixBalExtension -ext WixUtilExtension
%LIGHT_EXE% %WIX_OUT%\%APP_NAME%Bundle.wixobj -out Output\%BOOTSTRAPPER_NAME% -pdbout %WIX_OUT%\%APP_NAME%Bundle.wixpdb ^
    -ext WixBalExtension -ext WixUtilExtension -cultures:en-us

:: sign bootstrapper
if %DO_SIGN% == ON (
  echo -- Insignia: Detach engine.exe
  %INSIGNIA% -ib Output\%BOOTSTRAPPER_NAME% -nologo -o Output\engine.exe

  echo -- SignTool: Sign engine.exe
  %SIGNTOOL% sign /t %TIMESTAMP_SERVER% /sha1 %SHA1_THUMBPRINT% /sm /v Output\engine.exe
  %SIGNTOOL% sign /tr %TIMESTAMP_SERVER% /td sha256 /fd sha256 /as /sha1 %SHA256_THUMBPRINT% /sm /v Output\engine.exe

  echo -- Insignia: Attach engine.exe
  md Output\Unsigned
  move Output\%BOOTSTRAPPER_NAME% Output\Unsigned\
  %INSIGNIA% -ab Output\engine.exe Output\Unsigned\%BOOTSTRAPPER_NAME% -nologo -o Output\%BOOTSTRAPPER_NAME%
  rd  /s /q Output\Unsigned
  del /q Output\engine.exe

  echo -- SignTool: Sign bootstrapper
  %SIGNTOOL% sign /t %TIMESTAMP_SERVER% /sha1 %SHA1_THUMBPRINT% /sm /v Output\%BOOTSTRAPPER_NAME%
  %SIGNTOOL% sign /tr %TIMESTAMP_SERVER% /td sha256 /fd sha256 /as /sha1 %SHA256_THUMBPRINT% /sm /v Output\%BOOTSTRAPPER_NAME%
)