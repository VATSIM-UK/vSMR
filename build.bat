conan install . --build=missing --profile=profiles/win32-msvc
cd build
cmake -G "Visual Studio 17 2022" -A Win32 ..
cmake --build . --config Release