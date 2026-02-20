from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMakeToolchain, CMakeDeps


class RMERecipe(ConanFile):
    name = "rme"
    version = "4.1.2"
    description = "Remere's Map Editor - OTAcademy Edition"
    license = "GPL-3.0"
    
    settings = "os", "compiler", "build_type", "arch"
    
    def requirements(self):
        # On Linux, most dependencies come from apt - only need glad from Conan
        # On other platforms, use full Conan dependency tree
        if self.settings.os == "Linux":
            # Only dependencies NOT available via apt
            self.requires("glad/0.1.36")
            self.requires("opengl/system")
            self.requires("tomlplusplus/3.4.0")
            # Note: nanovg is in ext/nanovg
        else:
            # Full dependency tree for Windows/macOS
            self.requires("wxwidgets/3.2.6")
            self.requires("asio/1.32.0")
            self.requires("nlohmann_json/3.11.3")
            self.requires("libarchive/3.7.7")
            self.requires("boost/1.87.0")
            self.requires("zlib/1.3.1")
            self.requires("opengl/system")
            self.requires("glad/0.1.36")
            self.requires("glm/1.0.1")
            self.requires("spdlog/1.15.0")

        # Lua dependencies for all platforms (unless Linux has them in apt, but let's be safe)
        # Upstream uses Lua 5.x and Sol2
        self.requires("lua/5.4.6")
        self.requires("sol2/3.3.1")
        # cpr is used for HTTP requests in Lua API
        self.requires("cpr/1.10.5")
    
    def layout(self):
        cmake_layout(self)
    
    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        
        tc = CMakeToolchain(self, generator="Ninja")
        tc.cache_variables["CMAKE_CXX_STANDARD"] = "20"
        tc.cache_variables["CMAKE_CXX_STANDARD_REQUIRED"] = "ON"
        # Ensure Unicode mode on Windows
        tc.preprocessor_definitions["UNICODE"] = ""
        tc.preprocessor_definitions["_UNICODE"] = ""
        tc.generate()
    
    def configure(self):
        self.options["glad/*"].gl_profile = "core"
        self.options["glad/*"].gl_version = "4.6"
        self.options["cpr/*"].with_ssl = True # Enable SSL for secure HTTPS requests

        if self.settings.os != "Linux":
            # Boost components needed
            self.options["boost/*"].without_python = True
            self.options["boost/*"].without_test = True
            
            # wxWidgets components needed
            self.options["wxwidgets/*"].opengl = True
            self.options["wxwidgets/*"].aui = True
            self.options["wxwidgets/*"].html = True
            self.options["wxwidgets/*"].unicode = True
