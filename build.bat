@echo off


if not exist build mkdir build

echo [1/3] Conan install
conan install . --profile=profiles/win32-msvc --build=missing || ( echo Conan failed & exit /b 1 )

echo [2/3] CMake configure
cmake --preset conan-default || ( echo CMake configure failed & exit /b 1 )

echo [3/3] CMake build (Release)
cmake --build --preset conan-release || ( echo CMake build failed & exit /b 1 )

if "%1"=="-c" (
    echo Copying DLL to EuroScope plugin folder...
    if not exist "%APPDATA%\EuroScope\UK\Data\Plugin\vSMR" mkdir "%APPDATA%\EuroScope\UK\Data\Plugin\vSMR"
    copy /Y build\Release\vSMR.dll "%APPDATA%\EuroScope\UK\Data\Plugin\vSMR" >nul
)

echo Done.
exit /b 0
