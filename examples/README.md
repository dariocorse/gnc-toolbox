# Examples

These files show the rule-file shape expected by `gnc-toolbox`.

They are safe to inspect and edit, but the commands below still need a real GnuCash book with the referenced accounts already created.

## Import Preview

```bash
build/gnc-toolbox import examples/bank-us.csv \
  --book finances.gnucash \
  --config examples/import-us.rules.csv \
  --dry-run \
  --verbose
```

Save the import and skip transactions already present in the destination account:

```bash
build/gnc-toolbox import examples/bank-us.csv \
  --book finances.gnucash \
  --config examples/import-us.rules.csv \
  --no-duplicates
```

## Unmatched Review

`examples/import-us.rules.csv` includes:

```text
UNMATCHED,Expenses:Uncategorized
```

Rows that do not match a rule are still imported and balanced against that account, so you can reassign them by hand inside GnuCash later.

## Merge Preview

```bash
build/gnc-toolbox merge \
  --book current.gnucash \
  --from archive.gnucash \
  --account "Assets:Checking" \
  --config examples/merge.rules.csv \
  --dry-run \
  --verbose
```

The merge command uses the same rule-file structure as import. The column-definition row is required for structural validation even though merge reads transactions from the source book rather than from a CSV file.

