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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>

#include "test_book_integration.h"
#include "../inc/gnc_utilities.h"
#include "../inc/match_params_loader.h"
#include "../inc/matcher.h"
#include "../inc/merge.h"
#include "../inc/output.h"
#include "../inc/utilities.h"

#include <gnucash/Account.h>
#include <gnucash/gnc-commodity.h>
#include <gnucash/qofbook.h>
#include <gnucash/qofsession.h>

typedef struct {
    char *path;
    QofSession *session;
} TempBook_t;

static gboolean session_has_error(QofSession *session)
{
    const char *message = qof_session_get_error_message(session);
    return message != NULL && *message != '\0';
}

static void ensure_engine_initialized(void)
{
    static gboolean initialized = FALSE;

    if (!initialized) {
        char *argv[] = {"test_gnc-toolbox", NULL};
        init(1, argv);
        atexit(end);
        initialized = TRUE;
    }
}

static char *create_temp_path(const char *pattern)
{
    gchar *path = NULL;
    gint fd = g_file_open_tmp(pattern, &path, NULL);
    g_assert_cmpint(fd, >=, 0);
    close(fd);
    return path;
}

static gnc_commodity *get_test_currency(QofBook *book)
{
    gnc_commodity_table *table = gnc_commodity_table_get_table(book);
    g_assert_nonnull(table);

    gnc_commodity *currency = gnc_commodity_table_lookup(table, "CURRENCY", "USD");
    g_assert_nonnull(currency);
    return currency;
}

static Account *create_account(QofBook *book, Account *parent, const char *name,
                               GNCAccountType type)
{
    gnc_commodity *currency = get_test_currency(book);
    Account *account = xaccMallocAccount(book);
    g_assert_nonnull(account);

    xaccAccountBeginEdit(account);
    xaccAccountSetName(account, name);
    xaccAccountSetType(account, type);
    xaccAccountSetCommodity(account, currency);
    xaccAccountSetCommoditySCU(account, gnc_commodity_get_fraction(currency));
    gnc_account_append_child(parent, account);
    xaccAccountCommitEdit(account);

    return account;
}

static TempBook_t create_temp_book(void)
{
    TempBook_t temp_book = {0};
    temp_book.path = create_temp_path("gnc-toolbox-book-XXXXXX.gnucash");
    unlink(temp_book.path);

    QofBook *book = qof_book_new();
    g_assert_nonnull(book);

    temp_book.session = qof_session_new(book);
    g_assert_nonnull(temp_book.session);

    qof_session_begin(temp_book.session, temp_book.path, SESSION_NEW_OVERWRITE);
    g_assert_false(session_has_error(temp_book.session));

    Account *root = gnc_book_get_root_account(book);
    if (root == NULL) {
        root = gnc_account_create_root(book);
        gnc_book_set_root_account(book, root);
    }
    g_assert_nonnull(root);

    return temp_book;
}

static void destroy_temp_book(TempBook_t *temp_book)
{
    if (temp_book == NULL) {
        return;
    }

    if (temp_book->session != NULL) {
        qof_session_destroy(temp_book->session);
        temp_book->session = NULL;
    }

    if (temp_book->path != NULL) {
        unlink(temp_book->path);
        g_free(temp_book->path);
        temp_book->path = NULL;
    }
}

static void rollback_unsaved_temp_book(TempBook_t *temp_book)
{
    if (temp_book == NULL || temp_book->session == NULL) {
        return;
    }

    /*
     * GnuCash 4.8 can raise a fatal DBI unlock assertion when destroying a
     * newly-created, unsaved file session. End the session here and leave
     * process teardown to reclaim it. Saved and reloaded sessions still go
     * through closebook(), which fully destroys them.
     */
    qof_session_end(temp_book->session);
    temp_book->session = NULL;
}

static MatchEntry_t *create_single_split_match(const char *account_name)
{
    MatchEntry_t *entry = g_new0(MatchEntry_t, 1);
    AccountSplit_t *split = g_new0(AccountSplit_t, 1);

    entry->type = MATCH_TYPE_CONTAINS;
    entry->data.gstring = g_string_new("test");

    split->accountName = g_string_new(account_name);
    split->percentage = 100;

    entry->accountSplits = g_list_append(NULL, split);
    g_assert_nonnull(entry->accountSplits);

    return entry;
}

