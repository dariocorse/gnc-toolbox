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
#include <sys/stat.h>
#include "test_input_validation.h"
#include "../inc/utilities.h"

static void test_input_validation_cleanup(void)
{
    chmod("test_readonly.txt", 0644);
    unlink("test_file.txt");
    unlink("test_readonly.txt");
    unlink("new_file.txt");
    unlink("test.txt");
    unlink("test.gnucash");
    unlink("test.xac");
    unlink("test.xml");
    unlink("lock_test.gnucash.LCK");
    unlink("lock_test.gnucash");
    rmdir("test_dir");
}

// Test file existence validation
static void test_validate_file_exists_and_readable(void)
{
    GError *error = NULL;

    // Test with NULL file path
    g_assert_false(validate_file_exists_and_readable(NULL, &error));
    g_assert_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL);
    g_clear_error(&error);

    // Test with non-existent file
    g_assert_false(validate_file_exists_and_readable("nonexistent_file.txt", &error));
    g_assert_error(error, G_FILE_ERROR, G_FILE_ERROR_NOENT);
    g_clear_error(&error);

    // Create a test file
    FILE *fp = fopen("test_file.txt", "w");
    fprintf(fp, "test content");
    fclose(fp);

    // Test with existing readable file
    g_assert_true(validate_file_exists_and_readable("test_file.txt", &error));
    g_assert_no_error(error);

    // Create read-only file and test
    fp = fopen("test_readonly.txt", "w");
    fprintf(fp, "readonly content");
    fclose(fp);
    chmod("test_readonly.txt", 0000); // Remove all permissions

    g_assert_false(validate_file_exists_and_readable("test_readonly.txt", &error));
    g_assert_error(error, G_FILE_ERROR, G_FILE_ERROR_ACCES);
    g_clear_error(&error);

    // Restore permissions for cleanup
    chmod("test_readonly.txt", 0644);
}

// Test directory validation
static void test_validate_directory_exists(void)
{
    GError *error = NULL;

    // Test with NULL path
    g_assert_false(validate_directory_exists(NULL, &error));
    g_assert_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL);
    g_clear_error(&error);

    // Test with non-existent directory
    g_assert_false(validate_directory_exists("nonexistent_dir", &error));
    g_assert_error(error, G_FILE_ERROR, G_FILE_ERROR_NOENT);
    g_clear_error(&error);

    // Create test directory
    mkdir("test_dir", 0755);

    // Test with existing directory
    g_assert_true(validate_directory_exists("test_dir", &error));
    g_assert_no_error(error);

    // Test with current directory
    g_assert_true(validate_directory_exists(".", &error));
    g_assert_no_error(error);
}

// Test file writability validation
static void test_validate_file_writable(void)
{
    GError *error = NULL;

    // Test with NULL path
    g_assert_false(validate_file_writable(NULL, &error));
    g_assert_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL);
    g_clear_error(&error);

    // Test with non-existent file in existing directory
    g_assert_true(validate_file_writable("new_file.txt", &error));
    g_assert_no_error(error);

    // Create existing writable file
    FILE *fp = fopen("test_file.txt", "w");
    fprintf(fp, "test content");
    fclose(fp);

    g_assert_true(validate_file_writable("test_file.txt", &error));
    g_assert_no_error(error);
}

