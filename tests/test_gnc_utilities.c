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

#define HAVE_SCANF_LLD 1

#include <glib.h>
#include "test_gnc_utilities.h"
#include "../inc/gnc_utilities.h"

typedef struct {
    int save_calls;
    int error_calls;
    QofSession *last_session;
    const char *error_message;
} SaveHookState_t;

static void fake_save_hook(QofSession *session, void *user_data)
{
    SaveHookState_t *state = (SaveHookState_t *)user_data;
    state->save_calls++;
    state->last_session = session;
}

static const char *fake_error_hook(QofSession *session, void *user_data)
{
    SaveHookState_t *state = (SaveHookState_t *)user_data;
    state->error_calls++;
    state->last_session = session;
    return state->error_message;
}

static void test_save_session_with_hooks_success(void)
{
    SaveHookState_t state = {0};
    const char *error_message = "sentinel";
    QofSession *session = (QofSession *)0x1;

    gboolean result =
        save_session_with_hooks(session, fake_save_hook, fake_error_hook, &state, &error_message);

    g_assert_true(result);
    g_assert_cmpint(state.save_calls, ==, 1);
    g_assert_cmpint(state.error_calls, ==, 1);
    g_assert_true(state.last_session == session);
    g_assert_null(error_message);
}

static void test_save_session_with_hooks_failure(void)
{
    SaveHookState_t state = {.error_message = "write failed"};
    const char *error_message = NULL;
    QofSession *session = (QofSession *)0x2;

    gboolean result =
        save_session_with_hooks(session, fake_save_hook, fake_error_hook, &state, &error_message);

    g_assert_false(result);
    g_assert_cmpint(state.save_calls, ==, 1);
    g_assert_cmpint(state.error_calls, ==, 1);
    g_assert_true(state.last_session == session);
    g_assert_cmpstr(error_message, ==, "write failed");
}

static void test_save_session_with_hooks_invalid_args(void)
{
    const char *error_message = NULL;

    g_assert_false(
        save_session_with_hooks(NULL, fake_save_hook, fake_error_hook, NULL, &error_message));
    g_assert_cmpstr(error_message, ==, "invalid save handler configuration");

    error_message = NULL;
    g_assert_false(
        save_session_with_hooks((QofSession *)0x3, NULL, fake_error_hook, NULL, &error_message));
    g_assert_cmpstr(error_message, ==, "invalid save handler configuration");
}

void test_gnc_utilities_register(void)
{
    g_test_add_func("/gnc_utilities/save_session_with_hooks_success",
                    test_save_session_with_hooks_success);
    g_test_add_func("/gnc_utilities/save_session_with_hooks_failure",
                    test_save_session_with_hooks_failure);
    g_test_add_func("/gnc_utilities/save_session_with_hooks_invalid_args",
                    test_save_session_with_hooks_invalid_args);
}
