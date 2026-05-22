# GnuCash Toolbox

GnuCash Toolbox is a command-line utility for importing and reconciling transactions in GnuCash books.

It currently supports two workflows:

- `import`: read transactions from a bank CSV export, apply parsing and matching rules, and create transactions in a GnuCash book.
- `merge`: copy missing transactions from an account in one GnuCash book into another book.

The tool is designed for repeatable imports where the software should parse the source data, match what it can, and still keep unmatched transactions available for manual review in GnuCash.

## Features

- Configurable CSV parsing: delimiter, quote character, decimal separator, thousand separator, skipped header rows, and date formats.
- Rule-based row filtering before import.
- Description matching by substring, shell-style wildcard pattern, or GLib regular expression.
- Multi-account split creation with percentage allocations.
- Explicit unmatched-transaction routing through an `UNMATCHED,<account>` rule.
- Duplicate detection against the destination account.
- Local GnuCash files, `file://` URIs, and supported GnuCash backend URIs.
- Local file lock checks for `.LCK` and `.LNK` files.
- Dry-run mode for reviewing changes before writing to a book.

## Requirements

This project is written in C and links against GnuCash, GLib, and libcsv.

On Ubuntu/Debian:

```bash
sudo apt install libgnucash-dev libglib2.0-dev libcsv-dev cmake build-essential
```

Backend URIs such as MySQL work only when the locally installed GnuCash libraries support that backend.

## Build And Install

```bash
make build
```

The executable is created at:

```text
build/gnc-toolbox
```

Useful build targets:

```bash
make build                         # Release build
make debug                         # Debug build
make test                          # Build and run the test suite through CTest
make install PREFIX=$HOME/.local   # Install binary and docs
make uninstall PREFIX=$HOME/.local # Remove installed files from the install manifest
make clean                         # Remove build artifacts
```

Direct CMake builds are also supported. See [BUILDING.md](BUILDING.md) for install prefixes, staged installs with `DESTDIR`, uninstall behavior, and source packages.

## Quick Start

Preview a CSV import without saving changes:

```bash
build/gnc-toolbox import bank.csv --book finances.gnucash --config bank-rules.csv --dry-run --verbose
```

Import and skip transactions that already exist in the destination account:

```bash
build/gnc-toolbox import bank.csv --book finances.gnucash --config bank-rules.csv --no-duplicates
```

Merge missing transactions from another book:

```bash
build/gnc-toolbox merge --book target.gnucash --from source.gnucash --account "Assets:Checking" --config merge-rules.csv --dry-run
```

Show command help:

```bash
build/gnc-toolbox help
build/gnc-toolbox help import
build/gnc-toolbox help merge
build/gnc-toolbox import --help
build/gnc-toolbox merge --help
```

## Rule File Example

Rule files are CSV-like text files. They are not INI files, even if an older example or local setup uses a `.ini` extension.

```text
## Parse a US-style bank CSV and skip the CSV header row
CSVPARAMS,"DELIMITER=,",QUOTE=",DECIMAL_SEP=.,"THOUSAND_SEP=,",SKIP_ROWS=1
FILTEROUT,CONTAINS,PENDING
Assets:Checking, -
UNMATCHED,Expenses:Uncategorized
DATE:FORMAT=MM/DD/YYYY,DESCR,AMOUNT,NUM
Grocery,Expenses:Groceries,100
Salary,Income:Salary,100
Mortgage,Expenses:Mortgage,80,Expenses:Insurance,20
$AMZN*,Expenses:Shopping,100
```

Important behavior:

- The base account row, `Assets:Checking, -` in the example, is the account receiving the imported bank-side split.
- Matched transactions are balanced against the accounts in the matching rows.
- Unmatched transactions are imported only when an `UNMATCHED` account is configured.
- Without `UNMATCHED`, unmatched transactions are reported as errors and are not written.

Full command and rule-file documentation is in [USAGE.md](USAGE.md).

## Examples

The [examples](examples/) directory contains:

- `bank-us.csv`: a small sample bank export,
- `import-us.rules.csv`: import rules with filters, matching, split allocation, and `UNMATCHED` routing,
- `merge.rules.csv`: a merge rule file using the same matching behavior.

The examples are designed for dry-run experiments against a local test book.

## Safety

- Use `--dry-run --verbose` while creating or changing rule files.
- Back up important GnuCash books before writing imports or merges.
- Close GnuCash before running this tool against a local file-backed book.
- Create the base, matched, and unmatched accounts in GnuCash before importing.
- Use full account paths such as `Expenses:Food:Groceries` when short names may be ambiguous.

## Documentation

- [USAGE.md](USAGE.md): user guide, commands, rule files, matching, and troubleshooting.
- [BUILDING.md](BUILDING.md): build, install, uninstall, and packaging commands.
- [PACKAGING.md](PACKAGING.md): package archive contents and release checklist.
- [TESTING.md](TESTING.md): test-suite structure and commands.
- [CONTRIBUTING.md](CONTRIBUTING.md): contributor workflow and project conventions.
- [CHANGELOG.md](CHANGELOG.md): release history and release checklist.
- [SECURITY.md](SECURITY.md): vulnerability reporting policy.

## License

GnuCash Toolbox is licensed under the GNU Affero General Public License v3.0 or later.

```text
SPDX-License-Identifier: AGPL-3.0-or-later
```

See [LICENSE](LICENSE) for the full license text.

## Project Status

The current focus is reliable CSV import, matching, unmatched review routing, duplicate handling, and merge support.
