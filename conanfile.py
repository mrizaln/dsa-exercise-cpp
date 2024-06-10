from conan import ConanFile

class Recipe(ConanFile):
    settings   = ["os", "compiler", "build_type", "arch"]
    generators = ["CMakeToolchain", "CMakeDeps"]
    requires   = ["fmt/10.2.1", "boost-ext-ut/1.1.9"]

    def layout(self):
        self.folders.generators = "conan"
