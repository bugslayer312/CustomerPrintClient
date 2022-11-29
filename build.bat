@echo OFF

set PLATFORM=x64
set CONFIG=Release

echo %*

for %%x in (%*) do (
  for /F "tokens=1,2 delims=: " %%a in ("%%x") do (
    if "%%a" == "/p" (
      if NOT "%%b" == "/" (
        set PLATFORM=%%~b
      )
    )
    if "%%a" == "/c" (
      if NOT "%%b" == "/" (
        set CONFIG=%%~b
      )
    )
  )
)

echo Platform:%PLATFORM% Configuration:%CONFIG%

set BUILD_DIR="build64"
if "%PLATFORM%" == "x86" (
  set BUILD_DIR="build"
  set PLATFORM="Win32"
)

cmake.exe -G "Visual Studio 16 2019" -A %PLATFORM% -S . -B %BUILD_DIR%
cmake.exe --build %BUILD_DIR% --config %CONFIG% --parallel 4
