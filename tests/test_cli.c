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

#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>

#include "test_cli.h"
#include "../inc/cli.h"
#include "../inc/output.h"

static const char *CLI_TEST_CONFIG = "test_cli_config.cfg";
static const char *CLI_TEST_CSV = "test_cli_transactions.csv";

static void write_file(const char *path, const char *contents)
{
    FILE *fp = fopen(path, "w");
    g_assert_nonnull(fp);
    fputs(contents, fp);
    fclose(fp);
}

static void remove_cli_test_files(void)
{
    unlink(CLI_TEST_CONFIG);
    unlink(CLI_TEST_CSV);
}

static gchar *capture_stream(int stream_fd, FILE *stream, void (*fn)(void *), void *user_data)
{
    gchar template[] = "/tmp/gnc-toolbox-cli-XXXXXX";
    int capture_fd = g_mkstemp(template);
    g_assert_cmpint(capture_fd, >=, 0);

    int saved_stream = dup(stream_fd);
    g_assert_cmpint(saved_stream, >=, 0);

    fflush(stream);
    dup2(capture_fd, stream_fd);

    fn(user_data);

    fflush(stream);
    dup2(saved_stream, stream_fd);
    close(saved_stream);
    close(capture_fd);

    gchar *contents = NULL;
    gsize length = 0;
    GError *error = NULL;
    g_file_get_contents(template, &contents, &length, &error);
    g_assert_no_error(error);
    g_assert_nonnull(contents);

    unlink(template);
    return contents;
}

static gchar *capture_stdout(void (*fn)(void *), void *user_data)
{
    return capture_stream(STDOUT_FILENO, stdout, fn, user_data);
}

static gchar *capture_stderr(void (*fn)(void *), void *user_data)
{
    return capture_stream(STDERR_FILENO, stderr, fn, user_data);
}

static void print_help_wrapper(void *user_data)
{
    cli_print_help(GPOINTER_TO_INT(user_data));
}

static void print_version_wrapper(void *user_data)
{
    (void)user_data;
    cli_print_version();
}

static void emit_verbose_output_wrapper(void *user_data)
{
    (void)user_data;
    cli_configure_output_level(CLI_VERBOSITY_VERBOSE);
    OUT_INFO("info visible\n");
    OUT_VERBOSE("verbose visible\n");
    OUT_DEBUG("debug hidden\n");
}

static void emit_normal_summary_output_wrapper(void *user_data)
{
    (void)user_data;
    cli_configure_output_level(CLI_VERBOSITY_NORMAL);
    OUT_SUMMARY("summary visible\n");
    OUT_INFO("info hidden\n");
}

static void emit_normal_warning_output_wrapper(void *user_data)
{
    (void)user_data;
    cli_configure_output_level(CLI_VERBOSITY_NORMAL);
    OUT_WARNING("warning visible\n");
}

static void emit_quiet_output_wrapper(void *user_data)
{
    (void)user_data;
    cli_configure_output_level(CLI_VERBOSITY_QUIET);
    OUT_WARNING("warning hidden\n");
    OUT_ERROR("error visible\n");
}

static void test_cli_parse_no_args(void)
{
    CliArgs_t args;
    char *argv[] = {"gnc-toolbox", NULL};

    g_assert_true(cli_parse_args(1, argv, &args));
    g_assert_cmpint(args.operation, ==, CLI_OPERATION_HELP);
    g_assert_false(args.help_requested);
    cli_cleanup_args(&args);
}

static void test_cli_parse_global_flags(void)
{
    CliArgs_t args;
    char *help_argv[] = {"gnc-toolbox", "--help", NULL};
    char *version_argv[] = {"gnc-toolbox", "--version", NULL};

    g_assert_true(cli_parse_args(2, help_argv, &args));
    g_assert_cmpint(args.operation, ==, CLI_OPERATION_HELP);
    g_assert_true(args.help_requested);
    cli_cleanup_args(&args);

    g_assert_true(cli_parse_args(2, version_argv, &args));
    g_assert_cmpint(args.operation, ==, CLI_OPERATION_VERSION);
    g_assert_true(args.version_requested);
    cli_cleanup_args(&args);
}