static Data_t *create_test_data(const char *date_text, const char *number, const char *description,
                                gint amount_cents)
{
    Data_t *data = g_new0(Data_t, 1);
    data->date = getDateTimeFromText(date_text, "YYYY-MM-DD");
    data->number = g_string_new(number);
    data->description = g_string_new(description);
    data->amountCents = amount_cents;
    data->hasValidAmount = TRUE;
    return data;
}

static gboolean find_exact_split(Account *account, Data_t *data)
{
    Split *found_split = NULL;
    Split_Match_Result_e result =
        find_split_in_account(account, data->description->str, g_date_time_to_unix(data->date),
                              gnc_numeric_create(data->amountCents, 100), &found_split);

    return result == SPLIT_MATCH_EXACT && found_split != NULL;
}

static void test_real_book_create_save_reload(void)
{
    ensure_engine_initialized();
    out_setLevel(OUT_LEVEL_ERROR);

    TempBook_t book = create_temp_book();
    QofBook *qof_book = qof_session_get_book(book.session);
    Account *root = gnc_book_get_root_account(qof_book);

    Account *checking = create_account(qof_book, root, "Roundtrip Checking", ACCT_TYPE_BANK);
    create_account(qof_book, root, "Roundtrip Expense", ACCT_TYPE_EXPENSE);

    Data_t *data = create_test_data("2024-01-15", "R001", "Roundtrip Save", -1250);
    MatchEntry_t *match = create_single_split_match("Roundtrip Expense");

    g_assert_true(createTransaction(book.session, checking, data, match));
    g_assert_true(closebook(book.session, true));
    book.session = NULL;

    QofSession *reloaded = loadBook(book.path);
    g_assert_nonnull(reloaded);
    g_assert_true(accountExists(reloaded, "Roundtrip Checking"));
    g_assert_true(accountExists(reloaded, "Roundtrip Expense"));

    Account *reloaded_checking = getAccount(reloaded, "Roundtrip Checking");
    g_assert_nonnull(reloaded_checking);
    g_assert_true(find_exact_split(reloaded_checking, data));

    closebook(reloaded, false);

    free_data_t(data);
    free_match_entry_t(match);
    destroy_temp_book(&book);
}

static void test_real_import_pipeline_roundtrip(void)
{
    ensure_engine_initialized();
    out_setLevel(OUT_LEVEL_ERROR);

    TempBook_t book = create_temp_book();
    QofBook *qof_book = qof_session_get_book(book.session);
    Account *root = gnc_book_get_root_account(qof_book);
    create_account(qof_book, root, "Import Checking", ACCT_TYPE_BANK);
    create_account(qof_book, root, "Food Expense", ACCT_TYPE_EXPENSE);

    char *config_path = create_temp_path("gnc-toolbox-import-config-XXXXXX.cfg");
    char *csv_path = create_temp_path("gnc-toolbox-import-data-XXXXXX.csv");

    g_file_set_contents(config_path,
                        "CSVPARAMS,DELIMITER=;,SKIP_ROWS=0\n"
                        "Import Checking, -\n"
                        "DATE:FORMAT=YYYY-MM-DD,DESCR,AMOUNT,NUM\n"
                        "Coffee,Food Expense,100\n",
                        -1, NULL);

    g_file_set_contents(csv_path, "2024-02-01;Coffee Shop;-4.50;REF-COFFEE\n", -1, NULL);

    ParserSetting_t *settings = mpl_load(config_path);
    g_assert_nonnull(settings);
    g_assert_nonnull(settings->matches);

    GList *transactions = m_match(csv_path, settings);
    g_assert_nonnull(transactions);
    g_assert_cmpuint(g_list_length(transactions), ==, 1);

    Data_t *transaction = (Data_t *)transactions->data;
    MatchEntry_t *match = (MatchEntry_t *)settings->matches->data;
    Account *base_account = getAccount(book.session, "Import Checking");
    g_assert_nonnull(base_account);

    g_assert_true(createTransaction(book.session, base_account, transaction, match));
    g_assert_true(closebook(book.session, true));
    book.session = NULL;

    QofSession *reloaded = loadBook(book.path);
    g_assert_nonnull(reloaded);

    Account *reloaded_checking = getAccount(reloaded, "Import Checking");
    g_assert_nonnull(reloaded_checking);
    g_assert_true(tx_in_account(reloaded_checking, transaction));

    closebook(reloaded, false);

    g_list_free_full(transactions, (GDestroyNotify)free_data_t);
    free_parser_settings_t(settings);
    unlink(config_path);
    unlink(csv_path);
    g_free(config_path);
    g_free(csv_path);
    destroy_temp_book(&book);
}

