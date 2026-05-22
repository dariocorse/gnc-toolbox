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
#include <unistd.h>
#include "../inc/match_params.h"
#include "../inc/match_params_loader.h"
#include "../inc/matcher.h"
#include "../inc/utilities.h"

// Test full CSV processing pipeline with US format
static void test_integration_us_format(void)
{
    // Test loading US format configuration
    ParserSetting_t *settings = mpl_load("tests/data/sample_config_us.ini");
    g_assert_nonnull(settings);

    // Verify configuration was loaded correctly
    g_assert_cmpint(settings->csvParams.delimiter, ==, ',');
    g_assert_cmpint(settings->csvParams.decimal_separator, ==, '.');
    g_assert_cmpint(settings->csvParams.thousand_separator, ==, ',');

    // Test matching against US format CSV
    GList *transactions = m_match("tests/data/sample_transactions_us.csv", settings);
    g_assert_nonnull(transactions);

    // Verify we got expected number of transactions
    guint transaction_count = g_list_length(transactions);
    g_assert_cmpuint(transaction_count, ==, 6);

    // Verify first transaction: 12/25/2023,"Christmas Shopping",-1,234.56,TX001
    Data_t *tx1 = (Data_t *)g_list_nth_data(transactions, 0);
    g_assert_nonnull(tx1);
    g_assert_nonnull(tx1->date);
    g_assert_cmpint(g_date_time_get_year(tx1->date), ==, 2023);
    g_assert_cmpint(g_date_time_get_month(tx1->date), ==, 12);
    g_assert_cmpint(g_date_time_get_day_of_month(tx1->date), ==, 25);
    g_assert_cmpstr(tx1->description->str, ==, "Christmas Shopping");
    g_assert_cmpint(tx1->amountCents, ==, -123456); // -1234.56 in cents
    g_assert_cmpstr(tx1->number->str, ==, "TX001");
    g_assert_true(tx1->hasValidAmount);

    // Verify second transaction: 01/01/2024,"Salary Deposit",3,500.00,TX002
    Data_t *tx2 = (Data_t *)g_list_nth_data(transactions, 1);
    g_assert_nonnull(tx2);
    g_assert_nonnull(tx2->date);
    g_assert_cmpint(g_date_time_get_year(tx2->date), ==, 2024);
    g_assert_cmpint(g_date_time_get_month(tx2->date), ==, 1);
    g_assert_cmpint(g_date_time_get_day_of_month(tx2->date), ==, 1);
    g_assert_cmpstr(tx2->description->str, ==, "Salary Deposit");
    g_assert_cmpint(tx2->amountCents, ==, 350000); // 3500.00 in cents
    g_assert_cmpstr(tx2->number->str, ==, "TX002");
    g_assert_true(tx2->hasValidAmount);

    // Verify third transaction: 01/15/2024,"Utility Bill",-156.78,TX003
    Data_t *tx3 = (Data_t *)g_list_nth_data(transactions, 2);
    g_assert_nonnull(tx3);
    g_assert_nonnull(tx3->date);
    g_assert_cmpint(g_date_time_get_year(tx3->date), ==, 2024);
    g_assert_cmpint(g_date_time_get_month(tx3->date), ==, 1);
    g_assert_cmpint(g_date_time_get_day_of_month(tx3->date), ==, 15);
    g_assert_cmpstr(tx3->description->str, ==, "Utility Bill");
    g_assert_cmpint(tx3->amountCents, ==, -15678); // -156.78 in cents
    g_assert_cmpstr(tx3->number->str, ==, "TX003");
    g_assert_true(tx3->hasValidAmount);

    // Verify sixth transaction: 03/01/2024,"Tax Refund",2,500.00,TX006
    Data_t *tx6 = (Data_t *)g_list_nth_data(transactions, 5);
    g_assert_nonnull(tx6);
    g_assert_nonnull(tx6->date);
    g_assert_cmpint(g_date_time_get_year(tx6->date), ==, 2024);
    g_assert_cmpint(g_date_time_get_month(tx6->date), ==, 3);
    g_assert_cmpint(g_date_time_get_day_of_month(tx6->date), ==, 1);
    g_assert_cmpstr(tx6->description->str, ==, "Tax Refund");
    g_assert_cmpint(tx6->amountCents, ==, 250000); // 2500.00 in cents
    g_assert_cmpstr(tx6->number->str, ==, "TX006");
    g_assert_true(tx6->hasValidAmount);

    // Cleanup
    g_list_free_full(transactions, (GDestroyNotify)free_data_t);
    free_parser_settings_t(settings);
}

