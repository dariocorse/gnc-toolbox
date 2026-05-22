# Packaging

This project uses CPack through CMake. Packaging currently supports portable `.tar.gz` archives:

- a source archive for release uploads,
- a binary archive containing the installed executable and documentation,
- a `SHA256SUMS` file for generated archives.

Distribution-specific packages such as `.deb` or `.rpm` are intentionally not enabled yet. The project license is AGPLv3 or later; distro package metadata and release policy can be added when those package formats are introduced.

## Create Packages

From a clean checkout:

```bash
make package
```

This creates:

```text
build/package/gnc-toolbox-<version>-source.tar.gz
build/package/gnc-toolbox-<version>-<system>-<arch>.tar.gz
build/package/SHA256SUMS
```

Create only the source package:

```bash
make package-source
```

Create only the binary package:

```bash
make package-binary
```

Regenerate checksums:

```bash
make package-checksums
```

## Source Archive

The source archive is intended for GitHub releases and downstream rebuilds. It includes:

- source files,
- headers,
- tests and test data,
- examples,
- CMake and Make build files,
- user and contributor documentation,
- GitHub workflow and issue template files,
- the AGPLv3 license text.

It excludes generated build output, local editor/tool state, local scratch test files, and package archives.

## Binary Archive

The binary archive contains the same files that `cmake --install` would install:

```text
bin/gnc-toolbox
share/doc/gnc-toolbox/
share/man/man1/gnc-toolbox.1
```

The documentation directory includes `LICENSE` with the full AGPLv3 license text and `examples/` with sample CSV/rule files.

The binary archive is platform-specific. It depends on compatible installed GnuCash, GLib, and libcsv runtime libraries on the target system.

## Staged Install

For distro packaging tools that expect a package root, use `DESTDIR`:

```bash
DESTDIR=/tmp/gnc-toolbox-root make install PREFIX=/usr
```

This installs under:

```text
/tmp/gnc-toolbox-root/usr/bin/gnc-toolbox
/tmp/gnc-toolbox-root/usr/share/doc/gnc-toolbox/
/tmp/gnc-toolbox-root/usr/share/man/man1/gnc-toolbox.1
```

## Release Checklist

Before attaching packages to a public release:

1. Run `make clean`.
2. Run `make test`.
3. Run `make package`.
4. Inspect `build/package/SHA256SUMS`.
5. Extract the source archive and confirm it builds.
6. Extract the binary archive and confirm `bin/gnc-toolbox --version` works on the target system.
7. Confirm the license file, source headers, package metadata, and release notes are up to date.
