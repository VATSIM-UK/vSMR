from conan import ConanFile
from conan.tools.cmake import cmake_layout

class vSMR(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain" , "CMakeDeps"

    def requirements(self):
        self.requires("rapidjson/1.1.0")

    def build_requirements(self):
        self.tool_requires("cmake/[>3.20.0]")

    def layout(self):
        cmake_layout(self)
        
    