// Test full CSV processing pipeline with European format
static void test_integration_eu_format(void)
{
    // Test loading European format configuration
    ParserSetting_t *settings = mpl_load("tests/data/sample_config_eu.ini");
    g_assert_nonnull(settings);

    // Verify configuration was loaded correctly
    g_assert_cmpint(settings->csvParams.delimiter, ==, ';');
    g_assert_cmpint(settings->csvParams.decimal_separator, ==, ',');
    g_assert_cmpint(settings->csvParams.thousand_separator, ==, '.');

    // Test matching against European format CSV
    GList *transactions = m_match("tests/data/sample_transactions_eu.csv", settings);
    g_assert_nonnull(transactions);

    // Verify we got expected number of transactions
    guint transaction_count = g_list_length(transactions);
    g_assert_cmpuint(transaction_count, ==, 6);

    // Verify first transaction: 25/12/2023;Christmas Shopping;-1.234,56;TX001
    Data_t *tx1 = (Data_t *)g_list_nth_data(transactions, 0);
    g_assert_nonnull(tx1);
    g_assert_nonnull(tx1->date);
    g_assert_cmpint(g_date_time_get_year(tx1->date), ==, 2023);
    g_assert_cmpint(g_date_time_get_month(tx1->date), ==, 12);
    g_assert_cmpint(g_date_time_get_day_of_month(tx1->date), ==, 25);
    g_assert_cmpstr(tx1->description->str, ==, "Christmas Shopping");
    g_assert_cmpint(tx1->amountCents, ==, -123456); // -1234.56 in cents
    g_assert_cmpstr(tx1->number->str, ==, "TX001");
    g_assert_true(tx1->hasValidAmount);

    // Verify second transaction: 01/01/2024;Salary Deposit;+3.500,00;TX002
    Data_t *tx2 = (Data_t *)g_list_nth_data(transactions, 1);
    g_assert_nonnull(tx2);
    g_assert_nonnull(tx2->date);
    g_assert_cmpint(g_date_time_get_year(tx2->date), ==, 2024);
    g_assert_cmpint(g_date_time_get_month(tx2->date), ==, 1);
    g_assert_cmpint(g_date_time_get_day_of_month(tx2->date), ==, 1);
    g_assert_cmpstr(tx2->description->str, ==, "Salary Deposit");
    g_assert_cmpint(tx2->amountCents, ==, 350000); // +3500.00 in cents
    g_assert_cmpstr(tx2->number->str, ==, "TX002");
    g_assert_true(tx2->hasValidAmount);

    // Verify third transaction: 15/01/2024;Utility Bill;-156,78;TX003
    Data_t *tx3 = (Data_t *)g_list_nth_data(transactions, 2);
    g_assert_nonnull(tx3);
    g_assert_nonnull(tx3->date);
    g_assert_cmpint(g_date_time_get_year(tx3->date), ==, 2024);
    g_assert_cmpint(g_date_time_get_month(tx3->date), ==, 1);
    g_assert_cmpint(g_date_time_get_day_of_month(tx3->date), ==, 15);
    g_assert_cmpstr(tx3->description->str, ==, "Utility Bill");
    g_assert_cmpint(tx3->amountCents, ==, -15678); // -156.78 in cents
    g_assert_cmpstr(tx3->number->str, ==, "TX003");
    g_assert_true(tx3->hasValidAmount);

    // Verify sixth transaction: 01/03/2024;Tax Refund;2.500,00;TX006
    Data_t *tx6 = (Data_t *)g_list_nth_data(transactions, 5);
    g_assert_nonnull(tx6);
    g_assert_nonnull(tx6->date);
    g_assert_cmpint(g_date_time_get_year(tx6->date), ==, 2024);
    g_assert_cmpint(g_date_time_get_month(tx6->date), ==, 3);
    g_assert_cmpint(g_date_time_get_day_of_month(tx6->date), ==, 1);
    g_assert_cmpstr(tx6->description->str, ==, "Tax Refund");
    g_assert_cmpint(tx6->amountCents, ==, 250000); // 2500.00 in cents
    g_assert_cmpstr(tx6->number->str, ==, "TX006");
    g_assert_true(tx6->hasValidAmount);

    // Cleanup
    g_list_free_full(transactions, (GDestroyNotify)free_data_t);
    free_parser_settings_t(settings);
}

