# Testing

This document describes the test suite currently present in the repository. The project uses the GLib testing framework, a single `test_gnc-toolbox` executable, and CTest integration through the build.

## Prerequisites

The test suite uses the same native dependencies as the application:

```bash
sudo apt install libgnucash-dev libglib2.0-dev libcsv-dev cmake build-essential
```

Some tests create temporary GnuCash books through the installed GnuCash backend. They should not require a real user book and should not modify repository sample data.

## Overview

The current tests cover:

- CSV rule-file parsing
- date parsing
- localized amount parsing
- CLI parsing, validation, and help/version output
- CSV helper message formatting
- utility helpers and output formatting
- GnuCash book-reference validation and lock detection
- save-session error handling hooks
- parser and matcher integration with sample data
- temporary real-book save/load/import/merge round-trips

The suite now includes temporary real-book integration tests in addition to parser-level integration coverage.

## Test Structure

```text
tests/
├── test_main.c
├── test_utilities.h/c
├── test_cli.h/c
├── test_csv_helper.h/c
├── test_date_parsing.h/c
├── test_number_parsing.h/c
├── test_csv_params.h/c
├── test_gnc_utilities.h/c
├── test_input_validation.h/c
├── test_integration.h/c
├── test_book_integration.h/c
└── data/
    ├── sample_transactions_us.csv
    ├── sample_transactions_eu.csv
    ├── sample_config_us.ini
    ├── sample_config_eu.ini
    └── malformed_data.csv
```

## Running Tests

### Common Commands

```bash
make test
make test-verbose
make test-unit
```

### Test Runner Script

The repository also ships `run_tests.sh`:

```bash
./run_tests.sh all
./run_tests.sh unit
./run_tests.sh ctest
./run_tests.sh validation
./run_tests.sh specific '/input_validation/*'
./run_tests.sh clean
```

`make test` is the default command to run before sharing a change.

### Formatting

When changing C sources, check formatting with:

```bash
make format-check
```

Apply formatting with:

```bash
make format
```

### Direct Execution

```bash
make debug
cd build
./test_gnc-toolbox
./test_gnc-toolbox --verbose
./test_gnc-toolbox -p '/input_validation/*'
./test_gnc-toolbox -p '/gnc_utilities/*'
./test_gnc-toolbox -p '/cli/parse_import_args'
./test_gnc-toolbox -p '/csv_helper/print_message_during_parse'
./test_gnc-toolbox -p /book_integration/import_pipeline_roundtrip
```

## Test Categories

### 1. CLI Tests (`test_cli.c`)

These tests cover:

- no-argument behavior
- global help and version flags
- `import` and `merge` argument parsing
- rejection of invalid subcommands and oversized arguments
- argument validation for import and merge
- help and version output text

### 2. Input Validation Tests (`test_input_validation.c`)

These tests cover:

- file existence, readability, and writability checks
- directory validation
- GnuCash book-reference validation for local files, `file://` URIs, and backend URIs
- `.LCK` lock detection for file-backed books
- account name validation
- amount-string validation with configurable decimal and thousand separators

### 3. CSV Helper Tests (`test_csv_helper.c`)

These tests cover:

- `csv_printMessage()` without active parser context
- `csv_printMessage()` during field and row callbacks
- formatted message generation through `csv_printMessagef()`

### 4. Date Parsing Tests (`test_date_parsing.c`)

These tests cover:

- supported date formats such as `DD/MM/YYYY` and `MM/DD/YYYY`
- multiple separators
- invalid dates and malformed input
- null-handling and edge cases

### 5. Number Parsing Tests (`test_number_parsing.c`)

These tests cover:

- US-style number parsing
- European-style number parsing
- sign handling
- invalid amount strings and normalization edge cases

### 6. CSV Parameter and Rule-File Tests (`test_csv_params.c`)

These tests cover the current rule-file parser behavior:

- optional `CSVPARAMS` handling
- delimiter, quote, decimal separator, thousand separator, and `SKIP_ROWS`
- filter type parsing for `PATTERN`/`GLOB` shell wildcards and real `REGEX`
- optional `UNMATCHED` account parsing for manually reviewed imports
- behavior when `CSVPARAMS` is absent
- minimal and malformed rule files

Important detail: the configuration file is a CSV-like rule file, not an INI parser. The sample files keep the `.ini` extension for historical reasons only.

### 7. Utility Function Tests (`test_utilities.c`)

These tests cover:

- `GString` helpers
- prefix and pattern matching helpers
- safe cleanup helpers
- byte-buffer to string conversion
- `printData()` formatting, including null and partially populated transactions

### 8. GnuCash Session Utility Tests (`test_gnc_utilities.c`)

These tests cover the save/error path through `save_session_with_hooks()`:

- successful save when no session error is reported
- failed save when GnuCash reports an error message
- invalid hook/session configuration rejection

These are unit-level tests of the save contract, not full runtime save tests against a real book.

### 9. Integration Tests (`test_integration.c`)

These tests exercise the parser and matcher pipeline using repository sample data:

- US-format transaction samples
- EU-format transaction samples
- malformed input handling
- repeated load/match/free cycles for cleanup coverage
- config and CSV file presence checks

These tests validate parser and matcher behavior, but they do not open or save real GnuCash books.

### 10. Real-Book Integration Tests (`test_book_integration.c`)

These tests cover:

- creating temporary real GnuCash books through the backend
- saving and reloading persisted books
- import round-trips from CSV parsing through transaction creation and duplicate lookup
- unmatched import routing to a review account
- merge round-trips across separate source and target books

These are the highest-fidelity tests in the current suite because they exercise the real GnuCash storage backend rather than only in-memory parser behavior.

## Test Data Files

- `sample_transactions_us.csv`: US-style CSV data
- `sample_transactions_eu.csv`: EU-style CSV data
- `malformed_data.csv`: intentionally bad rows
- `sample_config_us.ini`: US-style rule file sample
- `sample_config_eu.ini`: EU-style rule file sample

The sample config files are structured CSV-like rule files even though they use the `.ini` extension.

## Build Integration

The build generates a single test executable and exposes it through CTest. In practice the most useful entry points are:

```bash
make test
ctest --test-dir build --output-on-failure
./build/test_gnc-toolbox
```

## Notes

- `run_tests.sh validation` performs lightweight CLI checks such as `--help`, missing required arguments, and `--version`.
- The suite now spans helper-level tests, parser/matcher integration, and temporary real-book backend round-trips.
- There is still room to add full CLI subprocess end-to-end tests if you want to verify argument parsing, logging, and exit codes together with the backend.
- Build artifacts are written under `build/` and can be removed with `make clean`.
