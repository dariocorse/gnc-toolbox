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
#include "test_date_parsing.h"
#include "../inc/utilities.h"

// Helper function to compare GDateTime objects
static gboolean datetime_equals(GDateTime *dt1, int year, int month, int day)
{
    if (!dt1)
        return FALSE;

    return (g_date_time_get_year(dt1) == year && g_date_time_get_month(dt1) == month &&
            g_date_time_get_day_of_month(dt1) == day);
}

static gboolean datetime_equals_either(GDateTime *dt, int y1, int y2, int m, int d)
{
    if (!dt)
        return FALSE;
    int y = g_date_time_get_year(dt);
    return ((y == y1 || y == y2) && g_date_time_get_month(dt) == m &&
            g_date_time_get_day_of_month(dt) == d);
}

// Test DD/MM/YYYY format parsing
static void test_date_parsing_ddmmyyyy(void)
{
    GDateTime *result;

    // Test valid dates
    result = getDateTimeFromText("15/03/2023", "DD/MM/YYYY");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals(result, 2023, 3, 15));
    g_date_time_unref(result);

    result = getDateTimeFromText("01/12/2024", "DD/MM/YYYY");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals(result, 2024, 12, 1));
    g_date_time_unref(result);

    // Test with different separators
    result = getDateTimeFromText("25-08-2022", "DD-MM-YYYY");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals(result, 2022, 8, 25));
    g_date_time_unref(result);

    result = getDateTimeFromText("10.06.2021", "DD.MM.YYYY");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals(result, 2021, 6, 10));
    g_date_time_unref(result);
}

// Test MM/DD/YYYY format parsing
static void test_date_parsing_mmddyyyy(void)
{
    GDateTime *result;

    // Test valid dates
    result = getDateTimeFromText("03/15/2023", "MM/DD/YYYY");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals(result, 2023, 3, 15));
    g_date_time_unref(result);

    result = getDateTimeFromText("12/01/2024", "MM/DD/YYYY");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals(result, 2024, 12, 1));
    g_date_time_unref(result);

    // Test with different separators
    result = getDateTimeFromText("08-25-2022", "MM-DD-YYYY");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals(result, 2022, 8, 25));
    g_date_time_unref(result);
}

// Test YYYY/MM/DD format parsing
static void test_date_parsing_yyyymmdd(void)
{
    GDateTime *result;

    // Test valid dates
    result = getDateTimeFromText("2023/03/15", "YYYY/MM/DD");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals(result, 2023, 3, 15));
    g_date_time_unref(result);

    result = getDateTimeFromText("2024-12-01", "YYYY-MM-DD");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals(result, 2024, 12, 1));
    g_date_time_unref(result);

    // Test ISO format
    result = getDateTimeFromText("2022.08.25", "YYYY.MM.DD");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals(result, 2022, 8, 25));
    g_date_time_unref(result);
}

static void test_date_parsing_yy_variants(void)
{
    GDateTime *result;

    // DD/MM/YY
    result = getDateTimeFromText("15/03/23", "DD/MM/YY");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals_either(result, 1923, 2023, 3, 15));
    g_date_time_unref(result);

    // MM/DD/YY
    result = getDateTimeFromText("03/15/23", "MM/DD/YY");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals_either(result, 1923, 2023, 3, 15));
    g_date_time_unref(result);

    // YY-MM-DD
    result = getDateTimeFromText("23-03-15", "YY-MM-DD");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals_either(result, 1923, 2023, 3, 15));
    g_date_time_unref(result);

    // YY.MM.DD
    result = getDateTimeFromText("99.12.31", "YY.MM.DD");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals_either(result, 1999, 2099, 12, 31));
    g_date_time_unref(result);

    // Single-digit day/month still OK
    result = getDateTimeFromText("5/3/23", "DD/MM/YY");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals_either(result, 1923, 2023, 3, 5));
    g_date_time_unref(result);

    // Leap year (YY = 20 => 1920 or 2020)
    result = getDateTimeFromText("29/02/20", "DD/MM/YY");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals_either(result, 1920, 2020, 2, 29));
    g_date_time_unref(result);

    // Not a leap year (21 => 1921 or 2021) -> should fail
    result = getDateTimeFromText("29/02/21", "DD/MM/YY");
    g_assert_null(result);

    // Wrong separator vs format -> should fail
    result = getDateTimeFromText("15-03-23", "DD/MM/YY");
    g_assert_null(result);

    // Out-of-range month/day -> should fail
    result = getDateTimeFromText("32/01/23", "DD/MM/YY");
    g_assert_null(result);
    result = getDateTimeFromText("15/13/23", "DD/MM/YY");
    g_assert_null(result);
}

