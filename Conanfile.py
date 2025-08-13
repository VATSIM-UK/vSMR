from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout


class vSMRConanFile(ConanFile):
    name = "vSMR"

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"
    exports_sources = "CMakeLists.txt", "src/*", "inc/*", "euroscope/*"

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        ...

    def build(self):
        cmake = CMake(self)
        cmake.configure(variables={"CMAKE_GENERATOR_PLATFORM": "Win32"})
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()