static void test_cli_parse_import_args(void)
{
    CliArgs_t args;
    char *argv[] = {
        "gnc-toolbox", "import",           "--book",    "mysql://user:pass@localhost/gnucash",
        "--config",    "bank.cfg",         "--dry-run", "-D",
        "--verbose",   "transactions.csv", NULL};

    g_assert_true(cli_parse_args(10, argv, &args));
    g_assert_cmpint(args.operation, ==, CLI_OPERATION_IMPORT);
    g_assert_cmpstr(args.book_ref, ==, "mysql://user:pass@localhost/gnucash");
    g_assert_cmpstr(args.config_file, ==, "bank.cfg");
    g_assert_cmpstr(args.csv_file, ==, "transactions.csv");
    g_assert_true(args.dry_run);
    g_assert_true(args.no_duplicates);
    g_assert_cmpint(args.verbosity, ==, CLI_VERBOSITY_VERBOSE);

    cli_cleanup_args(&args);
    g_assert_null(args.book_ref);
    g_assert_null(args.config_file);
    g_assert_null(args.csv_file);
}

static void test_cli_parse_merge_args(void)
{
    CliArgs_t args;
    char *argv[] = {
        "gnc-toolbox", "merge",           "--book",    "target.gnucash",
        "--config",    "merge.cfg",       "--from",    "mysql://user:pass@localhost/source",
        "--account",   "Assets:Checking", "--dry-run", "--debug",
        NULL};

    g_assert_true(cli_parse_args(12, argv, &args));
    g_assert_cmpint(args.operation, ==, CLI_OPERATION_MERGE);
    g_assert_cmpstr(args.book_ref, ==, "target.gnucash");
    g_assert_cmpstr(args.config_file, ==, "merge.cfg");
    g_assert_cmpstr(args.source_book_ref, ==, "mysql://user:pass@localhost/source");
    g_assert_cmpstr(args.source_account, ==, "Assets:Checking");
    g_assert_true(args.dry_run);
    g_assert_cmpint(args.verbosity, ==, CLI_VERBOSITY_DEBUG);
    cli_cleanup_args(&args);
}

static void test_cli_parse_quiet_args(void)
{
    CliArgs_t args;
    char *import_argv[] = {"gnc-toolbox", "import",           "-q",
                           "--book",      "book.gnucash",     "--config",
                           "bank.cfg",    "transactions.csv", NULL};
    char *merge_argv[] = {"gnc-toolbox", "merge",           "--book", "target.gnucash",
                          "--config",    "merge.cfg",       "--from", "source.gnucash",
                          "--account",   "Assets:Checking", "-q",     NULL};

    g_assert_true(cli_parse_args(8, import_argv, &args));
    g_assert_cmpint(args.operation, ==, CLI_OPERATION_IMPORT);
    g_assert_cmpint(args.verbosity, ==, CLI_VERBOSITY_QUIET);
    cli_cleanup_args(&args);

    g_assert_true(cli_parse_args(11, merge_argv, &args));
    g_assert_cmpint(args.operation, ==, CLI_OPERATION_MERGE);
    g_assert_cmpint(args.verbosity, ==, CLI_VERBOSITY_QUIET);
    cli_cleanup_args(&args);
}

static void test_cli_parse_subcommand_help_and_version(void)
{
    CliArgs_t args;
    char *help_argv[] = {"gnc-toolbox", "help", "import", NULL};
    char *merge_help_argv[] = {"gnc-toolbox", "help", "merge", NULL};
    char *version_argv[] = {"gnc-toolbox", "import", "--version", NULL};

    g_assert_true(cli_parse_args(3, help_argv, &args));
    g_assert_cmpint(args.operation, ==, CLI_OPERATION_IMPORT);
    g_assert_true(args.help_requested);
    cli_cleanup_args(&args);

    g_assert_true(cli_parse_args(3, merge_help_argv, &args));
    g_assert_cmpint(args.operation, ==, CLI_OPERATION_MERGE);
    g_assert_true(args.help_requested);
    cli_cleanup_args(&args);

    g_assert_true(cli_parse_args(3, version_argv, &args));
    g_assert_cmpint(args.operation, ==, CLI_OPERATION_VERSION);
    g_assert_true(args.version_requested);
    cli_cleanup_args(&args);
}