static void test_real_import_unmatched_transaction_uses_review_account(void)
{
    ensure_engine_initialized();
    out_setLevel(OUT_LEVEL_ERROR);

    TempBook_t book = create_temp_book();
    QofBook *qof_book = qof_session_get_book(book.session);
    Account *root = gnc_book_get_root_account(qof_book);

    Account *checking = create_account(qof_book, root, "Unmatched Checking", ACCT_TYPE_BANK);
    Account *review = create_account(qof_book, root, "Review Later", ACCT_TYPE_EXPENSE);

    Data_t *data = create_test_data("2024-04-10", "UNMATCHED-01", "Needs manual category", -4321);
    MatchEntry_t *review_match = create_single_split_match("Review Later");

    g_assert_true(createTransaction(book.session, checking, data, review_match));

    Split *found_review_split = NULL;
    Split_Match_Result_e review_result =
        find_split_in_account(review, data->description->str, g_date_time_to_unix(data->date),
                              gnc_numeric_create(4321, 100), &found_review_split);

    g_assert_cmpint(review_result, ==, SPLIT_MATCH_EXACT);
    g_assert_nonnull(found_review_split);

    rollback_unsaved_temp_book(&book);

    free_data_t(data);
    free_match_entry_t(review_match);
    destroy_temp_book(&book);
}

static void test_real_import_rejects_missing_match(void)
{
    ensure_engine_initialized();
    out_setLevel(OUT_LEVEL_ERROR);

    TempBook_t book = create_temp_book();
    QofBook *qof_book = qof_session_get_book(book.session);
    Account *root = gnc_book_get_root_account(qof_book);
    Account *checking = create_account(qof_book, root, "Strict Checking", ACCT_TYPE_BANK);

    Data_t *data = create_test_data("2024-04-11", "NO-MATCH-01", "No destination match", -1000);

    g_assert_false(createTransaction(book.session, checking, data, NULL));

    rollback_unsaved_temp_book(&book);

    free_data_t(data);
    destroy_temp_book(&book);
}

static void test_real_import_missing_destination_account_creates_no_partial_transaction(void)
{
    ensure_engine_initialized();
    out_setLevel(OUT_LEVEL_ERROR);

    TempBook_t book = create_temp_book();
    QofBook *qof_book = qof_session_get_book(book.session);
    Account *root = gnc_book_get_root_account(qof_book);
    Account *checking = create_account(qof_book, root, "Partial Safety Checking", ACCT_TYPE_BANK);

    Data_t *data =
        create_test_data("2024-04-12", "MISSING-DEST-01", "Missing destination account", -777);
    MatchEntry_t *match = create_single_split_match("Missing Expense Account");

    g_assert_false(createTransaction(book.session, checking, data, match));
    g_assert_false(find_exact_split(checking, data));

    rollback_unsaved_temp_book(&book);

    free_data_t(data);
    free_match_entry_t(match);
    destroy_temp_book(&book);
}

