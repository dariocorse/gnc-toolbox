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
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "test_utilities.h"
#include "../inc/output.h"
#include "../inc/utilities.h"

static gchar *capture_print_data_output(OutLevel_e level, const Data_t *data)
{
    gchar template[] = "/tmp/gnc-toolbox-print-data-XXXXXX";
    int capture_fd = g_mkstemp(template);
    g_assert_cmpint(capture_fd, >=, 0);

    int saved_stdout = dup(STDOUT_FILENO);
    g_assert_cmpint(saved_stdout, >=, 0);

    out_setLevel(OUT_LEVEL_DEBUG);
    fflush(stdout);
    dup2(capture_fd, STDOUT_FILENO);

    printData(level, data);

    fflush(stdout);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
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

// Test GString utility functions
static void test_gstring_utilities(void)
{
    GString *haystack = g_string_new("The quick brown fox jumps over the lazy dog");
    GString *needle1 = g_string_new("brown fox");
    GString *needle2 = g_string_new("purple cat");

    // Test gstring_contains
    g_assert_true(gstring_contains(haystack, needle1));
    g_assert_false(gstring_contains(haystack, needle2));

    // Test with NULL inputs
    g_assert_false(gstring_contains(NULL, needle1));
    g_assert_false(gstring_contains(haystack, NULL));

    // Test empty strings
    GString *empty = g_string_new("");
    g_assert_true(gstring_contains(haystack, empty)); // Any string contains empty string
    g_assert_true(gstring_contains(empty, empty));    // Empty contains empty

    g_string_free(haystack, TRUE);
    g_string_free(needle1, TRUE);
    g_string_free(needle2, TRUE);
    g_string_free(empty, TRUE);
}

// Test pattern matching functions
static void test_pattern_matching(void)
{
    GString *test_str = g_string_new("test@example.com");

    // Shell-style pattern matching.
    g_assert_true(gstring_matches_pattern(test_str, "*@*.com"));
    g_assert_false(gstring_matches_pattern(test_str, "*@*.org"));
    g_assert_false(gstring_matches_pattern(test_str, "^test@.*\\.com$"));

    // Test with NULL inputs
    g_assert_false(gstring_matches_pattern(NULL, "*@*.com"));
    g_assert_false(gstring_matches_pattern(test_str, NULL));

    g_string_free(test_str, TRUE);
}

static void test_regex_matching(void)
{
    GString *test_str = g_string_new("test@example.com");

    g_assert_true(gstring_matches_regex(test_str, "^test@.*\\.com$"));
    g_assert_false(gstring_matches_regex(test_str, "^admin@.*\\.com$"));
    g_assert_false(gstring_matches_regex(test_str, "*@*.com"));

    g_assert_false(gstring_matches_regex(NULL, "^test"));
    g_assert_false(gstring_matches_regex(test_str, NULL));

    g_string_free(test_str, TRUE);
}

// Test startsWith function
static void test_starts_with(void)
{
    const char *test_string = "Hello World";

    // Test valid prefixes
    g_assert_true(startsWith(test_string, strlen(test_string), "Hello"));
    g_assert_true(startsWith(test_string, strlen(test_string), "Hell"));
    g_assert_true(startsWith(test_string, strlen(test_string), "H"));
    g_assert_true(startsWith(test_string, strlen(test_string), "")); // Empty prefix

    // Test invalid prefixes
    g_assert_false(startsWith(test_string, strlen(test_string), "World"));
    g_assert_false(startsWith(test_string, strlen(test_string), "hello")); // Case sensitive
    g_assert_false(
        startsWith(test_string, strlen(test_string), "Hello World!")); // Longer than string

    // Test with NULL inputs
    g_assert_false(startsWith(NULL, 0, "Hello"));
    g_assert_false(startsWith(test_string, strlen(test_string), NULL));

    // Test with zero length
    g_assert_false(startsWith(test_string, 0, "Hello"));
}

// Test memory management utilities
static void test_memory_management(void)
{
    // Test safe_g_free
    gpointer test_ptr = g_malloc(100);
    g_assert_nonnull(test_ptr);
    safe_g_free(&test_ptr);
    g_assert_null(test_ptr);

    // Test safe_g_free with NULL pointer
    gpointer null_ptr = NULL;
    safe_g_free(&null_ptr); // Should not crash
    g_assert_null(null_ptr);

    // Test safe_g_string_free
    GString *test_string = g_string_new("Test string");
    g_assert_nonnull(test_string);
    safe_g_string_free(&test_string);
    g_assert_null(test_string);

    // Test safe_g_string_free with NULL
    GString *null_string = NULL;
    safe_g_string_free(&null_string); // Should not crash
    g_assert_null(null_string);

    // Test safe_g_date_time_unref
    GDateTime *test_datetime = g_date_time_new_now_utc();
    g_assert_nonnull(test_datetime);
    safe_g_date_time_unref(&test_datetime);
    g_assert_null(test_datetime);

    // Test safe_g_date_time_unref with NULL
    GDateTime *null_datetime = NULL;
    safe_g_date_time_unref(&null_datetime); // Should not crash
    g_assert_null(null_datetime);
}

// Test data conversion functions
static void test_data_conversion(void)
{
    const char *test_data = "Hello World";
    size_t test_len = strlen(test_data);

    // Test getCStringFromData
    char *c_string = getCStringFromData((void *)test_data, test_len);
    g_assert_nonnull(c_string);
    g_assert_cmpstr(c_string, ==, "Hello World");
    g_free(c_string);

    // Test getGStringFromData
    GString *g_string = getGStringFromData((void *)test_data, test_len);
    g_assert_nonnull(g_string);
    g_assert_cmpstr(g_string->str, ==, "Hello World");
    g_assert_cmpuint(g_string->len, ==, test_len);
    g_string_free(g_string, TRUE);

    // Test with empty data
    c_string = getCStringFromData((void *)"", 0);
    g_assert_nonnull(c_string);
    g_assert_cmpstr(c_string, ==, "");
    g_free(c_string);

    g_string = getGStringFromData((void *)"", 0);
    g_assert_nonnull(g_string);
    g_assert_cmpuint(g_string->len, ==, 0);
    g_string_free(g_string, TRUE);

    // Test with NULL data
    c_string = getCStringFromData(NULL, 0);
    g_assert_nonnull(c_string);
    g_assert_cmpstr(c_string, ==, "");
    g_free(c_string);

    g_string = getGStringFromData(NULL, 0);
    g_assert_nonnull(g_string);
    g_assert_cmpuint(g_string->len, ==, 0);
    g_string_free(g_string, TRUE);
}

// Test printData function (basic coverage)
static void test_print_data(void)
{
    // Create test data structure
    Data_t test_data = {0};
    test_data.description = g_string_new("Test transaction");
    test_data.number = g_string_new("12345");
    test_data.amountCents = 10050; // 100.50
    test_data.date = g_date_time_new_utc(2023, 12, 25, 0, 0, 0);

    gchar *formatted_output = capture_print_data_output(OUT_LEVEL_INFO, &test_data);
    g_assert_nonnull(strstr(formatted_output, "2023-12-25 00:00:00"));
    g_assert_nonnull(strstr(formatted_output, "12345"));
    g_assert_nonnull(strstr(formatted_output, "Test transaction"));
    g_assert_nonnull(strstr(formatted_output, "100.50"));
    g_free(formatted_output);

    gchar *null_output = capture_print_data_output(OUT_LEVEL_DEBUG, NULL);
    g_assert_nonnull(strstr(null_output, "(null data)"));
    g_free(null_output);

    Data_t partial_data = {0};
    gchar *partial_output = capture_print_data_output(OUT_LEVEL_INFO, &partial_data);
    g_assert_nonnull(strstr(partial_output, "(no-date)"));
    g_assert_nonnull(strstr(partial_output, "(no description)"));
    g_free(partial_output);

    // Cleanup
    if (test_data.description)
        g_string_free(test_data.description, TRUE);
    if (test_data.number)
        g_string_free(test_data.number, TRUE);
    if (test_data.date)
        g_date_time_unref(test_data.date);
}

void test_utilities_register(void)
{
    // Register test cases
    g_test_add_func("/utilities/gstring_functions", test_gstring_utilities);
    g_test_add_func("/utilities/pattern_matching", test_pattern_matching);
    g_test_add_func("/utilities/regex_matching", test_regex_matching);
    g_test_add_func("/utilities/starts_with", test_starts_with);
    g_test_add_func("/utilities/memory_management", test_memory_management);
    g_test_add_func("/utilities/data_conversion", test_data_conversion);
    g_test_add_func("/utilities/print_data", test_print_data);
}
