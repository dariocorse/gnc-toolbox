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
#include <math.h>
#include "test_number_parsing.h"
#include "../inc/utilities.h"

// Helper function to compare doubles with tolerance
static gboolean double_equals(double a, double b, double tolerance)
{
    return fabs(a - b) < tolerance;
}

// Test US/UK format parsing (dot decimal, comma thousands)
static void test_number_parsing_us_format(void)
{
    double result;

    // Test basic numbers
    result = parse_amount_with_locale("123.45", '.', ',');
    g_assert_true(double_equals(result, 123.45, 0.001));

    result = parse_amount_with_locale("1,234.56", '.', ',');
    g_assert_true(double_equals(result, 1234.56, 0.001));

    result = parse_amount_with_locale("12,345.67", '.', ',');
    g_assert_true(double_equals(result, 12345.67, 0.001));

    // Test large numbers
    result = parse_amount_with_locale("1,234,567.89", '.', ',');
    g_assert_true(double_equals(result, 1234567.89, 0.001));

    // Test negative numbers
    result = parse_amount_with_locale("-123.45", '.', ',');
    g_assert_true(double_equals(result, -123.45, 0.001));

    result = parse_amount_with_locale("-1,234.56", '.', ',');
    g_assert_true(double_equals(result, -1234.56, 0.001));
}

// Test European format parsing (comma decimal, dot thousands)
static void test_number_parsing_european_format(void)
{
    double result;

    // Test basic numbers
    result = parse_amount_with_locale("123,45", ',', '.');
    g_assert_true(double_equals(result, 123.45, 0.001));

    result = parse_amount_with_locale("1.234,56", ',', '.');
    g_assert_true(double_equals(result, 1234.56, 0.001));

    result = parse_amount_with_locale("12.345,67", ',', '.');
    g_assert_true(double_equals(result, 12345.67, 0.001));

    // Test large numbers
    result = parse_amount_with_locale("+1.234.567,89", ',', '.');
    g_assert_true(double_equals(result, 1234567.89, 0.001));

    // Test negative numbers
    result = parse_amount_with_locale("-123,45", ',', '.');
    g_assert_true(double_equals(result, -123.45, 0.001));

    result = parse_amount_with_locale("-1.234,56", ',', '.');
    g_assert_true(double_equals(result, -1234.56, 0.001));
}

// Test space as thousands separator
static void test_number_parsing_space_separator(void)
{
    double result;

    // Test with space thousands separator
    result = parse_amount_with_locale("1 234,56", ',', ' ');
    g_assert_true(double_equals(result, 1234.56, 0.001));

    result = parse_amount_with_locale("12 345,67", ',', ' ');
    g_assert_true(double_equals(result, 12345.67, 0.001));

    result = parse_amount_with_locale("1 234 567,89", ',', ' ');
    g_assert_true(double_equals(result, 1234567.89, 0.001));

    // Test negative numbers
    result = parse_amount_with_locale("-1 234,56", ',', ' ');
    g_assert_true(double_equals(result, -1234.56, 0.001));
}

// Test numbers without thousands separator
static void test_number_parsing_no_thousands(void)
{
    double result;

    // Test simple numbers (US format)
    result = parse_amount_with_locale("123.45", '.', ',');
    g_assert_true(double_equals(result, 123.45, 0.001));

    result = parse_amount_with_locale("9876.54", '.', ',');
    g_assert_true(double_equals(result, 9876.54, 0.001));

    // Test simple numbers (European format)
    result = parse_amount_with_locale("123,45", ',', '.');
    g_assert_true(double_equals(result, 123.45, 0.001));

    result = parse_amount_with_locale("9876,54", ',', '.');
    g_assert_true(double_equals(result, 9876.54, 0.001));

    // Test integers
    result = parse_amount_with_locale("1234", '.', ',');
    g_assert_true(double_equals(result, 1234.0, 0.001));

    result = parse_amount_with_locale("5678", ',', '.');
    g_assert_true(double_equals(result, 5678.0, 0.001));
}

// Test edge cases and special values
static void test_number_parsing_edge_cases(void)
{
    double result;

    // Test NULL input
    result = parse_amount_with_locale(NULL, '.', ',');
    g_assert_true(result == 0.0);

    // Test empty string
    result = parse_amount_with_locale("", '.', ',');
    g_assert_true(result == 0.0);

    // Test whitespace
    result = parse_amount_with_locale("   123.45   ", '.', ',');
    g_assert_true(double_equals(result, 123.45, 0.001));

    // Test zero values
    result = parse_amount_with_locale("0", '.', ',');
    g_assert_true(result == 0.0);

    result = parse_amount_with_locale("0.00", '.', ',');
    g_assert_true(result == 0.0);

    result = parse_amount_with_locale("0,00", ',', '.');
    g_assert_true(result == 0.0);

    // Test very small numbers
    result = parse_amount_with_locale("0.01", '.', ',');
    g_assert_true(double_equals(result, 0.01, 0.001));

    result = parse_amount_with_locale("0,01", ',', '.');
    g_assert_true(double_equals(result, 0.01, 0.001));
}

