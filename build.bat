echo Conan install
conan build . --profile=profiles/win32-msvc --build=missing || ( echo Conan failed & exit /b 1 )


if "%1"=="-c" (
    echo Copying DLL to EuroScope plugin folder...
    if not exist "%APPDATA%\EuroScope\UK\Data\Plugin\vSMR" mkdir "%APPDATA%\EuroScope\UK\Data\Plugin\vSMR"
    copy /Y build\Release\vSMR.dll "%APPDATA%\EuroScope\UK\Data\Plugin\vSMR" >nul
)

echo Done.
exit /b 0
