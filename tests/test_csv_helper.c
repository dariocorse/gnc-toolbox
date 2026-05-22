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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>

#include "test_csv_helper.h"
#include "../inc/csv_helper.h"
#include "../inc/output.h"

static const char *CSV_HELPER_TEST_FILE = "test_csv_helper.csv";

static gchar *capture_stdout(void (*fn)(void *), void *user_data)
{
    gchar template[] = "/tmp/gnc-toolbox-csv-helper-XXXXXX";
    int capture_fd = g_mkstemp(template);
    g_assert_cmpint(capture_fd, >=, 0);

    int saved_stdout = dup(STDOUT_FILENO);
    g_assert_cmpint(saved_stdout, >=, 0);

    fflush(stdout);
    dup2(capture_fd, STDOUT_FILENO);

    fn(user_data);

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

static void print_standalone_message(void *user_data)
{
    (void)user_data;
    csv_printMessage(OUT_LEVEL_INFO, "standalone message");
    csv_printMessagef(OUT_LEVEL_INFO, "formatted %d", 42);
}

static int field_message_count = 0;
static int row_message_count = 0;

static void csv_helper_test_field(int rowIndex, int fieldIndex, void *s, size_t len)
{
    (void)s;
    (void)len;

    if (rowIndex == 0 && fieldIndex == 0 && field_message_count == 0) {
        field_message_count++;
        csv_printMessage(OUT_LEVEL_INFO, "plain field message");
        csv_printMessagef(OUT_LEVEL_INFO, "formatted field %d", 7);
    }
}

static void csv_helper_test_row(int rowIndex, char terminationChar)
{
    (void)terminationChar;

    if (rowIndex == 0 && row_message_count == 0) {
        row_message_count++;
        csv_printMessage(OUT_LEVEL_INFO, "row completed");
    }
}

static void load_csv_and_emit_messages(void *user_data)
{
    (void)user_data;

    CSVParseParam_t csv_params = {.delimiter = ',',
                                  .quote = '"',
                                  .decimal_separator = '.',
                                  .thousand_separator = ',',
                                  .skip_header_rows = 0};

    field_message_count = 0;
    row_message_count = 0;
    csv_load(CSV_HELPER_TEST_FILE, &csv_params, csv_helper_test_field, csv_helper_test_row);
}

static void test_csv_helper_print_message_without_context(void)
{
    out_setLevel(OUT_LEVEL_DEBUG);
    gchar *output = capture_stdout(print_standalone_message, NULL);
    g_assert_nonnull(strstr(output, "CSV: standalone message"));
    g_assert_nonnull(strstr(output, "CSV: formatted 42"));
    g_free(output);
    out_setLevel(OUT_LEVEL_WARNING);
}

static void test_csv_helper_print_message_during_parse(void)
{
    FILE *fp = fopen(CSV_HELPER_TEST_FILE, "w");
    g_assert_nonnull(fp);
    fprintf(fp, "alpha,beta\n");
    fclose(fp);

    out_setLevel(OUT_LEVEL_DEBUG);
    gchar *output = capture_stdout(load_csv_and_emit_messages, NULL);
    g_assert_nonnull(strstr(
        output, "CSV 'test_csv_helper.csv' row 1, field 1, value 'alpha': plain field message"));
    g_assert_nonnull(strstr(
        output, "CSV 'test_csv_helper.csv' row 1, field 1, value 'alpha': formatted field 7"));
    g_assert_nonnull(strstr(output, "CSV 'test_csv_helper.csv' row 1: row completed"));
    g_free(output);

    unlink(CSV_HELPER_TEST_FILE);
    out_setLevel(OUT_LEVEL_WARNING);
}

void test_csv_helper_register(void)
{
    g_test_add_func("/csv_helper/print_message_without_context",
                    test_csv_helper_print_message_without_context);
    g_test_add_func("/csv_helper/print_message_during_parse",
                    test_csv_helper_print_message_during_parse);
}
