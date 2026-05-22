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

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "test_csv_params.h"
#include "../inc/match_params.h"
#include "../inc/match_params_loader.h"
#include "../inc/utilities.h"

// Test data cleanup
static void test_cleanup(void)
{
    // Remove any test files created
    unlink("test_config.csv");
    unlink("test_params.csv");
}

// Test CSV parameter structure initialization
static void test_csv_params_initialization(void)
{
    CSVParseParam_t params = {0};

    // Test default initialization
    g_assert_cmpint(params.delimiter, ==, 0);
    g_assert_cmpint(params.quote, ==, 0);
    g_assert_cmpint(params.decimal_separator, ==, 0);
    g_assert_cmpint(params.thousand_separator, ==, 0);
    g_assert_cmpint(params.skip_header_rows, ==, 0);

    // Test manual initialization
    CSVParseParam_t manual_params = {.delimiter = ';',
                                     .quote = '"',
                                     .decimal_separator = ',',
                                     .thousand_separator = '.',
                                     .skip_header_rows = 1};

    g_assert_cmpint(manual_params.delimiter, ==, ';');
    g_assert_cmpint(manual_params.quote, ==, '"');
    g_assert_cmpint(manual_params.decimal_separator, ==, ',');
    g_assert_cmpint(manual_params.thousand_separator, ==, '.');
    g_assert_cmpint(manual_params.skip_header_rows, ==, 1);
}

// Test CSV configuration file parsing
static void test_csv_config_parsing(void)
{
    // Create a test configuration file with simplified structure
    // that matches what the parser expects
    FILE *fp = fopen("test_config.csv", "w");
    g_assert_nonnull(fp);

    // Write configuration without CSVPARAMS section first (test default behavior)
    fprintf(fp, "BaseAccount,1,\" - \"\n");
    fprintf(fp, "DATE:FORMAT=DD/MM/YYYY,DESCR,AMOUNT,NUM\n");
    fprintf(fp, "Transaction 1,Account1,50,Account2,50\n");

    fclose(fp);

    // Test loading the configuration (this should work with defaults)
    ParserSetting_t *settings = mpl_load("test_config.csv");
    g_assert_nonnull(settings);

    // Verify we got some basic structure (CSV params will have defaults)
    g_assert_nonnull(settings->baseAccount);
    g_assert_cmpstr(settings->baseAccount->str, ==, "BaseAccount");
    // Sign control now handled per-column via multipliers

    g_assert_cmpint(settings->csvParams.delimiter, ==, ',');
    g_assert_cmpint(settings->csvParams.quote, ==, '"');
    g_assert_cmpint(settings->csvParams.decimal_separator, ==, '.');
    g_assert_cmpint(settings->csvParams.thousand_separator, ==, ',');
    g_assert_cmpint(settings->csvParams.skip_header_rows, ==, 0);

    // Clean up
    free_parser_settings_t(settings);
}

// Test different delimiter configurations
static void test_csv_delimiter_options(void)
{
    FILE *fp;
    ParserSetting_t *settings;

    // Test with CSVPARAMS section - need to create proper format
    fp = fopen("test_params.csv", "w");
    fprintf(fp, "CSVPARAMS,DELIMITER=|,QUOTE=\"\n");
    fprintf(fp, "BaseAccount,1,\" - \"\n");
    fprintf(fp, "DATE,DESCR,AMOUNT,NUM\n");
    fprintf(fp, "Transaction,Account1,100\n");
    fclose(fp);

    settings = mpl_load("test_params.csv");
    g_assert_nonnull(settings);

    g_assert_nonnull(settings->baseAccount);
    g_assert_cmpstr(settings->baseAccount->str, ==, "BaseAccount");
    g_assert_cmpint(settings->csvParams.delimiter, ==, '|');
    g_assert_cmpint(settings->csvParams.quote, ==, '"');
    free_parser_settings_t(settings);

    // Test basic functionality works with defaults
    fp = fopen("test_params.csv", "w");
    fprintf(fp, "TestAccount,1,\" - \"\n");
    fprintf(fp, "DATE,DESCR,AMOUNT\n");
    fprintf(fp, "Transaction,Account1,100\n");
    fclose(fp);

    settings = mpl_load("test_params.csv");
    g_assert_nonnull(settings);
    g_assert_nonnull(settings->baseAccount);
    g_assert_cmpstr(settings->baseAccount->str, ==, "TestAccount");
    g_assert_cmpint(settings->csvParams.delimiter, ==, ',');
    free_parser_settings_t(settings);
}

