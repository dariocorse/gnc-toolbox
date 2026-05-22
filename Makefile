# Convenience wrapper for the CMake-based gnc-toolbox project.

CMAKE ?= cmake
CTEST ?= ctest
BUILD_DIR ?= build
PACKAGE_DIR ?= $(BUILD_DIR)/package
PREFIX ?= /usr/local
BUILD_TYPE ?= Release
CLANG_FORMAT ?= clang-format

CMAKE_CONFIGURE_FLAGS ?=
CMAKE_BUILD_FLAGS ?=

FORMAT_FILES := $(wildcard *.c src/*.c inc/*.h tests/*.c tests/*.h)

.PHONY: all build configure debug release clean install uninstall test test-verbose test-unit format format-check info package package-source package-binary package-checksums help

all: build

configure:
	@echo "Configuring gnc-toolbox ($(BUILD_TYPE))..."
	@$(CMAKE) -S . -B $(BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX) \
		$(CMAKE_CONFIGURE_FLAGS)

build:
	@$(MAKE) configure BUILD_TYPE=Release
	@echo "Building gnc-toolbox..."
	@$(CMAKE) --build $(BUILD_DIR) --target gnc-toolbox $(CMAKE_BUILD_FLAGS)

debug:
	@$(MAKE) configure BUILD_TYPE=Debug
	@echo "Building gnc-toolbox (Debug)..."
	@$(CMAKE) --build $(BUILD_DIR) --target gnc-toolbox $(CMAKE_BUILD_FLAGS)

release:
	@$(MAKE) configure BUILD_TYPE=Release
	@echo "Building gnc-toolbox (Release)..."
	@$(CMAKE) --build $(BUILD_DIR) --target gnc-toolbox $(CMAKE_BUILD_FLAGS)

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)

install: release
	@echo "Installing gnc-toolbox to $(PREFIX)..."
	@DESTDIR="$(DESTDIR)" $(CMAKE) --install $(BUILD_DIR)

uninstall:
	@echo "Uninstalling gnc-toolbox from $(PREFIX)..."
	@DESTDIR="$(DESTDIR)" $(CMAKE) --build $(BUILD_DIR) --target uninstall

test:
	@$(MAKE) configure BUILD_TYPE=Debug
	@echo "Building tests..."
	@$(CMAKE) --build $(BUILD_DIR) --target test_gnc-toolbox $(CMAKE_BUILD_FLAGS)
	@echo "Running tests..."
	@$(CTEST) --test-dir $(BUILD_DIR) --output-on-failure

test-verbose:
	@$(MAKE) configure BUILD_TYPE=Debug
	@echo "Building tests..."
	@$(CMAKE) --build $(BUILD_DIR) --target test_gnc-toolbox $(CMAKE_BUILD_FLAGS)
	@echo "Running tests with verbose output..."
	@$(CTEST) --test-dir $(BUILD_DIR) -V

test-unit:
	@$(MAKE) configure BUILD_TYPE=Debug
	@echo "Building tests..."
	@$(CMAKE) --build $(BUILD_DIR) --target test_gnc-toolbox $(CMAKE_BUILD_FLAGS)
	@echo "Running test executable directly..."
	@$(BUILD_DIR)/test_gnc-toolbox

format:
	@echo "Formatting C sources with clang-format..."
	@command -v $(CLANG_FORMAT) >/dev/null 2>&1 || { echo "clang-format is required. Install it or set CLANG_FORMAT=/path/to/clang-format."; exit 127; }
	@$(CLANG_FORMAT) -i $(FORMAT_FILES)

format-check:
	@echo "Checking C source formatting with clang-format..."
	@command -v $(CLANG_FORMAT) >/dev/null 2>&1 || { echo "clang-format is required. Install it or set CLANG_FORMAT=/path/to/clang-format."; exit 127; }
	@$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_FILES)

info:
	@echo "=== Build Information ==="
	@echo "CMake: $$($(CMAKE) --version | head -n1)"
	@echo "C compiler: $$(which gcc || which clang || echo 'not found')"
	@echo "Build directory: $(BUILD_DIR)"
	@echo "Build type: $(BUILD_TYPE)"
	@echo "Install prefix: $(PREFIX)"
	@echo "Executable: $(BUILD_DIR)/gnc-toolbox"

package: package-source package-binary package-checksums

package-source:
	@$(MAKE) configure BUILD_TYPE=Release
	@echo "Creating source package..."
	@cd $(BUILD_DIR) && cpack --config CPackSourceConfig.cmake

package-binary: release
	@echo "Creating binary package..."
	@cd $(BUILD_DIR) && cpack --config CPackConfig.cmake

package-checksums:
	@echo "Creating package checksums..."
	@cd $(PACKAGE_DIR) && sha256sum *.tar.gz > SHA256SUMS

help:
	@echo "Available targets:"
	@echo "  make build                         Build Release binary"
	@echo "  make debug                         Build Debug binary"
	@echo "  make release                       Build Release binary"
	@echo "  make test                          Build Debug binary and run CTest"
	@echo "  make test-verbose                  Run CTest with verbose output"
	@echo "  make test-unit                     Run the test executable directly"
	@echo "  make format                        Format C sources with clang-format"
	@echo "  make format-check                  Check C source formatting"
	@echo "  make install PREFIX=/usr/local     Install binary and docs"
	@echo "  make uninstall PREFIX=/usr/local   Remove installed files from manifest"
	@echo "  make package                       Create source and binary packages"
	@echo "  make package-source                Create source archive"
	@echo "  make package-binary                Create binary archive"
	@echo "  make package-checksums             Create SHA256SUMS for package archives"
	@echo "  make clean                         Remove build artifacts"
	@echo "  make info                          Show build settings"
	@echo ""
	@echo "Variables:"
	@echo "  BUILD_DIR=build                    Build directory"
	@echo "  PACKAGE_DIR=build/package          Package output directory"
	@echo "  PREFIX=/usr/local                  Install prefix"
	@echo "  BUILD_TYPE=Release                 CMake build type for configure"
	@echo "  CLANG_FORMAT=clang-format          clang-format executable"
	@echo "  CMAKE_CONFIGURE_FLAGS=...          Extra CMake configure flags"
	@echo "  CMAKE_BUILD_FLAGS=...              Extra cmake --build flags"
	@echo ""
	@echo "Examples:"
	@echo "  make debug"
	@echo "  make test"
	@echo "  make install PREFIX=$$HOME/.local"
	@echo "  DESTDIR=/tmp/pkgroot make install PREFIX=/usr"
	@echo "  make package"
