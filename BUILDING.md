# Build And Install

This project uses CMake. The repository also includes a Makefile wrapper for common commands.

## Dependencies

On Ubuntu/Debian:

```bash
sudo apt install gnucash gnucash-common libglib2.0-dev libcsv-dev cmake build-essential
```

Optional developer tooling:

```bash
sudo apt install clang-format
```

Runtime behavior depends on the installed GnuCash libraries. Backend URIs such as MySQL work only when that backend is available in the local GnuCash installation.

## Build With Make

Release build:

```bash
make build
```

Debug build:

```bash
make debug
```

The executable is written to:

```text
build/gnc-toolbox
```

Useful targets:

```bash
make test
make test-verbose
make test-unit
make format-check
make clean
make info
make package
```

## Build With CMake Directly

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Debug build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

## Install

Install the executable and documentation:

```bash
make install
```

The default install prefix is `/usr/local`, so the executable is installed as:

```text
/usr/local/bin/gnc-toolbox
```

Documentation, including the full AGPLv3 license text, is installed under:

```text
/usr/local/share/doc/gnc-toolbox/
```

The manual page is installed as:

```text
/usr/local/share/man/man1/gnc-toolbox.1
```

Install somewhere else:

```bash
make install PREFIX=$HOME/.local
```

Equivalent direct CMake commands:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build build
cmake --install build
```

## Staged Install For Packaging

Use `DESTDIR` when preparing a package root:

```bash
DESTDIR=/tmp/gnc-toolbox-root make install PREFIX=/usr
```

This stages files under:

```text
/tmp/gnc-toolbox-root/usr/bin/gnc-toolbox
/tmp/gnc-toolbox-root/usr/share/doc/gnc-toolbox/
/tmp/gnc-toolbox-root/usr/share/man/man1/gnc-toolbox.1
```

## Uninstall

The build writes an install manifest. After installing from the same build directory, uninstall with:

```bash
make uninstall
```

For a staged install:

```bash
DESTDIR=/tmp/gnc-toolbox-root make uninstall PREFIX=/usr
```

The uninstall target removes files listed in CMake's `install_manifest.txt`. It does not remove parent directories that may contain other files.

## Packages

Create source and binary `.tar.gz` packages plus checksums:

```bash
make package
```

The generated archives are written under `build/package/`.

See [PACKAGING.md](PACKAGING.md) for package contents, staged install details, and the release checklist.