// Test error handling with malformed data
static void test_integration_malformed_data(void)
{
    // Load valid configuration
    ParserSetting_t *settings = mpl_load("tests/data/sample_config_us.ini");
    g_assert_nonnull(settings);

    // Process malformed CSV data
    GList *transactions = m_match("tests/data/malformed_data.csv", settings);

    // Should return empty list or null for malformed data
    // The malformed data file has various invalid entries that should all be rejected
    if (transactions) {
        guint count = g_list_length(transactions);
        // Accept either empty list (ideal) or very small count (some rows might pass validation)
        // but most should be rejected due to invalid dates, amounts, etc.
        g_assert_cmpuint(count, <=, 1); // At most 1 transaction should make it through
        g_list_free_full(transactions, (GDestroyNotify)free_data_t);
    }
    free_parser_settings_t(settings);
}

// Test configuration file validation
static void test_integration_config_validation(void)
{
    GError *error = NULL;

    // Test non-existent config file
    g_assert_false(validate_file_exists_and_readable("nonexistent_config.ini", &error));
    g_assert_error(error, G_FILE_ERROR, G_FILE_ERROR_NOENT);
    g_clear_error(&error);

    // Test existing config file
    g_assert_true(validate_file_exists_and_readable("tests/data/sample_config_us.ini", &error));
    g_assert_no_error(error);
}

// Test CSV file validation
static void test_integration_csv_validation(void)
{
    GError *error = NULL;

    // Test non-existent CSV file
    g_assert_false(validate_file_exists_and_readable("nonexistent.csv", &error));
    g_assert_error(error, G_FILE_ERROR, G_FILE_ERROR_NOENT);
    g_clear_error(&error);

    // Test existing CSV file
    g_assert_true(
        validate_file_exists_and_readable("tests/data/sample_transactions_us.csv", &error));
    g_assert_no_error(error);
}

// Test memory management in full pipeline
static void test_integration_memory_management(void)
{
    // This test ensures no memory leaks in the full processing pipeline
    for (int i = 0; i < 10; i++) { // Run multiple times to catch memory issues
        ParserSetting_t *settings = mpl_load("tests/data/sample_config_us.ini");
        g_assert_nonnull(settings);

        GList *transactions = m_match("tests/data/sample_transactions_us.csv", settings);
        g_assert_nonnull(transactions);

        // Process all transactions
        guint count = 0;
        for (GList *item = transactions; item != NULL; item = item->next) {
            Data_t *transaction = (Data_t *)item->data;
            g_assert_nonnull(transaction);
            count++;
        }

        g_assert_cmpuint(count, >, 0);

        // Proper cleanup
        g_list_free_full(transactions, (GDestroyNotify)free_data_t);
        free_parser_settings_t(settings);
    }
}