// Test invalid inputs
static void test_number_parsing_invalid_input(void)
{
    double result;

    // Test non-numeric strings
    result = parse_amount_with_locale("abc", '.', ',');
    g_assert_true(result == 0.0 || isnan(result));

    result = parse_amount_with_locale("123abc", '.', ',');
    g_assert_true(double_equals(result, 123.0, 0.001)); // Should parse up to first invalid char

    // Test multiple decimal separators
    result = parse_amount_with_locale("123.45.67", '.', ',');
    g_assert_true(double_equals(result, 123.45, 0.001)); // Should stop at first invalid pattern

    // Test mixed separators incorrectly
    result = parse_amount_with_locale("1,234.56", ',', '.'); // Using wrong separator combination
    // Result depends on implementation - might parse as 1 or handle gracefully

    // Test invalid thousand separator placement
    result = parse_amount_with_locale("12,34.56", '.', ','); // Comma in wrong position
    // Should handle gracefully
}

// Test normalize_amount_string function
static void test_normalize_amount_string(void)
{
    char *result;

    // Test US format normalization
    result = normalize_amount_string("1,234.56", '.', ',');
    g_assert_nonnull(result);
    g_assert_cmpstr(result, ==, "1234.56");
    g_free(result);

    // Test European format normalization
    result = normalize_amount_string("1.234,56", ',', '.');
    g_assert_nonnull(result);
    g_assert_cmpstr(result, ==, "1234.56"); // Normalized to standard format
    g_free(result);

    // Test space separator normalization
    result = normalize_amount_string("1 234,56", ',', ' ');
    g_assert_nonnull(result);
    g_assert_cmpstr(result, ==, "1234.56"); // Normalized to standard format
    g_free(result);

    // Test NULL input
    result = normalize_amount_string(NULL, '.', ',');
    g_assert_null(result);

    // Test empty string
    result = normalize_amount_string("", '.', ',');
    g_assert_nonnull(result);
    g_assert_cmpstr(result, ==, "");
    g_free(result);
}

// Test precision and rounding
static void test_number_parsing_precision(void)
{
    double result;

    // Test high precision numbers
    result = parse_amount_with_locale("123.456789", '.', ',');
    g_assert_true(double_equals(result, 123.456789, 0.000001));

    result = parse_amount_with_locale("0.000001", '.', ',');
    g_assert_true(double_equals(result, 0.000001, 0.000001));

    // Test very large numbers
    result = parse_amount_with_locale("1,000,000.00", '.', ',');
    g_assert_true(double_equals(result, 1000000.0, 0.001));

    // Test numbers with many decimal places
    result = parse_amount_with_locale("123.123456789012345", '.', ',');
    g_assert_true(double_equals(result, 123.123456789012345, 0.000000000000001));
}

static void test_parse_amount_to_cents_exact_decimal_values(void)
{
    gint cents = 0;

    g_assert_true(parse_amount_to_cents("0.29", '.', ',', &cents));
    g_assert_cmpint(cents, ==, 29);

    g_assert_true(parse_amount_to_cents("1.13", '.', ',', &cents));
    g_assert_cmpint(cents, ==, 113);

    g_assert_true(parse_amount_to_cents("-0.29", '.', ',', &cents));
    g_assert_cmpint(cents, ==, -29);

    g_assert_true(parse_amount_to_cents("1.234,56", ',', '.', &cents));
    g_assert_cmpint(cents, ==, 123456);

    g_assert_true(parse_amount_to_cents("-1.234,56", ',', '.', &cents));
    g_assert_cmpint(cents, ==, -123456);

    g_assert_true(parse_amount_to_cents("10.125", '.', ',', &cents));
    g_assert_cmpint(cents, ==, 1013);

    g_assert_false(parse_amount_to_cents("21474836.48", '.', ',', &cents));
    g_assert_false(parse_amount_to_cents("12.34.56", '.', ',', &cents));
}

void test_number_parsing_register(void)
{
    // Register test cases
    g_test_add_func("/number_parsing/us_format", test_number_parsing_us_format);
    g_test_add_func("/number_parsing/european_format", test_number_parsing_european_format);
    g_test_add_func("/number_parsing/space_separator", test_number_parsing_space_separator);
    g_test_add_func("/number_parsing/no_thousands", test_number_parsing_no_thousands);
    g_test_add_func("/number_parsing/edge_cases", test_number_parsing_edge_cases);
    g_test_add_func("/number_parsing/invalid_input", test_number_parsing_invalid_input);
    g_test_add_func("/number_parsing/normalize_string", test_normalize_amount_string);
    g_test_add_func("/number_parsing/precision", test_number_parsing_precision);
    g_test_add_func("/number_parsing/parse_amount_to_cents_exact_decimal_values",
                    test_parse_amount_to_cents_exact_decimal_values);
}