// Test account name validation
static void test_is_valid_account_name(void)
{
    // Test NULL account name
    g_assert_false(is_valid_account_name(NULL));

    // Test empty account name
    g_assert_false(is_valid_account_name(""));

    // Test valid account names
    g_assert_true(is_valid_account_name("Checking Account"));
    g_assert_true(is_valid_account_name("Savings"));
    g_assert_true(is_valid_account_name("Credit Card"));
    g_assert_true(is_valid_account_name("Assets:Current Assets:Checking"));

    // Test account names with special characters
    g_assert_true(is_valid_account_name("Account-Name"));
    g_assert_true(is_valid_account_name("Account_Name"));
    g_assert_true(is_valid_account_name("Account (Subtype)"));

    // Test very long account name (exceeds 2048 character limit)
    char long_name[2100]; // Exceeds the 2048 limit
    memset(long_name, 'A', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    g_assert_false(is_valid_account_name(long_name));
}

// Test amount string validation
static void test_is_valid_amount_string(void)
{
    // Test NULL amount string
    g_assert_false(is_valid_amount_string(NULL, '.', ','));

    // Test empty amount string
    g_assert_false(is_valid_amount_string("", '.', ','));

    // Test valid amount strings
    g_assert_true(is_valid_amount_string("123.45", '.', ','));
    g_assert_true(is_valid_amount_string("-123,45", ',', '.'));
    g_assert_true(is_valid_amount_string("1234", '.', ','));
    g_assert_true(is_valid_amount_string("0.99", '.', ','));
    g_assert_true(is_valid_amount_string("123,45", ',', '.'));    // Comma as decimal separator
    g_assert_true(is_valid_amount_string("+1234.56", '.', ','));  // No thousand separator
    g_assert_true(is_valid_amount_string("+1,234.56", '.', ',')); // Thousand separator and + sign

    // Test invalid amount strings
    g_assert_false(is_valid_amount_string("abc", '.', ','));
    g_assert_false(is_valid_amount_string("12.34.56", '.', ','));
    g_assert_false(is_valid_amount_string("12,,34", ',', '.'));
    g_assert_false(
        is_valid_amount_string("1,234.56", ',', '.')); // thousand separator after decimal separator
    g_assert_false(is_valid_amount_string("$123.45", '.', ',')); // Currency symbols not allowed
    g_assert_false(is_valid_amount_string("123.45€", '.', ','));
}

// Test GnuCash book reference validation
static void test_validate_gnucash_book_ref(void)
{
    GError *error = NULL;

    // Test with NULL book reference
    g_assert_false(validate_gnucash_book_ref(NULL, &error));
    g_assert_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL);
    g_clear_error(&error);

    // Test with invalid extension
    FILE *fp = fopen("test.txt", "w");
    fprintf(fp, "not a gnucash file");
    fclose(fp);

    g_assert_false(validate_gnucash_book_ref("test.txt", &error));
    g_assert_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL);
    g_clear_error(&error);

    // Test with valid file-backed extensions
    fp = fopen("test.gnucash", "w");
    fprintf(fp, "gnucash test file");
    fclose(fp);

    g_assert_true(validate_gnucash_book_ref("test.gnucash", &error));
    g_assert_no_error(error);

    fp = fopen("test.xac", "w");
    fprintf(fp, "xac test file");
    fclose(fp);

    g_assert_true(validate_gnucash_book_ref("test.xac", &error));
    g_assert_no_error(error);

    fp = fopen("test.xml", "w");
    fprintf(fp, "xml test file");
    fclose(fp);

    g_assert_true(validate_gnucash_book_ref("test.xml", &error));
    g_assert_no_error(error);

    // Test valid file:// URI
    gchar *cwd = g_get_current_dir();
    gchar *absolute_book_path = g_build_filename(cwd, "test.gnucash", NULL);
    gchar *book_uri = g_filename_to_uri(absolute_book_path, NULL, &error);
    g_assert_no_error(error);
    g_assert_nonnull(book_uri);

    g_assert_true(validate_gnucash_book_ref(book_uri, &error));
    g_assert_no_error(error);

    // Test backend URI support without local filesystem checks
    g_assert_true(validate_gnucash_book_ref("mysql://user:pass@localhost/gnucash", &error));
    g_assert_no_error(error);

    g_free(book_uri);
    g_free(absolute_book_path);
    g_free(cwd);

    unlink("test.txt");
    unlink("test.gnucash");
    unlink("test.xac");
    unlink("test.xml");
}

static void test_gnc_is_locked_book_refs(void)
{
    FILE *fp = fopen("lock_test.gnucash", "w");
    g_assert_nonnull(fp);
    fprintf(fp, "gnucash test file");
    fclose(fp);

    fp = fopen("lock_test.gnucash.LCK", "w");
    g_assert_nonnull(fp);
    fprintf(fp, "lock");
    fclose(fp);

    g_assert_true(gnc_is_locked("lock_test.gnucash"));

    gchar *cwd = g_get_current_dir();
    gchar *absolute_book_path = g_build_filename(cwd, "lock_test.gnucash", NULL);
    gchar *book_uri = g_filename_to_uri(absolute_book_path, NULL, NULL);
    g_assert_nonnull(book_uri);

    g_assert_true(gnc_is_locked(book_uri));
    g_assert_false(gnc_is_locked("mysql://user:pass@localhost/gnucash"));

    g_free(book_uri);
    g_free(absolute_book_path);
    g_free(cwd);

    unlink("lock_test.gnucash.LCK");
    unlink("lock_test.gnucash");
}

void test_input_validation_register(void)
{
    g_test_add_func("/input_validation/cleanup_before", test_input_validation_cleanup);

    // Register test cases
    g_test_add_func("/input_validation/file_exists_readable",
                    test_validate_file_exists_and_readable);
    g_test_add_func("/input_validation/directory_exists", test_validate_directory_exists);
    g_test_add_func("/input_validation/file_writable", test_validate_file_writable);
    g_test_add_func("/input_validation/account_name", test_is_valid_account_name);
    g_test_add_func("/input_validation/amount_string", test_is_valid_amount_string);
    g_test_add_func("/input_validation/gnucash_book_ref", test_validate_gnucash_book_ref);
    g_test_add_func("/input_validation/gnucash_lock_detection", test_gnc_is_locked_book_refs);

    g_test_add_func("/input_validation/cleanup_after", test_input_validation_cleanup);
}
