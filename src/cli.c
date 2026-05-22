/*
 * GnuCash Toolbox - CSV Import, Merge and Analysis Tools
 * Copyright (C) 2024 Dario Corsetti <dev@dariocorsetti.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * This file is part of GnuCash Toolbox.
 *
 * GnuCash Toolbox is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * GnuCash Toolbox is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with GnuCash Toolbox. If not, see <https://www.gnu.org/licenses/>.
 *
 * Repository: https://github.com/dariocorsetti/gnc-toolbox
 */

/* Includes ------------------------------------------------------------------*/
#include <cli.h>
#include <output.h>
#include <utilities.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>

/* Private definitions -------------------------------------------------------*/
#define MAX_ARG_LENGTH 512

/* Private macros ------------------------------------------------------------*/
/* Private typedefs ----------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/

// Import-specific options
static struct option import_options[] = {
    {"help", no_argument, 0, 'h'},       {"version", no_argument, 0, 'V'},
    {"book", required_argument, 0, 'b'}, {"config", required_argument, 0, 'c'},
    {"dry-run", no_argument, 0, 'n'},    {"no-duplicates", no_argument, 0, 'D'},
    {"verbose", no_argument, 0, 'v'},    {"quiet", no_argument, 0, 'q'},
    {"debug", no_argument, 0, 'd'},      {0, 0, 0, 0}};

// Merge-specific options
static struct option merge_options[] = {{"help", no_argument, 0, 'h'},
                                        {"version", no_argument, 0, 'V'},
                                        {"book", required_argument, 0, 'b'},
                                        {"config", required_argument, 0, 'c'},
                                        {"from", required_argument, 0, 'f'},
                                        {"account", required_argument, 0, 'a'},
                                        {"dry-run", no_argument, 0, 'n'},
                                        {"verbose", no_argument, 0, 'v'},
                                        {"quiet", no_argument, 0, 'q'},
                                        {"debug", no_argument, 0, 'd'},
                                        {0, 0, 0, 0}};

/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static bool parse_import_args(int argc, char *argv[], CliArgs_t *args);
static bool parse_merge_args(int argc, char *argv[], CliArgs_t *args);
static bool parse_help_args(int argc, char *argv[], CliArgs_t *args);
static void print_general_help(void);
static void print_import_help(void);
static void print_merge_help(void);
static char *safe_strdup(const char *str);

/* Public constants ----------------------------------------------------------*/
/* Public functions ----------------------------------------------------------*/

void cli_init_args(CliArgs_t *args)
{
    if (args == NULL)
        return;

    memset(args, 0, sizeof(CliArgs_t));
    args->operation = CLI_OPERATION_NONE;
    args->verbosity = CLI_VERBOSITY_NORMAL;
    args->dry_run = false;
    args->no_duplicates = false;
    args->help_requested = false;
    args->version_requested = false;
}

void cli_cleanup_args(CliArgs_t *args)
{
    if (args == NULL)
        return;

    if (args->book_ref) {
        free(args->book_ref);
        args->book_ref = NULL;
    }
    if (args->config_file) {
        free(args->config_file);
        args->config_file = NULL;
    }
    if (args->csv_file) {
        free(args->csv_file);
        args->csv_file = NULL;
    }
    if (args->source_book_ref) {
        free(args->source_book_ref);
        args->source_book_ref = NULL;
    }
    if (args->source_account) {
        free(args->source_account);
        args->source_account = NULL;
    }
}

void cli_configure_output_level(CliVerbosity_e verbosity)
{
    switch (verbosity) {
    case CLI_VERBOSITY_QUIET:
        out_setLevel(OUT_LEVEL_ERROR);
        break;
    case CLI_VERBOSITY_NORMAL:
        out_setLevel(OUT_LEVEL_SUMMARY);
        break;
    case CLI_VERBOSITY_VERBOSE:
        out_setLevel(OUT_LEVEL_VERBOSE);
        break;
    case CLI_VERBOSITY_DEBUG:
        out_setLevel(OUT_LEVEL_DEBUG);
        break;
    default:
        out_setLevel(OUT_LEVEL_SUMMARY);
        break;
    }
}