static void test_real_merge_pipeline_roundtrip(void)
{
    ensure_engine_initialized();
    out_setLevel(OUT_LEVEL_ERROR);

    TempBook_t source_book = create_temp_book();
    TempBook_t target_book = create_temp_book();

    QofBook *source_qof_book = qof_session_get_book(source_book.session);
    Account *source_root = gnc_book_get_root_account(source_qof_book);
    Account *source_checking =
        create_account(source_qof_book, source_root, "Source Checking", ACCT_TYPE_BANK);
    create_account(source_qof_book, source_root, "Source Offset", ACCT_TYPE_EXPENSE);

    Data_t *source_tx = create_test_data("2024-03-05", "MERGE-01", "Merge Candidate", -1875);
    MatchEntry_t *source_match = create_single_split_match("Source Offset");
    g_assert_true(createTransaction(source_book.session, source_checking, source_tx, source_match));
    g_assert_true(closebook(source_book.session, true));
    source_book.session = NULL;

    QofBook *target_qof_book = qof_session_get_book(target_book.session);
    Account *target_root = gnc_book_get_root_account(target_qof_book);
    create_account(target_qof_book, target_root, "Target Checking", ACCT_TYPE_BANK);
    create_account(target_qof_book, target_root, "Target Offset", ACCT_TYPE_EXPENSE);
    g_assert_true(closebook(target_book.session, true));
    target_book.session = NULL;

    QofSession *source_reloaded = loadBook(source_book.path);
    QofSession *target_reloaded = loadBook(target_book.path);
    g_assert_nonnull(source_reloaded);
    g_assert_nonnull(target_reloaded);

    Account *from_account = getAccount(source_reloaded, "Source Checking");
    Account *to_account = getAccount(target_reloaded, "Target Checking");
    g_assert_nonnull(from_account);
    g_assert_nonnull(to_account);

    GList *transactions = getTransactionToImport(from_account, to_account);
    g_assert_nonnull(transactions);
    g_assert_cmpuint(g_list_length(transactions), ==, 1);

    Data_t *merged_tx = (Data_t *)transactions->data;
    g_assert_cmpstr(merged_tx->description->str, ==, "Merge Candidate");
    g_assert_cmpstr(merged_tx->number->str, ==, "MERGE-01");
    g_assert_cmpint(merged_tx->amountCents, ==, -1875);

    g_assert_true(closebook(source_reloaded, false));
    source_reloaded = NULL;

    MatchEntry_t *target_match = create_single_split_match("Target Offset");
    g_assert_true(createTransaction(target_reloaded, to_account, merged_tx, target_match));
    g_assert_true(closebook(target_reloaded, true));

    QofSession *target_verify = loadBook(target_book.path);
    g_assert_nonnull(target_verify);

    Account *verified_target_account = getAccount(target_verify, "Target Checking");
    g_assert_nonnull(verified_target_account);
    g_assert_true(tx_in_account(verified_target_account, merged_tx));

    closebook(target_verify, false);

    g_list_free_full(transactions, (GDestroyNotify)free_data_t);
    free_data_t(source_tx);
    free_match_entry_t(source_match);
    free_match_entry_t(target_match);
    destroy_temp_book(&source_book);
    destroy_temp_book(&target_book);
}

static void test_real_book_accepts_qualified_match_account_paths(void)
{
    ensure_engine_initialized();
    out_setLevel(OUT_LEVEL_ERROR);

    TempBook_t book = create_temp_book();
    QofBook *qof_book = qof_session_get_book(book.session);
    Account *root = gnc_book_get_root_account(qof_book);

    Account *assets = create_account(qof_book, root, "Qualified Assets", ACCT_TYPE_ASSET);
    Account *checking = create_account(qof_book, assets, "Checking", ACCT_TYPE_BANK);
    Account *expenses = create_account(qof_book, root, "Qualified Expenses", ACCT_TYPE_EXPENSE);
    create_account(qof_book, expenses, "Holiday", ACCT_TYPE_EXPENSE);

    Data_t *data = create_test_data("2024-12-25", "QUAL-01", "Qualified path transaction", -12345);
    MatchEntry_t *match = create_single_split_match("Qualified Expenses:Holiday");

    gboolean created = createTransaction(book.session, checking, data, match);

    if (!created) {
        g_test_message(
            "Expected fully-qualified account path 'Qualified Expenses:Holiday' to resolve");
        g_test_fail();
    }

    rollback_unsaved_temp_book(&book);

    free_data_t(data);
    free_match_entry_t(match);
    destroy_temp_book(&book);
}

void test_book_integration_register(void)
{
    g_test_add_func("/book_integration/create_save_reload", test_real_book_create_save_reload);
    g_test_add_func("/book_integration/import_pipeline_roundtrip",
                    test_real_import_pipeline_roundtrip);
    g_test_add_func("/book_integration/import_unmatched_transaction_uses_review_account",
                    test_real_import_unmatched_transaction_uses_review_account);
    g_test_add_func("/book_integration/import_rejects_missing_match",
                    test_real_import_rejects_missing_match);
    g_test_add_func("/book_integration/import_missing_destination_account_creates_no_partial_tx",
                    test_real_import_missing_destination_account_creates_no_partial_transaction);
    g_test_add_func("/book_integration/merge_pipeline_roundtrip",
                    test_real_merge_pipeline_roundtrip);
    g_test_add_func("/book_integration/accepts_qualified_match_account_paths",
                    test_real_book_accepts_qualified_match_account_paths);
}