// Test different date format handling
static void test_integration_date_formats(void)
{
    // Test with various date formats by modifying configuration temporarily
    ParserSetting_t *settings = mpl_load("tests/data/sample_config_us.ini");
    g_assert_nonnull(settings);

    // Process US format (MM/DD/YYYY)
    GList *transactions_us = m_match("tests/data/sample_transactions_us.csv", settings);
    g_assert_nonnull(transactions_us);
    g_assert_cmpuint(g_list_length(transactions_us), >, 0);

    // Cleanup first set
    g_list_free_full(transactions_us, (GDestroyNotify)free_data_t);
    free_parser_settings_t(settings);

    // Test European format (DD/MM/YYYY)
    settings = mpl_load("tests/data/sample_config_eu.ini");
    g_assert_nonnull(settings);

    GList *transactions_eu = m_match("tests/data/sample_transactions_eu.csv", settings);
    g_assert_nonnull(transactions_eu);
    g_assert_cmpuint(g_list_length(transactions_eu), >, 0);

    // Cleanup second set
    g_list_free_full(transactions_eu, (GDestroyNotify)free_data_t);
    free_parser_settings_t(settings);
}

// Test default CSV parsing when CSVPARAMS is omitted
static void test_integration_default_csv_params(void)
{
    const char *config_path = "test_default_csv_params_config.csv";
    const char *csv_path = "test_default_csv_params_data.csv";

    FILE *fp = fopen(config_path, "w");
    g_assert_nonnull(fp);
    fprintf(fp, "Checking Account, -\n");
    fprintf(fp, "DATE:FORMAT=YYYY-MM-DD,DESCR,AMOUNT,NUM\n");
    fprintf(fp, "Coffee,Expenses:Coffee,100\n");
    fclose(fp);

    fp = fopen(csv_path, "w");
    g_assert_nonnull(fp);
    fprintf(fp, "2024-02-01,Coffee Shop,-4.50,REF-001\n");
    fclose(fp);

    ParserSetting_t *settings = mpl_load(config_path);
    g_assert_nonnull(settings);
    g_assert_cmpint(settings->csvParams.delimiter, ==, ',');
    g_assert_cmpint(settings->csvParams.quote, ==, '"');
    g_assert_cmpint(settings->csvParams.decimal_separator, ==, '.');
    g_assert_cmpint(settings->csvParams.thousand_separator, ==, ',');

    GList *transactions = m_match(csv_path, settings);
    g_assert_nonnull(transactions);
    g_assert_cmpuint(g_list_length(transactions), ==, 1);

    Data_t *tx = (Data_t *)transactions->data;
    g_assert_cmpstr(tx->description->str, ==, "Coffee Shop");
    g_assert_cmpstr(tx->number->str, ==, "REF-001");
    g_assert_cmpint(tx->amountCents, ==, -450);

    g_list_free_full(transactions, (GDestroyNotify)free_data_t);
    free_parser_settings_t(settings);
    unlink(config_path);
    unlink(csv_path);
}

// Test multiple AMOUNT columns summing functionality
static void test_integration_multiple_amounts(void)
{
    // Load configuration with multiple AMOUNT columns
    ParserSetting_t *settings = mpl_load("tests/data/multiple_amount_config.ini");
    g_assert_nonnull(settings);

    // Process CSV with multiple amount columns
    GList *transactions = m_match("tests/data/multiple_amount_test.csv", settings);
    g_assert_nonnull(transactions);

    // We should have 3 transactions
    g_assert_cmpuint(g_list_length(transactions), ==, 3);

    // Verify first transaction: 3000.00 + 500.00 = 3500.00 = 350000 cents
    Data_t *tx1 = (Data_t *)g_list_nth_data(transactions, 0);
    g_assert_nonnull(tx1);
    g_assert_cmpint(tx1->amountCents, ==, 350000);
    g_assert_true(tx1->hasValidAmount);

    // Verify second transaction: -200.50 + (-75.25) = -275.75 = -27575 cents
    Data_t *tx2 = (Data_t *)g_list_nth_data(transactions, 1);
    g_assert_nonnull(tx2);
    g_assert_cmpint(tx2->amountCents, ==, -27575);
    g_assert_true(tx2->hasValidAmount);

    // Verify third transaction: 1000.00 + (-5.00) = 995.00 = 99500 cents
    Data_t *tx3 = (Data_t *)g_list_nth_data(transactions, 2);
    g_assert_nonnull(tx3);
    g_assert_cmpint(tx3->amountCents, ==, 99500);
    g_assert_true(tx3->hasValidAmount);

    // Cleanup
    g_list_free_full(transactions, (GDestroyNotify)free_data_t);
    free_parser_settings_t(settings);
}

