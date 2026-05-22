# Changelog

All notable user-facing changes should be documented here.

This project follows semantic versioning once public releases begin. Dates use `YYYY-MM-DD`.

## [1.0.0] - Unreleased

### Added

- CSV import workflow with configurable parsing, row filters, duplicate checks, and dry-run mode.
- Rule-based account matching with substring, shell-style pattern, and GLib regular expression support.
- Explicit `UNMATCHED,<account>` routing so manually reviewed transactions can still be imported.
- Merge workflow for copying missing transactions from one GnuCash book/account into another.
- User documentation, build/install documentation, packaging documentation, and example rule files.
- Source and binary `.tar.gz` packaging through CPack.

### License

- Licensed as GNU AGPLv3 or later (`AGPL-3.0-or-later`).

## Release Process

Before publishing a release:

1. Run `make clean`.
2. Run `make test`.
3. Run `make package`.
4. Extract the source archive and confirm it builds.
5. Extract the binary archive and confirm `bin/gnc-toolbox --version` works.
6. Update this changelog with the release date.
7. Attach the generated archives and `SHA256SUMS` to the GitHub release.