// Test quote character configurations
static void test_csv_quote_options(void)
{
    FILE *fp;
    ParserSetting_t *settings;

    // Test basic configuration loading
    fp = fopen("test_params.csv", "w");
    fprintf(fp, "QuoteTestAccount,1,\" - \"\n");
    fprintf(fp, "DATE,DESCR,AMOUNT\n");
    fprintf(fp, "Transaction,Account1,100\n");
    fclose(fp);

    settings = mpl_load("test_params.csv");
    g_assert_nonnull(settings);
    g_assert_nonnull(settings->baseAccount);
    g_assert_cmpstr(settings->baseAccount->str, ==, "QuoteTestAccount");

    g_assert_cmpint(settings->csvParams.quote, ==, '"');
    free_parser_settings_t(settings);
}

// Test decimal and thousand separator configurations
static void test_csv_separator_options(void)
{
    FILE *fp;
    ParserSetting_t *settings;

    // Test default separator configuration
    fp = fopen("test_params.csv", "w");
    fprintf(fp, "SeparatorTestAccount,1,\" - \"\n");
    fprintf(fp, "DATE,DESCR,AMOUNT\n");
    fprintf(fp, "Transaction,Account1,100\n");
    fclose(fp);

    settings = mpl_load("test_params.csv");
    g_assert_nonnull(settings);
    g_assert_nonnull(settings->baseAccount);
    g_assert_cmpstr(settings->baseAccount->str, ==, "SeparatorTestAccount");

    g_assert_cmpint(settings->csvParams.decimal_separator, ==, '.');
    g_assert_cmpint(settings->csvParams.thousand_separator, ==, ',');
    free_parser_settings_t(settings);

    fp = fopen("test_params.csv", "w");
    fprintf(fp, "CSVPARAMS,DELIMITER=;,QUOTE=\",\"DECIMAL_SEP=,\",THOUSAND_SEP=.,SKIP_ROWS=2\n");
    fprintf(fp, "SeparatorOverrideAccount, -\n");
    fprintf(fp, "DATE,DESCR,AMOUNT\n");
    fprintf(fp, "Transaction,Account1,100\n");
    fclose(fp);

    settings = mpl_load("test_params.csv");
    g_assert_nonnull(settings);
    g_assert_nonnull(settings->baseAccount);
    g_assert_cmpstr(settings->baseAccount->str, ==, "SeparatorOverrideAccount");
    g_assert_cmpint(settings->csvParams.delimiter, ==, ';');
    g_assert_cmpint(settings->csvParams.quote, ==, '"');
    g_assert_cmpint(settings->csvParams.decimal_separator, ==, ',');
    g_assert_cmpint(settings->csvParams.thousand_separator, ==, '.');
    g_assert_cmpint(settings->csvParams.skip_header_rows, ==, 2);
    free_parser_settings_t(settings);
}

// Test header and skip row configurations
static void test_csv_header_options(void)
{
    FILE *fp;
    ParserSetting_t *settings;

    // Test default header options
    fp = fopen("test_params.csv", "w");
    fprintf(fp, "HeaderTestAccount,1,\" - \"\n");
    fprintf(fp, "DATE,DESCR,AMOUNT\n");
    fprintf(fp, "Transaction,Account1,100\n");
    fclose(fp);

    settings = mpl_load("test_params.csv");
    g_assert_nonnull(settings);
    g_assert_nonnull(settings->baseAccount);
    g_assert_cmpstr(settings->baseAccount->str, ==, "HeaderTestAccount");

    g_assert_cmpint(settings->csvParams.skip_header_rows, ==, 0);
    free_parser_settings_t(settings);
}

// Test configuration without CSVPARAMS section (should use defaults)
static void test_csv_default_params(void)
{
    // Create config file without CSVPARAMS section
    FILE *fp = fopen("test_params.csv", "w");
    fprintf(fp, "DefaultTestAccount,1,\" - \"\n");
    fprintf(fp, "DATE,DESCR,AMOUNT,NUM\n");
    fprintf(fp, "Transaction,Account1,100\n");
    fclose(fp);

    ParserSetting_t *settings = mpl_load("test_params.csv");
    g_assert_nonnull(settings);

    // Verify basic loading worked
    g_assert_nonnull(settings->baseAccount);
    g_assert_cmpstr(settings->baseAccount->str, ==, "DefaultTestAccount");

    g_assert_cmpint(settings->csvParams.delimiter, ==, ',');
    g_assert_cmpint(settings->csvParams.quote, ==, '"');
    g_assert_cmpint(settings->csvParams.decimal_separator, ==, '.');
    g_assert_cmpint(settings->csvParams.thousand_separator, ==, ',');
    g_assert_cmpint(settings->csvParams.skip_header_rows, ==, 0);

    free_parser_settings_t(settings);
}