bool cli_parse_args(int argc, char *argv[], CliArgs_t *args)
{
    if (argc < 1 || argv == NULL || args == NULL) {
        return false;
    }

    cli_init_args(args);

    // Handle no arguments or just program name
    if (argc == 1) {
        args->operation = CLI_OPERATION_HELP;
        return true;
    }

    // Check for global options first
    if (argc == 2) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            args->operation = CLI_OPERATION_HELP;
            args->help_requested = true;
            return true;
        }
        if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0) {
            args->operation = CLI_OPERATION_VERSION;
            args->version_requested = true;
            return true;
        }
    }

    // Parse subcommand
    const char *subcommand = argv[1];

    if (strcmp(subcommand, "import") == 0) {
        args->operation = CLI_OPERATION_IMPORT;
        return parse_import_args(argc - 1, argv + 1, args);
    } else if (strcmp(subcommand, "merge") == 0) {
        args->operation = CLI_OPERATION_MERGE;
        return parse_merge_args(argc - 1, argv + 1, args);
    } else if (strcmp(subcommand, "help") == 0) {
        return parse_help_args(argc, argv, args);
    } else {
        OUT_ERROR("Unknown subcommand: %s\n", subcommand);
        OUT_ERROR("Run 'gnc-toolbox help' for available commands.\n");
        return false;
    }
}

bool cli_validate_args(const CliArgs_t *args)
{
    if (args == NULL) {
        return false;
    }

    bool is_valid = true;

    switch (args->operation) {
    case CLI_OPERATION_IMPORT:
        // Required: book, config, csv file
        if (args->book_ref == NULL) {
            OUT_ERROR(
                "Import operation requires --book <BOOK_REF> for the destination GnuCash book\n");
            is_valid = false;
        } else {
            GError *error = NULL;
            if (!validate_gnucash_book_ref(args->book_ref, &error)) {
                OUT_ERROR("Book validation failed: %s\n", error->message);
                g_error_free(error);
                is_valid = false;
            }
        }

        if (args->config_file == NULL) {
            OUT_ERROR("Import operation requires --config <FILE> for the CSV-like rule file\n");
            is_valid = false;
        } else {
            GError *error = NULL;
            if (!validate_file_exists_and_readable(args->config_file, &error)) {
                OUT_ERROR("Config file validation failed: %s\n", error->message);
                g_error_free(error);
                is_valid = false;
            }
        }

        if (args->csv_file == NULL) {
            OUT_ERROR("Import operation requires a transaction CSV file argument\n");
            is_valid = false;
        } else {
            GError *error = NULL;
            if (!validate_file_exists_and_readable(args->csv_file, &error)) {
                OUT_ERROR("CSV file validation failed: %s\n", error->message);
                g_error_free(error);
                is_valid = false;
            }
        }
        break;

    case CLI_OPERATION_MERGE:
        // Required: book, config, from, account
        if (args->book_ref == NULL) {
            OUT_ERROR("Merge operation requires --book <BOOK_REF> for the target GnuCash book\n");
            is_valid = false;
        } else {
            GError *error = NULL;
            if (!validate_gnucash_book_ref(args->book_ref, &error)) {
                OUT_ERROR("Book validation failed: %s\n", error->message);
                g_error_free(error);
                is_valid = false;
            }
        }

        if (args->config_file == NULL) {
            OUT_ERROR("Merge operation requires --config <FILE> for the CSV-like rule file\n");
            is_valid = false;
        } else {
            GError *error = NULL;
            if (!validate_file_exists_and_readable(args->config_file, &error)) {
                OUT_ERROR("Config file validation failed: %s\n", error->message);
                g_error_free(error);
                is_valid = false;
            }
        }

        if (args->source_book_ref == NULL) {
            OUT_ERROR("Merge operation requires --from <BOOK_REF> for the source GnuCash book\n");
            is_valid = false;
        } else {
            GError *error = NULL;
            if (!validate_gnucash_book_ref(args->source_book_ref, &error)) {
                OUT_ERROR("Source book validation failed: %s\n", error->message);
                g_error_free(error);
                is_valid = false;
            }
        }

        if (args->source_account == NULL) {
            OUT_ERROR(
                "Merge operation requires --account <NAME> for the source account to merge\n");
            is_valid = false;
        } else if (!is_valid_account_name(args->source_account)) {
            OUT_ERROR("Invalid account name '%s': remove unsupported characters < > | * ? \"\n",
                      args->source_account);
            is_valid = false;
        }
        break;

    case CLI_OPERATION_HELP:
    case CLI_OPERATION_VERSION:
    case CLI_OPERATION_NONE:
        // No validation needed for these
        break;
    }

    return is_valid;
}

