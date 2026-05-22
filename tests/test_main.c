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
#include <locale.h>

// Test suite headers
#include "test_utilities.h"
#include "test_cli.h"
#include "test_csv_helper.h"
#include "test_date_parsing.h"
#include "test_number_parsing.h"
#include "test_csv_params.h"
#include "test_gnc_utilities.h"
#include "test_input_validation.h"
#include "test_integration.h"
#include "test_book_integration.h"

int main(int argc, char *argv[])
{
    // Initialize GLib test framework
    g_test_init(&argc, &argv, NULL);

    // Set locale for consistent testing
    setlocale(LC_ALL, "C");

    // Register test suites
    test_utilities_register();
    test_cli_register();
    test_csv_helper_register();
    test_date_parsing_register();
    test_number_parsing_register();
    test_csv_params_register();
    test_gnc_utilities_register();
    test_input_validation_register();
    test_integration_register();
    test_book_integration_register();

    // Run all tests
    int result = g_test_run();

    return result;
}