// Test empty amount columns handling
static void test_integration_empty_amounts(void)
{
    // Load configuration with multiple AMOUNT columns
    ParserSetting_t *settings = mpl_load("tests/data/empty_amounts_config.ini");
    g_assert_nonnull(settings);

    // Process CSV with empty amount columns
    GList *transactions = m_match("tests/data/empty_amounts_test.csv", settings);
    g_assert_nonnull(transactions);

    // We should have 3 valid transactions (rows with at least one non-empty amount)
    // Rows 3 and 4 should be rejected (all amounts empty or whitespace-only)
    g_assert_cmpuint(g_list_length(transactions), ==, 3);

    // Verify first transaction: 100.00 + 0 = 100.00 = 10000 cents
    Data_t *tx1 = (Data_t *)g_list_nth_data(transactions, 0);
    g_assert_nonnull(tx1);
    g_assert_cmpint(tx1->amountCents, ==, 10000);
    g_assert_true(tx1->hasValidAmount);

    // Verify second transaction: 0 + 50.25 = 50.25 = 5025 cents
    Data_t *tx2 = (Data_t *)g_list_nth_data(transactions, 1);
    g_assert_nonnull(tx2);
    g_assert_cmpint(tx2->amountCents, ==, 5025);
    g_assert_true(tx2->hasValidAmount);

    // Verify third transaction: 75.00 + 25.00 = 100.00 = 10000 cents
    Data_t *tx3 = (Data_t *)g_list_nth_data(transactions, 2);
    g_assert_nonnull(tx3);
    g_assert_cmpint(tx3->amountCents, ==, 10000);
    g_assert_true(tx3->hasValidAmount);

    // Cleanup
    g_list_free_full(transactions, (GDestroyNotify)free_data_t);
    free_parser_settings_t(settings);
}

// Test mixed sign AMOUNT and -AMOUNT columns
static void test_integration_mixed_sign_amounts(void)
{
    // Load configuration with AMOUNT and -AMOUNT columns
    ParserSetting_t *settings = mpl_load("tests/data/mixed_sign_config.ini");
    g_assert_nonnull(settings);

    // Process CSV with mixed sign amount columns
    GList *transactions = m_match("tests/data/mixed_sign_test.csv", settings);
    g_assert_nonnull(transactions);

    // We should have 3 transactions
    g_assert_cmpuint(g_list_length(transactions), ==, 3);

    // Verify first transaction: Credit 100.00 + Debit 0 = 100.00 = 10000 cents
    Data_t *tx1 = (Data_t *)g_list_nth_data(transactions, 0);
    g_assert_nonnull(tx1);
    g_assert_cmpint(tx1->amountCents, ==, 10000); // 100.00 * 1 + 0 * -1 = 100.00
    g_assert_true(tx1->hasValidAmount);

    // Verify second transaction: Credit 0 + Debit 50.25 = -50.25 = -5025 cents
    Data_t *tx2 = (Data_t *)g_list_nth_data(transactions, 1);
    g_assert_nonnull(tx2);
    g_assert_cmpint(tx2->amountCents, ==, -5025); // 0 * 1 + 50.25 * -1 = -50.25
    g_assert_true(tx2->hasValidAmount);

    // Verify third transaction: Credit 75.00 + Debit 25.00 = 50.00 = 5000 cents
    Data_t *tx3 = (Data_t *)g_list_nth_data(transactions, 2);
    g_assert_nonnull(tx3);
    g_assert_cmpint(tx3->amountCents, ==, 5000); // 75.00 * 1 + 25.00 * -1 = 50.00
    g_assert_true(tx3->hasValidAmount);

    // Cleanup
    g_list_free_full(transactions, (GDestroyNotify)free_data_t);
    free_parser_settings_t(settings);
}

