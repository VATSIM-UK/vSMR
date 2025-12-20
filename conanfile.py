from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout
from conan.tools.cmake import CMakeToolchain
from conan.tools.files import copy
import os


class vSMR(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        self.requires("rapidjson/1.1.0")

    def build_requirements(self):
        self.tool_requires("cmake/[>3.20.0]")
        self.tool_requires("ninja/[>=1.13]")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        toolchain = CMakeToolchain(self)
        toolchain.variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = "ON"
        toolchain.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

        compile_commands = os.path.join(
            self.build_folder, "compile_commands.json")
        if os.path.exists(compile_commands):
            copy(self, "compile_commands.json",
                 src=self.build_folder, dst=self.source_folder)