// Test edge cases and error conditions
static void test_date_parsing_edge_cases(void)
{
    GDateTime *result;

    // Test NULL inputs
    result = getDateTimeFromText(NULL, "DD/MM/YYYY");
    g_assert_null(result);

    result = getDateTimeFromText("15/03/2023", NULL);
    g_assert_null(result);

    // Test empty string
    result = getDateTimeFromText("", "DD/MM/YYYY");
    g_assert_null(result);

    // Test invalid dates
    result = getDateTimeFromText("32/01/2023", "DD/MM/YYYY"); // Invalid day
    g_assert_null(result);

    result = getDateTimeFromText("15/13/2023", "DD/MM/YYYY"); // Invalid month
    g_assert_null(result);

    result = getDateTimeFromText("29/02/2021", "DD/MM/YYYY"); // Invalid leap year
    g_assert_null(result);

    // Test valid leap year
    result = getDateTimeFromText("29/02/2020", "DD/MM/YYYY");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals(result, 2020, 2, 29));
    g_date_time_unref(result);

    // Test malformed input
    result = getDateTimeFromText("not-a-date", "DD/MM/YYYY");
    g_assert_null(result);

    result = getDateTimeFromText("15/03", "DD/MM/YYYY"); // Missing year
    g_assert_null(result);

    // Test wrong separator
    result = getDateTimeFromText("15-03-2023", "DD/MM/YYYY"); // Dash instead of slash
    g_assert_null(result);
}

// Test boundary dates
static void test_date_parsing_boundaries(void)
{
    GDateTime *result;

    // Test minimum valid dates
    result = getDateTimeFromText("01/01/1900", "DD/MM/YYYY");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals(result, 1900, 1, 1));
    g_date_time_unref(result);

    // Test maximum valid dates
    result = getDateTimeFromText("31/12/2099", "DD/MM/YYYY");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals(result, 2099, 12, 31));
    g_date_time_unref(result);

    // Test month boundaries
    result = getDateTimeFromText("31/01/2023", "DD/MM/YYYY"); // January has 31 days
    g_assert_nonnull(result);
    g_date_time_unref(result);

    result = getDateTimeFromText("30/04/2023", "DD/MM/YYYY"); // April has 30 days
    g_assert_nonnull(result);
    g_date_time_unref(result);

    result = getDateTimeFromText("31/04/2023", "DD/MM/YYYY"); // April doesn't have 31 days
    g_assert_null(result);
}

// Test format variations
static void test_date_parsing_format_variations(void)
{
    GDateTime *result;

    // Test single digit dates
    result = getDateTimeFromText("5/3/2023", "DD/MM/YYYY");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals(result, 2023, 3, 5));
    g_date_time_unref(result);

    // Test mixed formats
    result = getDateTimeFromText("05/3/2023", "DD/MM/YYYY");
    g_assert_nonnull(result);
    g_assert_true(datetime_equals(result, 2023, 3, 5));
    g_date_time_unref(result);

    // Test 2-digit year (should be handled if supported)
    result = getDateTimeFromText("15/03/23", "DD/MM/YY");
    if (result) { // Only check if 2-digit years are supported
        g_assert_true(datetime_equals(result, 2023, 3, 15) || datetime_equals(result, 1923, 3, 15));
        g_date_time_unref(result);
    }
}

void test_date_parsing_register(void)
{
    // Register test cases
    g_test_add_func("/date_parsing/ddmmyyyy", test_date_parsing_ddmmyyyy);
    g_test_add_func("/date_parsing/mmddyyyy", test_date_parsing_mmddyyyy);
    g_test_add_func("/date_parsing/yyyymmdd", test_date_parsing_yyyymmdd);
    g_test_add_func("/date_parsing/yy_variants", test_date_parsing_yy_variants);
    g_test_add_func("/date_parsing/edge_cases", test_date_parsing_edge_cases);
    g_test_add_func("/date_parsing/boundaries", test_date_parsing_boundaries);
    g_test_add_func("/date_parsing/format_variations", test_date_parsing_format_variations);
}
