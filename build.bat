conan install . -s arch=x86 -s build_type=Release --build=missing --output-folder=build
cmake --preset conan-default
cmake --build --preset conan-release