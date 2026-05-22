# GnuCash Toolbox User Guide

This guide explains what GnuCash Toolbox does, how to run it, and how to write the rule files used by CSV imports and merges.

## Contents

- [Concepts](#concepts)
- [Recommended Workflow](#recommended-workflow)
- [Commands](#commands)
- [Book References](#book-references)
- [Account Names](#account-names)
- [Rule File Format](#rule-file-format)
- [Complete Import Example](#complete-import-example)
- [Output Levels](#output-levels)
- [Troubleshooting](#troubleshooting)
- [Development And Tests](#development-and-tests)

## Concepts

GnuCash Toolbox works with three kinds of input:

- A GnuCash destination book, passed with `--book`.
- A rule file, passed with `--config`.
- A transaction source, either a CSV file for `import` or another GnuCash book/account for `merge`.

The rule file tells the tool:

- how to parse the transaction CSV,
- which rows to filter out,
- which destination account is the base account,
- which account receives unmatched transactions for later manual review,
- which CSV columns contain dates, descriptions, amounts, and transaction numbers,
- how to match transaction descriptions to GnuCash accounts.

For `merge`, the rule file still provides the target base account, filters, match rows, and optional unmatched account. CSV parsing settings and column definitions are parsed because the rule-file format is shared, but they are only meaningful for `import`.

## Recommended Workflow

1. Create or confirm the needed GnuCash accounts: base account, destination accounts, and optional unmatched review account.
2. Create a small rule file with the correct `CSVPARAMS`, base account, column mapping, and a few match rows.
3. Run with `--dry-run --debug` until parsing and matching are correct.
4. Add `UNMATCHED,<account>` if unmatched rows must still be imported for manual categorization.
5. Run with `--dry-run --verbose` and review the summary.
6. Back up the GnuCash book.
7. Run without `--dry-run` when the preview is acceptable.

## Commands

General help:

```bash
build/gnc-toolbox help
build/gnc-toolbox help import
build/gnc-toolbox help merge
```

Command-specific help is also available with `--help`:

```bash
build/gnc-toolbox import --help
build/gnc-toolbox merge --help
```

### `import`

Use `import` to read transactions from a CSV file and create transactions in a GnuCash book.

```bash
build/gnc-toolbox import [OPTIONS] <CSV_FILE>
```

Required:

- `<CSV_FILE>`: transaction CSV file to import.
- `--book <BOOK_REF>` or `-b <BOOK_REF>`: destination GnuCash book.
- `--config <FILE>` or `-c <FILE>`: rule file.

Options:

- `--dry-run` or `-n`: parse and match transactions without saving changes.
- `--no-duplicates` or `-D`: skip transactions already found in the destination account.
- `--verbose` or `-v`: show progress and summaries.
- `--debug` or `-d`: show detailed parser and matching diagnostics.
- `--quiet` or `-q`: show only errors.
- `--help` or `-h`: show import help.
- `--version` or `-V`: show version information.

Examples:

```bash
build/gnc-toolbox import bank.csv --book finances.gnucash --config bank-rules.csv --dry-run --verbose
build/gnc-toolbox import bank.csv -b finances.gnucash -c bank-rules.csv --no-duplicates
build/gnc-toolbox import bank.csv --book "mysql://user:pass@db.example.com/gnucash" --config bank-rules.csv --dry-run
```

### `merge`

Use `merge` to copy transactions from one GnuCash book/account into another GnuCash book.

```bash
build/gnc-toolbox merge [OPTIONS]
```

Required:

- `--book <BOOK_REF>` or `-b <BOOK_REF>`: target GnuCash book.
- `--from <BOOK_REF>` or `-f <BOOK_REF>`: source GnuCash book.
- `--account <NAME>` or `-a <NAME>`: source account to merge from.
- `--config <FILE>` or `-c <FILE>`: rule file.

Options:

- `--dry-run` or `-n`: preview the merge without saving changes.
- `--verbose` or `-v`: show progress and summaries.
- `--debug` or `-d`: show detailed diagnostics.
- `--quiet` or `-q`: show only errors.
- `--help` or `-h`: show merge help.
- `--version` or `-V`: show version information.

Examples:

```bash
build/gnc-toolbox merge --book current.gnucash --from archive.gnucash --account "Assets:Checking" --config merge-rules.csv --dry-run
build/gnc-toolbox merge -b current.gnucash -f archive.gnucash -a "Checking" -c merge-rules.csv --verbose
```

Merge behavior:

- The `--account` option selects the source account in the source book.
- The base-account row in the rule file selects the destination account in the target book.
- Existing target transactions are detected by date and amount; exact matches also compare description.
- Transactions selected for creation are then processed through filters and match rows like imported CSV transactions.
- Use `UNMATCHED` when source transactions may not match a rule but should still be copied for manual review.

Because the merge command uses the same rule-file parser as import, a merge rule file must still include a column-definition row. The row is not used to parse source-book transactions, but it keeps the rule file structurally valid.

## Book References

`--book` and `--from` accept:

- local paths, such as `finances.gnucash`,
- `file://` URIs, such as `file:///home/user/finances.gnucash`,
- backend URIs, such as `mysql://user:pass@db.example.com/gnucash`.

For local files and `file://` URIs:

- the file must exist and be readable,
- the extension must be `.gnucash`, `.xac`, or `.xml`,
- `.LCK` and `.LNK` lock files are checked before opening.

Backend URIs are passed to GnuCash. They work only if the installed GnuCash libraries support that backend.

## Account Names

Account names in rule files and command arguments can be simple names or full paths.

Examples:

```text
Checking
Assets:Checking
Expenses:Food:Groceries
```

Use full paths when more than one account shares the same short name.

Unsupported characters in account names are:

```text
< > | * ? "
```

## Rule File Format

The rule file is CSV-like text parsed row by row. It is not an INI file, even if the filename ends in `.ini`.

Rows must appear in this order:

1. optional `CSVPARAMS` row,
2. zero or more `FILTEROUT` rows,
3. one base-account row,
4. optional `UNMATCHED` row,
5. one column-definition row,
6. one or more match rows.

Rows whose first field starts with `##` are comments.

### `CSVPARAMS`

`CSVPARAMS` configures how the transaction CSV is parsed.

Supported fields:

- `DELIMITER=<char>`: CSV field delimiter.
- `QUOTE=<char>`: CSV quote character.
- `DECIMAL_SEP=<char>`: decimal separator for amounts.
- `THOUSAND_SEP=<char>`: thousand separator for amounts.
- `SKIP_ROWS=<n>`: number of initial transaction CSV rows to skip.

Default values when `CSVPARAMS` is omitted:

```text
DELIMITER=,
QUOTE="
DECIMAL_SEP=.
THOUSAND_SEP=,
SKIP_ROWS=0
```

US-style example:

```text
CSVPARAMS,"DELIMITER=,",QUOTE=",DECIMAL_SEP=.,"THOUSAND_SEP=,",SKIP_ROWS=1
```

European-style example:

```text
CSVPARAMS,DELIMITER=;,QUOTE=","DECIMAL_SEP=,",THOUSAND_SEP=.,SKIP_ROWS=1
```

### Filters

`FILTEROUT` rows remove matching transactions before account matching.

Supported filter types:

- `CONTAINS`: substring match on transaction description.
- `PATTERN` or `GLOB`: shell-style wildcard match on transaction description, using `*` and `?`.
- `REGEX`: GLib regular expression match on transaction description.
- `DATE_LEQ`: filters transactions whose date is less than or equal to a configured date.

Examples:

```text
FILTEROUT,CONTAINS,PENDING
FILTEROUT,PATTERN,*AUTHORIZATION*
FILTEROUT,REGEX,^CARD [0-9]+$
FILTEROUT,DATE_LEQ,01/01/2024,DD/MM/YYYY
```

For `DATE_LEQ`, field 3 is the date value and field 4 is the date format.

Pattern and regex behavior:

- `PATTERN` and `GLOB` are aliases. They use shell-style wildcards: `*` for any text and `?` for one character.
- `REGEX` uses GLib regular expressions. Use it for real regular expressions such as `^CARD [0-9]+$`.
- A value such as `*CARD*` is a pattern, not a valid regular expression. Use `PATTERN,*CARD*`.

### Base-Account Row

The base-account row defines the destination account that receives the primary split.

```text
Assets:Checking, -
```

Field 1 is the account name. Field 2 is optional and sets the separator used when multiple `DESCR` columns are joined.

### Unmatched-Account Row

The optional `UNMATCHED` row defines where transactions with no matching rule are imported.

```text
UNMATCHED,Expenses:Uncategorized
```

When a transaction has no match row:

- the base account still receives the primary split,
- the balancing split is written to the `UNMATCHED` account,
- the transaction is counted as unmatched in the summary,
- the user can later review that account in GnuCash and reassign the split by hand.

If unmatched transactions are found and no `UNMATCHED` account is configured, those transactions are not imported and are counted as errors. The tool does not intentionally rely on GnuCash's automatic imbalance accounts.

### Column-Definition Row

The column-definition row maps input CSV columns to transaction fields.

Supported column tokens:

- `DATE`
- `DATE:FORMAT=<pattern>`
- `DESCR`
- `NUM`
- `AMOUNT`
- `-AMOUNT`
- `IGNORE`
- `NONE`

Example:

```text
DATE:FORMAT=MM/DD/YYYY,DESCR,AMOUNT,NUM
```

Supported date format tokens:

- `DD`
- `MM`
- `YYYY`
- `YY`

Examples:

```text
DATE:FORMAT=DD/MM/YYYY
DATE:FORMAT=MM/DD/YYYY
DATE:FORMAT=YYYY-MM-DD
```

`AMOUNT` columns are summed. `-AMOUNT` columns are summed after sign inversion. This is useful for CSV files with separate credit and debit columns.

Example:

```text
DATE:FORMAT=YYYY-MM-DD,DESCR,AMOUNT,-AMOUNT,NUM
```

### Match Rows

Match rows map transaction descriptions to one or more destination accounts.

Format:

```text
MATCH_TEXT,ACCOUNT_NAME,PERCENTAGE,ACCOUNT_NAME,PERCENTAGE,...
```

Matching behavior:

- plain match text uses substring matching,
- match text starting with `$` uses shell-style wildcard matching with `*` and `?`; the `$` is not part of the pattern,
- one destination account defaults to `100`,
- multiple destination accounts require percentages,
- percentages must add up to `100`.

Examples:

```text
Grocery,Expenses:Food:Groceries,100
Salary,Income:Salary,100
Mortgage,Expenses:Mortgage,80,Expenses:Insurance,20
$AMZN*,Expenses:Shopping,100
```

Important: run `--dry-run --verbose` and review unmatched warnings before saving. If unmatched transactions should be imported for manual categorization, configure an `UNMATCHED` account.

Matching stops at the first matching row, so put more specific rules before broader rules.

## Complete Import Example

Bank CSV:

```text
Date,Description,Amount,Reference
12/25/2024,Grocery Store,-82.34,TX001
12/31/2024,Salary,2500.00,TX002
01/02/2025,PENDING CARD AUTH,-10.00,TX003
```

Rule file:

```text
## bank-rules.csv
CSVPARAMS,"DELIMITER=,",QUOTE=",DECIMAL_SEP=.,"THOUSAND_SEP=,",SKIP_ROWS=1
FILTEROUT,CONTAINS,PENDING
Assets:Checking, -
UNMATCHED,Expenses:Uncategorized
DATE:FORMAT=MM/DD/YYYY,DESCR,AMOUNT,NUM
Grocery,Expenses:Food:Groceries,100
Salary,Income:Salary,100
```

Preview:

```bash
build/gnc-toolbox import bank.csv --book finances.gnucash --config bank-rules.csv --dry-run --verbose
```

Save:

```bash
build/gnc-toolbox import bank.csv --book finances.gnucash --config bank-rules.csv --no-duplicates
```

## Output Levels

- `--quiet`: errors only.
- default: warnings and errors.
- `--verbose`: progress, summaries, and useful row information.
- `--debug`: detailed parser and matching diagnostics.

Color is enabled only when output is connected to a terminal. Set `NO_COLOR=1` to disable color explicitly.

When a CSV error occurs, messages include the file, row, field, and value where possible.

Example:

```text
CSV 'bank.csv' row 3, field 3, value 'abc': Invalid amount; expected decimal separator '.' and thousand separator ','
```

Exit behavior:

- A successful dry run exits successfully and does not save the destination book.
- A successful write run exits successfully after saving the destination book.
- Invalid arguments, invalid rule files, missing accounts, failed book loading, and failed saves exit with an error.

## Troubleshooting

### The book is locked

Close GnuCash and any other program using the book. For file-backed books, the tool checks for `.LCK` and `.LNK` files.

### The rule file fails to load

Check the CSV row and field reported in the error. Common causes are:

- rows are in the wrong order,
- unknown column token,
- invalid `DATE_LEQ` date or format,
- invalid `REGEX` pattern,
- split percentages do not add to `100`,
- the `UNMATCHED` account row is missing an account name.

### No transactions are imported

Use:

```bash
build/gnc-toolbox import bank.csv --book finances.gnucash --config bank-rules.csv --dry-run --debug
```

Check:

- `SKIP_ROWS` is not skipping too many rows,
- delimiter and quote settings match the CSV,
- date format matches the CSV dates,
- amount separators match the CSV amounts.

### Account not found

Use a full account path in the rule file or command argument:

```text
Assets:Checking
Expenses:Food:Groceries
```

Check that the account exists in the target or source book exactly as written.

### Unmatched transactions should be imported

Add an `UNMATCHED` row after the base-account row:

```text
Assets:Checking, -
UNMATCHED,Expenses:Uncategorized
```

Create that account in GnuCash first. During import, transactions with no match rule will be balanced against that account and can be reassigned manually later.

### Duplicate transactions

Use `--no-duplicates` or `-D` to skip transactions already found in the destination account. Duplicate detection compares date and amount, and exact matches also compare description.

## Development And Tests

Run:

```bash
make test
```

See [TESTING.md](TESTING.md) for details.