void cli_print_help(CliOperation_e operation)
{
    switch (operation) {
    case CLI_OPERATION_IMPORT:
        print_import_help();
        break;
    case CLI_OPERATION_MERGE:
        print_merge_help();
        break;
    case CLI_OPERATION_HELP:
    case CLI_OPERATION_NONE:
    default:
        print_general_help();
        break;
    }
}

void cli_print_version(void)
{
    printf("gnc-toolbox version %s\n", PROJECT_VERSION);
    printf("GnuCash Toolbox - CSV Import, Merge and Analysis Tools\n");
    printf("Copyright (C) 2024 Dario Corsetti <dev@dariocorsetti.com>\n");
    printf("License: GNU AGPLv3 or later (SPDX: AGPL-3.0-or-later)\n");
    printf("Repository: https://github.com/dariocorsetti/gnc-toolbox\n");
}

void cli_print_examples(void)
{
    printf("\nEXAMPLES:\n");
    printf("  # Import CSV data into GnuCash book\n");
    printf(
        "  gnc-toolbox import transactions.csv --book mybook.gnucash --config bank-rules.csv\n\n");

    printf("  # Preview import without making changes\n");
    printf("  gnc-toolbox import transactions.csv --book mybook.gnucash --config bank-rules.csv "
           "--dry-run\n\n");

    printf("  # Import with verbose output, skipping duplicates\n");
    printf("  gnc-toolbox import data.csv -b book.gnucash -c bank-rules.csv --verbose "
           "--no-duplicates\n\n");

    printf("  # Import into a MySQL-backed GnuCash book\n");
    printf("  gnc-toolbox import data.csv --book \"mysql://user:pass@db.example.com/gnucash\" -c "
           "bank-rules.csv --dry-run\n\n");

    printf("  # Merge accounts from another book\n");
    printf("  gnc-toolbox merge --book target.gnucash --config merge-rules.csv --from "
           "source.gnucash --account \"Assets:Checking\"\n\n");

    printf("  # Preview merge operation\n");
    printf("  gnc-toolbox merge -b main.gnucash -c merge-rules.csv -f other.gnucash -a "
           "\"Assets:Savings\" --dry-run\n\n");
}

/* Private functions ---------------------------------------------------------*/
static char *safe_strdup(const char *str)
{
    if (str == NULL)
        return NULL;

    size_t len = strlen(str);
    if (len >= MAX_ARG_LENGTH) {
        OUT_ERROR("Argument too long (max %d characters)\n", MAX_ARG_LENGTH - 1);
        return NULL;
    }

    return strdup(str);
}

static bool parse_help_args(int argc, char *argv[], CliArgs_t *args)
{
    args->operation = CLI_OPERATION_HELP;
    args->help_requested = true;

    if (argc == 2) {
        return true;
    }

    if (argc > 3) {
        OUT_ERROR("Too many arguments for help command\n");
        OUT_ERROR("Usage: gnc-toolbox help [import|merge]\n");
        return false;
    }

    if (strcmp(argv[2], "import") == 0) {
        args->operation = CLI_OPERATION_IMPORT;
        return true;
    }

    if (strcmp(argv[2], "merge") == 0) {
        args->operation = CLI_OPERATION_MERGE;
        return true;
    }

    if (strcmp(argv[2], "--help") == 0 || strcmp(argv[2], "-h") == 0) {
        return true;
    }

    OUT_ERROR("Unknown help topic: %s\n", argv[2]);
    OUT_ERROR("Available help topics: import, merge\n");
    return false;
}