// Test malformed CSV parameter configurations
static void test_csv_malformed_params(void)
{
    FILE *fp;
    ParserSetting_t *settings;

    // Test with empty file (should handle gracefully)
    fp = fopen("test_params.csv", "w");
    fclose(fp);

    settings = mpl_load("test_params.csv");
    g_assert_null(settings);

    // Test with minimal valid configuration
    fp = fopen("test_params.csv", "w");
    fprintf(fp, "MalformedTestAccount,1,\" - \"\n");
    fclose(fp);

    settings = mpl_load("test_params.csv");
    g_assert_null(settings);
}

static void test_csv_filter_pattern_and_regex_options(void)
{
    FILE *fp = fopen("test_params.csv", "w");
    g_assert_nonnull(fp);
    fprintf(fp, "FILTEROUT,PATTERN,*PENDING*\n");
    fprintf(fp, "FILTEROUT,REGEX,^CARD [0-9]+$\n");
    fprintf(fp, "FilterTestAccount, -\n");
    fprintf(fp, "DATE,DESCR,AMOUNT\n");
    fprintf(fp, "Coffee,Expenses:Dining,100\n");
    fclose(fp);

    ParserSetting_t *settings = mpl_load("test_params.csv");
    g_assert_nonnull(settings);
    g_assert_cmpuint(g_list_length(settings->filters), ==, 2);

    TxFilter_t *pattern_filter = (TxFilter_t *)settings->filters->data;
    TxFilter_t *regex_filter = (TxFilter_t *)settings->filters->next->data;

    g_assert_cmpint(pattern_filter->type, ==, MATCH_TYPE_PATTERN);
    g_assert_cmpstr(pattern_filter->value.gstring->str, ==, "*PENDING*");
    g_assert_cmpint(regex_filter->type, ==, MATCH_TYPE_REGEX);
    g_assert_cmpstr(regex_filter->value.gstring->str, ==, "^CARD [0-9]+$");

    free_parser_settings_t(settings);
}

static void test_csv_filter_rejects_invalid_regex(void)
{
    FILE *fp = fopen("test_params.csv", "w");
    g_assert_nonnull(fp);
    fprintf(fp, "FILTEROUT,REGEX,*not-regex\n");
    fprintf(fp, "FilterTestAccount, -\n");
    fprintf(fp, "DATE,DESCR,AMOUNT\n");
    fclose(fp);

    ParserSetting_t *settings = mpl_load("test_params.csv");
    g_assert_null(settings);
}

static void test_csv_unmatched_account_option(void)
{
    FILE *fp = fopen("test_params.csv", "w");
    g_assert_nonnull(fp);
    fprintf(fp, "Checking Account, -\n");
    fprintf(fp, "UNMATCHED,Expenses:Uncategorized\n");
    fprintf(fp, "DATE,DESCR,AMOUNT\n");
    fprintf(fp, "Coffee,Expenses:Dining,100\n");
    fclose(fp);

    ParserSetting_t *settings = mpl_load("test_params.csv");
    g_assert_nonnull(settings);
    g_assert_nonnull(settings->unmatchedAccount);
    g_assert_cmpstr(settings->unmatchedAccount->str, ==, "Expenses:Uncategorized");
    g_assert_cmpuint(g_list_length(settings->cols), ==, 3);
    g_assert_cmpuint(g_list_length(settings->matches), ==, 1);

    free_parser_settings_t(settings);
}

void test_csv_params_register(void)
{
    // Setup and cleanup functions
    g_test_add_func("/csv_params/cleanup", test_cleanup);

    // Register test cases
    g_test_add_func("/csv_params/initialization", test_csv_params_initialization);
    g_test_add_func("/csv_params/config_parsing", test_csv_config_parsing);
    g_test_add_func("/csv_params/delimiter_options", test_csv_delimiter_options);
    g_test_add_func("/csv_params/quote_options", test_csv_quote_options);
    g_test_add_func("/csv_params/separator_options", test_csv_separator_options);
    g_test_add_func("/csv_params/header_options", test_csv_header_options);
    g_test_add_func("/csv_params/default_params", test_csv_default_params);
    g_test_add_func("/csv_params/malformed_params", test_csv_malformed_params);
    g_test_add_func("/csv_params/filter_pattern_and_regex_options",
                    test_csv_filter_pattern_and_regex_options);
    g_test_add_func("/csv_params/filter_rejects_invalid_regex",
                    test_csv_filter_rejects_invalid_regex);
    g_test_add_func("/csv_params/unmatched_account_option", test_csv_unmatched_account_option);

    // Final cleanup
    g_test_add_func("/csv_params/final_cleanup", test_cleanup);
}