static void test_cli_parse_invalid_inputs(void)
{
    CliArgs_t args;
    char *unknown_argv[] = {"gnc-toolbox", "unknown", NULL};
    char *bad_option_argv[] = {"gnc-toolbox", "merge", "--bad-option", NULL};
    char *bad_help_argv[] = {"gnc-toolbox", "help", "unknown", NULL};
    char *extra_import_argv[] = {"gnc-toolbox",  "import",   "--book",
                                 "book.gnucash", "--config", "rules.csv",
                                 "one.csv",      "two.csv",  NULL};
    char *extra_merge_argv[] = {"gnc-toolbox", "merge", "unexpected", NULL};

    out_setLevel(OUT_LEVEL_FATAL);
    g_assert_false(cli_parse_args(2, unknown_argv, &args));

    opterr = 0;
    g_assert_false(cli_parse_args(3, bad_option_argv, &args));
    g_assert_false(cli_parse_args(3, bad_help_argv, &args));
    g_assert_false(cli_parse_args(8, extra_import_argv, &args));
    g_assert_false(cli_parse_args(3, extra_merge_argv, &args));
    opterr = 1;
    out_setLevel(OUT_LEVEL_WARNING);
}

static void test_cli_parse_rejects_oversized_arg(void)
{
    CliArgs_t args;
    char oversized[600];
    memset(oversized, 'A', sizeof(oversized) - 1);
    oversized[sizeof(oversized) - 1] = '\0';

    char *argv[] = {"gnc-toolbox", "import", "--book",   oversized,
                    "--config",    "cfg",    "data.csv", NULL};

    out_setLevel(OUT_LEVEL_FATAL);
    g_assert_false(cli_parse_args(7, argv, &args));
    out_setLevel(OUT_LEVEL_WARNING);
}

static void test_cli_configure_output_level(void)
{
    gchar *normal_output = capture_stdout(emit_normal_summary_output_wrapper, NULL);
    g_assert_nonnull(strstr(normal_output, "summary visible"));
    g_assert_null(strstr(normal_output, "info hidden"));
    g_free(normal_output);

    gchar *normal_error_output = capture_stderr(emit_normal_warning_output_wrapper, NULL);
    g_assert_nonnull(strstr(normal_error_output, "warning visible"));
    g_free(normal_error_output);

    gchar *verbose_output = capture_stdout(emit_verbose_output_wrapper, NULL);
    g_assert_nonnull(strstr(verbose_output, "info visible"));
    g_assert_nonnull(strstr(verbose_output, "verbose visible"));
    g_assert_null(strstr(verbose_output, "debug hidden"));
    g_free(verbose_output);

    gchar *quiet_output = capture_stderr(emit_quiet_output_wrapper, NULL);
    g_assert_nonnull(strstr(quiet_output, "error visible"));
    g_assert_null(strstr(quiet_output, "warning hidden"));
    g_free(quiet_output);

    out_setLevel(OUT_LEVEL_WARNING);
}

static void test_cli_validate_import_args(void)
{
    CliArgs_t args = {0};

    write_file(CLI_TEST_CONFIG, "Checking Account, -\nDATE,DESCR,AMOUNT\n");
    write_file(CLI_TEST_CSV, "2024-01-01,Test,1.00\n");

    args.operation = CLI_OPERATION_IMPORT;
    args.book_ref = strdup("mysql://user:pass@localhost/gnucash");
    args.config_file = strdup(CLI_TEST_CONFIG);
    args.csv_file = strdup(CLI_TEST_CSV);

    g_assert_true(cli_validate_args(&args));
    cli_cleanup_args(&args);
    remove_cli_test_files();
}