static bool parse_import_args(int argc, char *argv[], CliArgs_t *args)
{
    int c;
    int option_index = 0;

    // Reset getopt
    optind = 1;

    while ((c = getopt_long(argc, argv, "hVb:c:nDvqd", import_options, &option_index)) != -1) {
        switch (c) {
        case 'h':
            args->help_requested = true;
            return true;

        case 'V':
            args->version_requested = true;
            args->operation = CLI_OPERATION_VERSION;
            return true;

        case 'b':
            args->book_ref = safe_strdup(optarg);
            if (args->book_ref == NULL)
                return false;
            break;

        case 'c':
            args->config_file = safe_strdup(optarg);
            if (args->config_file == NULL)
                return false;
            break;

        case 'n':
            args->dry_run = true;
            break;

        case 'D':
            args->no_duplicates = true;
            break;

        case 'v':
            args->verbosity = CLI_VERBOSITY_VERBOSE;
            break;

        case 'q':
            args->verbosity = CLI_VERBOSITY_QUIET;
            break;

        case 'd':
            args->verbosity = CLI_VERBOSITY_DEBUG;
            break;

        case '?':
            return false;

        default:
            OUT_ERROR("Unknown option for import command\n");
            return false;
        }
    }

    // Get CSV file as positional argument
    if (optind < argc) {
        args->csv_file = safe_strdup(argv[optind]);
        if (args->csv_file == NULL)
            return false;
        optind++;
    }

    if (optind < argc) {
        OUT_ERROR("Unexpected argument for import command: %s\n", argv[optind]);
        OUT_ERROR("Usage: gnc-toolbox import [OPTIONS] <CSV_FILE>\n");
        return false;
    }

    return true;
}

static bool parse_merge_args(int argc, char *argv[], CliArgs_t *args)
{
    int c;
    int option_index = 0;

    // Reset getopt
    optind = 1;

    while ((c = getopt_long(argc, argv, "hVb:c:f:a:nvqd", merge_options, &option_index)) != -1) {
        switch (c) {
        case 'h':
            args->help_requested = true;
            return true;

        case 'V':
            args->version_requested = true;
            args->operation = CLI_OPERATION_VERSION;
            return true;

        case 'b':
            args->book_ref = safe_strdup(optarg);
            if (args->book_ref == NULL)
                return false;
            break;

        case 'c':
            args->config_file = safe_strdup(optarg);
            if (args->config_file == NULL)
                return false;
            break;

        case 'f':
            args->source_book_ref = safe_strdup(optarg);
            if (args->source_book_ref == NULL)
                return false;
            break;

        case 'a':
            args->source_account = safe_strdup(optarg);
            if (args->source_account == NULL)
                return false;
            break;

        case 'n':
            args->dry_run = true;
            break;

        case 'v':
            args->verbosity = CLI_VERBOSITY_VERBOSE;
            break;

        case 'q':
            args->verbosity = CLI_VERBOSITY_QUIET;
            break;

        case 'd':
            args->verbosity = CLI_VERBOSITY_DEBUG;
            break;

        case '?':
            return false;

        default:
            OUT_ERROR("Unknown option for merge command\n");
            return false;
        }
    }

    if (optind < argc) {
        OUT_ERROR("Unexpected argument for merge command: %s\n", argv[optind]);
        OUT_ERROR("Usage: gnc-toolbox merge [OPTIONS]\n");
        return false;
    }

    return true;
}

