from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import copy
import os


class VsmrRecipe(ConanFile):
    name = "vSMR"
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = (
        "CMakeLists.txt",
        "vSMR/*",
        "lib/*",
        "*.cur",
        "*.wav",
        "README.md",
    )

    requires = (
        "asio/1.28.2",
        "libcurl/8.9.1",
        "rapidjson/1.1.0",
    )
    default_options = {
        "libcurl/*:shared": False,
        "libcurl/*:with_ssl": "schannel",
        "libcurl/*:with_zlib": False,
        "libcurl/*:with_brotli": False,
        "libcurl/*:with_ldap": False,
        "libcurl/*:with_libidn": False,
    }

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self)
        tc.cache_variables["CMAKE_MFC_FLAG"] = "2"
        if str(self.settings.get_safe("arch")) == "x86":
            tc.variables["CMAKE_GENERATOR_PLATFORM"] = "Win32"
        tc.variables["CMAKE_BUILD_TYPE"] = str(self.settings.get_safe("build_type") or "Release")
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        for src in [self.build_folder, os.path.join(self.build_folder, "bin")]:
            if os.path.isdir(src):
                copy(self, pattern="*.dll", src=src, dst=os.path.join(self.package_folder, "bin"), keep_path=False)
        copy(self, pattern="LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))

    def package_info(self):
        self.cpp_info.bindirs = ["bin"]
        self.cpp_info.libdirs = []
        self.cpp_info.includedirs = []
