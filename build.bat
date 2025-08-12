conan install . -s arch=x86 -s compiler.cppstd=17 -s compiler.runtime=static -s build_type=Release --build=missing --output-folder=build
cmake --preset conan-default
cmake --build --preset conan-release