static void print_general_help(void)
{
    printf("USAGE:\n");
    printf("  gnc-toolbox <SUBCOMMAND> [OPTIONS]\n\n");
    printf("  gnc-toolbox help [import|merge]\n\n");

    printf("SUBCOMMANDS:\n");
    printf("  import    Import CSV transactions into GnuCash book\n");
    printf("  merge     Merge transactions from another GnuCash book\n");
    printf("  help      Show help information\n\n");

    printf("GLOBAL OPTIONS:\n");
    printf("  -h, --help       Show help information\n");
    printf("  -V, --version    Show version information\n\n");

    printf("Use 'gnc-toolbox <subcommand> --help' or 'gnc-toolbox help <subcommand>' for command "
           "help.\n");

    cli_print_examples();
}

static void print_import_help(void)
{
    printf("USAGE:\n");
    printf("  gnc-toolbox import [OPTIONS] <CSV_FILE>\n\n");

    printf("DESCRIPTION:\n");
    printf("  Import CSV transaction data into a GnuCash book using configuration\n");
    printf("  rules to match and categorize transactions.\n\n");

    printf("ARGUMENTS:\n");
    printf("  <CSV_FILE>              Path to CSV file containing transaction data\n\n");

    printf("REQUIRED OPTIONS:\n");
    printf("  -b, --book <BOOK_REF>   GnuCash book path or backend URI\n");
    printf("  -c, --config <FILE>     Configuration file with import rules\n\n");

    printf("OPTIONS:\n");
    printf("  -n, --dry-run           Preview import without making changes\n");
    printf("  -D, --no-duplicates     Skip transactions that already exist\n");
    printf("  -v, --verbose           Show detailed progress information\n");
    printf("  -q, --quiet             Suppress non-essential output\n");
    printf("  -d, --debug             Enable debug output\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -V, --version           Show version information\n\n");

    printf("EXAMPLES:\n");
    printf("  gnc-toolbox import bank_statement.csv -b mybook.gnucash -c bank-rules.csv\n");
    printf("  gnc-toolbox import data.csv --book accounts.gnucash --config bank-rules.csv "
           "--dry-run\n");
    printf("  gnc-toolbox import transactions.csv -b book.gnucash -c bank-rules.csv -v -D\n");
    printf("  gnc-toolbox import data.csv --book \"mysql://user:pass@db.example.com/gnucash\" -c "
           "bank-rules.csv --dry-run\n");
}

static void print_merge_help(void)
{
    printf("USAGE:\n");
    printf("  gnc-toolbox merge [OPTIONS]\n\n");

    printf("DESCRIPTION:\n");
    printf("  Merge transactions from a specific account in one GnuCash book into\n");
    printf("  another book, using configuration rules for matching and processing.\n\n");

    printf("REQUIRED OPTIONS:\n");
    printf("  -b, --book <BOOK_REF>   Target GnuCash book path or backend URI\n");
    printf("  -c, --config <FILE>     Configuration file with merge rules\n");
    printf("  -f, --from <BOOK_REF>   Source GnuCash book path or backend URI\n");
    printf("  -a, --account <NAME>    Account name to merge from source book\n\n");

    printf("OPTIONS:\n");
    printf("  -n, --dry-run           Preview merge without making changes\n");
    printf("  -v, --verbose           Show detailed progress information\n");
    printf("  -q, --quiet             Suppress non-essential output\n");
    printf("  -d, --debug             Enable debug output\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -V, --version           Show version information\n\n");

    printf("EXAMPLES:\n");
    printf("  gnc-toolbox merge -b main.gnucash -c merge-rules.csv -f other.gnucash -a "
           "\"Assets:Checking\"\n");
    printf("  gnc-toolbox merge --book target.gnucash --config merge-rules.csv --from "
           "source.gnucash --account \"Assets:Savings\" --dry-run\n");
    printf("  gnc-toolbox merge --book \"mysql://user:pass@db.example.com/target\" --config "
           "merge-rules.csv --from archive.gnucash --account \"Expenses:Travel\" --dry-run\n");
}

/* Callbacks -----------------------------------------------------------------*/
/*** end of file ***/
