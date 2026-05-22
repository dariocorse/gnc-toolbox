# Contributing

This project is a small C command-line tool built with CMake. Contributions should keep the import behavior predictable, preserve existing user data, and include tests for behavior changes.

By contributing code, tests, or documentation, you agree that your contribution is provided under the project's license: GNU AGPLv3 or later (`AGPL-3.0-or-later`).

## Development Setup

Install the same dependencies used for normal builds:

```bash
sudo apt install libgnucash-dev libglib2.0-dev libcsv-dev cmake build-essential
```

Build and test:

```bash
make debug
make test
```

The main executable is:

```text
build/gnc-toolbox
```

The test executable is:

```text
build/test_gnc-toolbox
```

## Before Changing Behavior

Check the user-facing documentation first:

- [README.md](README.md) for the public overview.
- [BUILDING.md](BUILDING.md) for build, install, uninstall, and packaging behavior.
- [PACKAGING.md](PACKAGING.md) for release archive expectations.
- [USAGE.md](USAGE.md) for command and rule-file behavior.
- [TESTING.md](TESTING.md) for test-suite structure.

If code behavior changes, update the relevant documentation in the same change. If documentation and implementation disagree, treat it as a bug unless the documentation is clearly outdated.

## Code Style

- C99.
- Keep changes focused on the requested behavior.
- Prefer existing helper functions and local patterns over new abstractions.
- Keep user-facing errors specific: include the file, row, field, account, or command option when that context is available.
- Do not silently route unmatched imports to an automatic imbalance account. Use the explicit `UNMATCHED,<account>` rule.

## Tests

Run the full suite before submitting a change:

```bash
make test
```

Useful targeted commands:

```bash
make test-unit
./run_tests.sh specific '/csv_params/*'
./run_tests.sh specific '/book_integration/*'
```

Check formatting when touching C sources:

```bash
make format-check
```

Apply repository formatting:

```bash
make format
```

Add or update tests when changing:

- rule-file parsing,
- CSV parsing,
- date or amount parsing,
- matching/filter behavior,
- unmatched transaction behavior,
- duplicate detection,
- command-line validation,
- GnuCash book read/write behavior.

## Documentation Changes

Public-facing behavior belongs in [USAGE.md](USAGE.md). The README should stay short and point readers to the detailed guide.

When adding a new rule-file token or command option, document:

- the syntax,
- the default behavior,
- a short example,
- the failure mode when input is invalid.

## Safety Expectations

Changes that write to GnuCash books should be conservative:

- keep `--dry-run` behavior intact,
- avoid implicit account selection,
- fail clearly when configured accounts are missing,
- preserve manual-review workflows for unmatched transactions,
- keep duplicate handling explicit through `--no-duplicates`.
