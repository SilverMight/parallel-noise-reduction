from conan import ConanFile


class Recipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"

    def layout(self):
        self.folders.generators = "conan"

    def configure(self):
        self.options['fftw/3.3.10'].threads = True

    def requirements(self):
        self.requires("fmt/11.0.2")
        self.requires("fftw/3.3.10")
        self.requires("bshoshany-thread-pool/5.0.0")
        pass

    def build_requirements(self):
        pass