static void test_cli_validate_merge_args(void)
{
    CliArgs_t args = {0};

    write_file(CLI_TEST_CONFIG, "Checking Account, -\nDATE,DESCR,AMOUNT\n");

    args.operation = CLI_OPERATION_MERGE;
    args.book_ref = strdup("mysql://user:pass@localhost/target");
    args.config_file = strdup(CLI_TEST_CONFIG);
    args.source_book_ref = strdup("mysql://user:pass@localhost/source");
    args.source_account = strdup("Assets:Checking");

    g_assert_true(cli_validate_args(&args));
    cli_cleanup_args(&args);
    remove_cli_test_files();
}

static void test_cli_validate_failures(void)
{
    CliArgs_t args = {0};

    out_setLevel(OUT_LEVEL_FATAL);

    args.operation = CLI_OPERATION_IMPORT;
    g_assert_false(cli_validate_args(&args));

    args.operation = CLI_OPERATION_MERGE;
    args.book_ref = strdup("mysql://user:pass@localhost/target");
    args.config_file = strdup("missing.cfg");
    args.source_book_ref = strdup("mysql://user:pass@localhost/source");
    args.source_account = strdup("Invalid/Account");
    g_assert_false(cli_validate_args(&args));

    cli_cleanup_args(&args);
    out_setLevel(OUT_LEVEL_WARNING);
}

static void test_cli_print_help_and_version(void)
{
    gchar *general_help = capture_stdout(print_help_wrapper, GINT_TO_POINTER(CLI_OPERATION_HELP));
    g_assert_nonnull(strstr(general_help, "SUBCOMMANDS:"));
    g_assert_nonnull(strstr(general_help, "gnc-toolbox import transactions.csv"));
    g_assert_nonnull(strstr(general_help, "gnc-toolbox help [import|merge]"));
    g_free(general_help);

    gchar *import_help = capture_stdout(print_help_wrapper, GINT_TO_POINTER(CLI_OPERATION_IMPORT));
    g_assert_nonnull(strstr(import_help, "gnc-toolbox import [OPTIONS] <CSV_FILE>"));
    g_assert_nonnull(strstr(import_help, "-D, --no-duplicates"));
    g_assert_nonnull(strstr(import_help, "BOOK_REF"));
    g_free(import_help);

    gchar *merge_help = capture_stdout(print_help_wrapper, GINT_TO_POINTER(CLI_OPERATION_MERGE));
    g_assert_nonnull(strstr(merge_help, "gnc-toolbox merge [OPTIONS]"));
    g_assert_nonnull(strstr(merge_help, "--from <BOOK_REF>"));
    g_assert_nonnull(strstr(merge_help, "--account <NAME>"));
    g_free(merge_help);

    gchar *version = capture_stdout(print_version_wrapper, NULL);
    g_assert_nonnull(strstr(version, "gnc-toolbox version"));
    g_assert_nonnull(strstr(version, "License: GNU AGPLv3 or later"));
    g_assert_nonnull(strstr(version, "SPDX: AGPL-3.0-or-later"));
    g_assert_nonnull(strstr(version, "Repository: https://github.com/dariocorsetti/gnc-toolbox"));
    g_free(version);
}

void test_cli_register(void)
{
    g_test_add_func("/cli/parse_no_args", test_cli_parse_no_args);
    g_test_add_func("/cli/parse_global_flags", test_cli_parse_global_flags);
    g_test_add_func("/cli/parse_import_args", test_cli_parse_import_args);
    g_test_add_func("/cli/parse_merge_args", test_cli_parse_merge_args);
    g_test_add_func("/cli/parse_quiet_args", test_cli_parse_quiet_args);
    g_test_add_func("/cli/parse_subcommand_help_and_version",
                    test_cli_parse_subcommand_help_and_version);
    g_test_add_func("/cli/parse_invalid_inputs", test_cli_parse_invalid_inputs);
    g_test_add_func("/cli/parse_rejects_oversized_arg", test_cli_parse_rejects_oversized_arg);
    g_test_add_func("/cli/configure_output_level", test_cli_configure_output_level);
    g_test_add_func("/cli/validate_import_args", test_cli_validate_import_args);
    g_test_add_func("/cli/validate_merge_args", test_cli_validate_merge_args);
    g_test_add_func("/cli/validate_failures", test_cli_validate_failures);
    g_test_add_func("/cli/print_help_and_version", test_cli_print_help_and_version);
}