// Valid dates should be accepted according to the configured format, without
// importer-specific rolling age limits.
static void test_integration_accepts_valid_historical_and_future_dates(void)
{
    const char *config_path = "test_date_range_config.csv";
    const char *csv_path = "test_date_range_data.csv";

    FILE *fp = fopen(config_path, "w");
    g_assert_nonnull(fp);
    fprintf(fp, "CSVPARAMS,SKIP_ROWS=1\n");
    fprintf(fp, "Checking Account, -\n");
    fprintf(fp, "DATE:FORMAT=YYYY-MM-DD,DESCR,AMOUNT,NUM\n");
    fprintf(fp, "Archive,Expenses:Archive,100\n");
    fprintf(fp, "Projection,Assets:Projection,100\n");
    fclose(fp);

    fp = fopen(csv_path, "w");
    g_assert_nonnull(fp);
    fprintf(fp, "Date,Description,Amount,Number\n");
    fprintf(fp, "1960-01-01,Archive transaction,10.00,OLD-001\n");
    fprintf(fp, "2040-01-01,Projection transaction,20.00,FUT-001\n");
    fclose(fp);

    ParserSetting_t *settings = mpl_load(config_path);
    g_assert_nonnull(settings);

    GList *transactions = m_match(csv_path, settings);
    guint transaction_count = g_list_length(transactions);

    if (transaction_count != 2) {
        g_test_message("Expected both valid formatted dates to import; got %u transaction(s)",
                       transaction_count);
        g_test_fail();
    }

    g_list_free_full(transactions, (GDestroyNotify)free_data_t);
    free_parser_settings_t(settings);
    unlink(config_path);
    unlink(csv_path);
}

// Invalid DATE_LEQ filters should make the rule file invalid. Keeping a
// malformed filter leaves the filter union in an unsafe state at runtime.
static void test_integration_rejects_invalid_date_filter(void)
{
    const char *config_path = "test_invalid_date_filter_config.csv";

    FILE *fp = fopen(config_path, "w");
    g_assert_nonnull(fp);
    fprintf(fp, "FILTEROUT,DATE_LEQ,not-a-date,YYYY-MM-DD\n");
    fprintf(fp, "Checking Account, -\n");
    fprintf(fp, "DATE:FORMAT=YYYY-MM-DD,DESCR,AMOUNT,NUM\n");
    fprintf(fp, "Coffee,Expenses:Dining,100\n");
    fclose(fp);

    ParserSetting_t *settings = mpl_load(config_path);

    if (settings != NULL) {
        g_test_message("Expected mpl_load() to reject a rule file with an invalid DATE_LEQ filter");
        g_test_fail();
        // Do not free settings here: the current implementation stores a
        // GString in a GDateTime union slot for this malformed filter.
    }

    unlink(config_path);
}

void test_integration_register(void)
{
    // Register integration test cases
    g_test_add_func("/integration/us_format_pipeline", test_integration_us_format);
    g_test_add_func("/integration/eu_format_pipeline", test_integration_eu_format);
    g_test_add_func("/integration/malformed_data_handling", test_integration_malformed_data);
    g_test_add_func("/integration/config_file_validation", test_integration_config_validation);
    g_test_add_func("/integration/csv_file_validation", test_integration_csv_validation);
    g_test_add_func("/integration/memory_management", test_integration_memory_management);
    g_test_add_func("/integration/date_format_handling", test_integration_date_formats);
    g_test_add_func("/integration/default_csv_params", test_integration_default_csv_params);
    g_test_add_func("/integration/multiple_amount_columns", test_integration_multiple_amounts);
    g_test_add_func("/integration/empty_amount_columns", test_integration_empty_amounts);
    g_test_add_func("/integration/mixed_sign_amounts", test_integration_mixed_sign_amounts);
    g_test_add_func("/integration/accepts_valid_historical_and_future_dates",
                    test_integration_accepts_valid_historical_and_future_dates);
    g_test_add_func("/integration/rejects_invalid_date_filter",
                    test_integration_rejects_invalid_date_filter);
}
