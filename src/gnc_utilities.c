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

/* Includes ------------------------------------------------------------------*/
#define HAVE_SCANF_LLD 1

#include <output.h>
#include <gnc_utilities.h>
#include <utilities.h>

#include <unistd.h>
#include <fcntl.h>

#include <gnucash/gnc-engine.h>
#include <gnucash/Account.h>
#include <gnucash/Transaction.h>
#include <gnucash/Split.h>
#include <gnucash/gnc-session.h>
#include <gnucash/qofsession.h>
#include <gnucash/qofbook.h>
#include <gnucash/gnc-commodity.h>

/* Private definitions -------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private typedefs ----------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
// static QofBook *book;
// static QofSession *session;

/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void operationOngoingCallback(const char *message, double percent);
static bool has_session_error_message(const char *error_message);
static void qof_session_save_hook(QofSession *session, void *user_data);
static const char *qof_session_get_error_hook(QofSession *session, void *user_data);
static Account *lookup_account(Account *root_account, const char *account_name);
static Account *lookup_account_by_path(Account *root_account, const char *account_name);

/* Public constants ----------------------------------------------------------*/
/* Public functions ----------------------------------------------------------*/
void init(int argc, char *argv[])
{
    // Load the book
    gnc_engine_init(argc, argv);
    // Example of accessing engine parameter
    OUT_DEBUG("Separation char '%c'\n", *gnc_get_account_separator_string());
}

QofSession *loadBook(const char *url)
{
    // Check if the GnuCash file is already locked/open
    if (gnc_is_locked(url)) {
        OUT_FATAL("Error: The GnuCash file appears to be already open in another application.\n");
        OUT_FATAL("       Please close the file in GnuCash or any other application before "
                  "proceeding.\n");
        OUT_FATAL("       File: %s\n", url);
        return NULL;
    }

    // Open test book
    QofBook *book = qof_book_new();
    QofSession *session = qof_session_new(book);
    qof_session_begin(session, url, SESSION_NORMAL_OPEN);

    // Load file and report progress through callback
    OUT_VERBOSE("Loading file\n");
    qof_session_load(session, operationOngoingCallback);
    OUT_DEBUG("\n");
    const char *error = qof_session_get_error_message(session);
    if (has_session_error_message(error)) {
        OUT_FATAL("Load error: '%s'\n", error);
        // Ensure session resources are released before returning.
        // qof_session_destroy() also destroys the book owned by the session.
        qof_session_destroy(session);
        return NULL;
    }
    return session;
}

bool accountExists(QofSession *session, const char *account)
{
    QofBook *book = qof_session_get_book(session);
    Account *root_account = gnc_book_get_root_account(book);
    if (NULL == root_account) {
        return false;
    }
    if (NULL == lookup_account(root_account, account)) {
        return false;
    }
    return true;
}

bool matchAccountsExist(QofSession *session, const MatchEntry_t *match,
                        const char **missing_account)
{
    static const char invalid_split[] = "(invalid account split)";

    if (missing_account != NULL) {
        *missing_account = NULL;
    }

    if (session == NULL || match == NULL || match->accountSplits == NULL) {
        if (missing_account != NULL) {
            *missing_account = invalid_split;
        }
        return false;
    }

    QofBook *book = qof_session_get_book(session);
    Account *root_account = gnc_book_get_root_account(book);
    if (root_account == NULL) {
        if (missing_account != NULL) {
            *missing_account = invalid_split;
        }
        return false;
    }

    for (GList *split = match->accountSplits; split != NULL; split = split->next) {
        AccountSplit_t *s = (AccountSplit_t *)split->data;
        if (s == NULL || s->accountName == NULL || s->accountName->len == 0) {
            if (missing_account != NULL) {
                *missing_account = invalid_split;
            }
            return false;
        }

        if (lookup_account(root_account, s->accountName->str) == NULL) {
            if (missing_account != NULL) {
                *missing_account = s->accountName->str;
            }
            return false;
        }
    }

    return true;
}

// Helper to extract only the date components from a time64 value
static void extract_date_parts(time64 trans_date, int *year, int *month, int *day)
{
    // Convert the time64 value to a GDateTime
    GDateTime *date_time = g_date_time_new_from_unix_local(trans_date);

    // Extract year, month, and day
    *year = g_date_time_get_year(date_time);
    *month = g_date_time_get_month(date_time);
    *day = g_date_time_get_day_of_month(date_time);

    // Release allocated memory
    g_date_time_unref(date_time);
}

static gboolean split_date_is(Split *split, int year, int month, int day)
{
    Transaction *trans = xaccSplitGetParent(split);
    if (trans == NULL) {
        return FALSE;
    }

    // Compare the date values
    time64 trans_date = xaccTransGetDate(trans);
    int tyear, tmonth, tday;
    extract_date_parts(trans_date, &tyear, &tmonth, &tday);
    if (tyear != year || tmonth != month || tday != day) {
        return FALSE;
    }

    return TRUE;
}

static gboolean split_description_is(Split *split, const gchar *description)
{
    Transaction *trans = xaccSplitGetParent(split);
    if (trans == NULL) {
        return FALSE;
    }

    const gchar *trans_desc = xaccTransGetDescription(trans);
    if (g_strcmp0(trans_desc, description) != 0) {
        return FALSE;
    }

    return TRUE;
}

bool tx_in_account(Account *account, Data_t *data)
{
    time64 dateTime64 = g_date_time_to_unix(data->date);
    gnc_numeric amount = gnc_numeric_create(data->amountCents, 100); // 1.23

    Split *matchingSplit;
    Split_Match_Result_e matchResult =
        find_split_in_account(account, data->description->str, dateTime64, amount, &matchingSplit);

    switch (matchResult) {
    case SPLIT_MATCH_EXACT:
        OUT_DEBUG("Exact matching transaction found:\n");
        printSplit(OUT_LEVEL_DEBUG, matchingSplit);
        return true;
        break;
    case SPLIT_MATCH_DATE_AMOUNT:
        OUT_INFO("Matching transaction with different description found:\n");
        printSplit(OUT_LEVEL_INFO, matchingSplit);
        return true;
        break;
    case SPLIT_MATCH_DATE_NEGAMOUNT:
        OUT_WARNING("Transaction not found, but negated one found!!! Tx will be created!\n");
        printSplit(OUT_LEVEL_WARNING, matchingSplit);
        break;
    case SPLIT_MATCH_NONE:
        // Nothing to do
        break;
    }
    return false;
}

Split_Match_Result_e find_split_in_account(Account *account, const gchar *description, time64 date,
                                           gnc_numeric amount, Split **foundSplit)
{
    Split_Match_Result_e matchResult = SPLIT_MATCH_NONE;
    int year, month, day;
    extract_date_parts(date, &year, &month, &day);
    GList *splits = xaccAccountGetSplitList(account);

    for (GList *node = splits; node != NULL; node = node->next) {
        Split *split = (Split *)node->data;

        Split_Match_Result_e newResult = SPLIT_MATCH_NONE;

        // Date must match before amount and description are compared.
        if (split_date_is(split, year, month, day)) {
            // Amount or negated amount must match
            gnc_numeric split_amount = xaccSplitGetAmount(split);
            if (gnc_numeric_equal(split_amount, gnc_numeric_neg(amount))) {
                newResult = SPLIT_MATCH_DATE_NEGAMOUNT;
            } else if (gnc_numeric_equal(split_amount, amount)) {
                // At least date and amount are the same
                newResult = SPLIT_MATCH_DATE_AMOUNT;

                if (split_description_is(split, description)) {
                    newResult = SPLIT_MATCH_EXACT;
                }
            }
        }

        if (matchResult < newResult) {
            if (foundSplit != NULL) {
                *foundSplit = split;
            }
            matchResult = newResult;
        }
    }

    return matchResult;
}

void printSplit(OutLevel_e level, Split *split)
{
    Transaction *tx = xaccSplitGetParent(split);
    gnc_numeric amount = xaccSplitGetAmount(split);
    GDateTime *date_time = g_date_time_new_from_unix_local(xaccTransGetDate(tx));
    gchar *datetime_str = g_date_time_format(date_time, "%Y-%m-%d %H:%M:%S");

    out(level, "%s %10s %s %s\n", datetime_str, xaccTransGetNum(tx), xaccTransGetDescription(tx),
        gnc_numeric_to_string(amount));

    // Release allocated memory
    g_date_time_unref(date_time);
    g_free(datetime_str);
}

Account *getAccount(QofSession *session, const char *accountName)
{
    QofBook *book = qof_session_get_book(session);
    Account *root_account = gnc_book_get_root_account(book);
    // Code adapted from https://wiki.gnucash.org/wiki/Using_the_API

    // Here's the code to create a new transaction (abbreviated by "txn")
    return lookup_account(root_account, accountName);
}

bool save_session_with_hooks(QofSession *session, GncSessionSaveHook save_hook,
                             GncSessionErrorHook error_hook, void *user_data,
                             const char **error_message)
{
    if (error_message != NULL) {
        *error_message = NULL;
    }

    if (session == NULL || save_hook == NULL || error_hook == NULL) {
        if (error_message != NULL) {
            *error_message = "invalid save handler configuration";
        }
        return false;
    }

    save_hook(session, user_data);

    const char *save_error = error_hook(session, user_data);
    if (error_message != NULL) {
        *error_message = save_error;
    }

    return !has_session_error_message(save_error);
}

bool createTransaction(QofSession *session, Account *account, Data_t *data, MatchEntry_t *match)
{
    if (match == NULL) {
        OUT_ERROR(
            "Cannot create transaction '%s': no destination account match was provided. Configure "
            "an UNMATCHED account for transactions that must be reviewed by hand.\n",
            data != NULL && data->description != NULL ? data->description->str
                                                      : "(no description)");
        return false;
    }

    if (session == NULL || account == NULL || data == NULL || data->date == NULL ||
        data->description == NULL) {
        OUT_ERROR("Cannot create transaction: invalid transaction data or session state.\n");
        return false;
    }

    const char *missing_account = NULL;
    if (!matchAccountsExist(session, match, &missing_account)) {
        OUT_ERROR("Cannot create transaction '%s': destination account '%s' was not found in "
                  "the book. Use the exact account name or a full path like Parent:Child in "
                  "the rule file.\n",
                  data->description->str, missing_account != NULL ? missing_account : "(unknown)");
        return false;
    }

    QofBook *book = qof_session_get_book(session);
    Account *root_account = gnc_book_get_root_account(book);
    // Code adapted from https://wiki.gnucash.org/wiki/Using_the_API

    time64 dateTime64 = g_date_time_to_unix(data->date);
    gnc_numeric amount = gnc_numeric_create(data->amountCents, 100); // 1.23

    Transaction *transaction = xaccMallocTransaction(book); // Creating the new transaction
    xaccTransBeginEdit(transaction);                        // Opening it for editing

    // The txn is created. We can set the txn-related data fields:
    xaccTransSetDatePostedSecs(transaction, dateTime64); // Date of this txn

    if (NULL != data->number) {
        xaccTransSetNum(transaction, data->number->str);
    }
    xaccTransSetDescription(transaction, data->description->str);

    // As GnuCash is capable of multi-currency handling, every txn must have a "transaction
    // currency" set:
    gnc_commodity *currency = xaccAccountGetCommodity(account);
    xaccTransSetCurrency(transaction, currency); // Txn must have a commodity/currency set

    // The actual amount/value of a transaction is given by its two or more splits. We create the
    // first split:
    Split *split = xaccMallocSplit(book);
    xaccTransAppendSplit(transaction, split); // Split belongs to this txn
    xaccAccountInsertSplit(account, split);   // Split belongs to this account
    xaccSplitSetValue(split, amount);
    xaccSplitSetAmount(split, amount);

    printData(OUT_LEVEL_DEBUG, data);

    // Then we create the second split so that we have a normal two-split transaction, as usual in
    // the double-entry bookkeeping:
    int residualAmount = data->amountCents;
    OUT_DEBUG("residualAmount %d\n", residualAmount);
    for (GList *split = match->accountSplits; NULL != split; split = split->next) {
        AccountSplit_t *s = (AccountSplit_t *)split->data;

        Account *splitAccount = lookup_account(root_account, s->accountName->str);
        if (NULL == splitAccount) {
            OUT_ERROR("Internal account validation error for '%s'.\n", s->accountName->str);
            xaccTransDestroy(transaction);
            xaccTransCommitEdit(transaction);
            return false;
        }

        Split *split2 = xaccMallocSplit(book);
        xaccTransAppendSplit(transaction, split2);    // Split belongs to this txn
        xaccAccountInsertSplit(splitAccount, split2); // Split belongs to this account

        int amount = data->amountCents * s->percentage / 100;
        if (split->next == NULL) {
            // To avoid rounding errors on the cent, the last split uses the remaining amount
            amount = residualAmount;
            OUT_DEBUG("  amount %d\n", amount);
        } else {
            // Reduce the remaining amount
            residualAmount -= amount;
            OUT_DEBUG("  amount %d residualAmount %d\n", amount, residualAmount);
        }

        gnc_numeric other_amount =
            gnc_numeric_create(-amount, 100); // -1.23, the negative value for above
        xaccSplitSetValue(split2, other_amount);
        xaccSplitSetAmount(split2, other_amount);

        OUT_DEBUG("    '%s' %d%%\n", s->accountName->str, s->percentage);
    }

    // In the end, we must finish the transaction editing by calling the CommitEdit function:
    xaccTransCommitEdit(transaction); // We are finished editing

    return true;
}

bool closebook(QofSession *session, bool save)
{
    if (session == NULL) {
        return false;
    }

    bool success = true;

    // We save the changes (we don't use the callback here)
    if (save) {
        OUT_VERBOSE("Saving file");

        gboolean suppress_save_output = !g_test_initialized();
        int stdout_fd = -1;
        int stderr_fd = -1;

        if (suppress_save_output) {
            // Temporarily suppress both stdout and stderr to hide gz_thread_func EOF message.
            fflush(stdout);
            fflush(stderr);

            stdout_fd = dup(STDOUT_FILENO);
            stderr_fd = dup(STDERR_FILENO);
            int dev_null = open("/dev/null", O_WRONLY);

            if (dev_null != -1) {
                dup2(dev_null, STDOUT_FILENO);
                dup2(dev_null, STDERR_FILENO);
                close(dev_null);
            }
        }

        const char *error_message = NULL;
        success = save_session_with_hooks(session, qof_session_save_hook,
                                          qof_session_get_error_hook, NULL, &error_message);

        if (suppress_save_output) {
            // Restore stdout and stderr.
            if (stdout_fd != -1) {
                dup2(stdout_fd, STDOUT_FILENO);
                close(stdout_fd);
            }
            if (stderr_fd != -1) {
                dup2(stderr_fd, STDERR_FILENO);
                close(stderr_fd);
            }
        }

        OUT_VERBOSE("\n");

        if (!success) {
            OUT_ERROR("Save error: '%s'\n",
                      error_message != NULL ? error_message : "unknown error");
        }
    }

    // Destroying the session also ends it and releases the backend lock.
    qof_session_destroy(session);
    return success;
}

void end()
{
    gnc_engine_shutdown();
}

/* Private functions ---------------------------------------------------------*/
static void operationOngoingCallback(const char *message, double percent)
{
    static int lastPercent = 100;

    int perc = (int)percent;
    if (0 != message) {
        OUT_INFO("\n%s (%d%%)\n", message, perc);
    } else if (perc % 10 == 0 && lastPercent != perc) {
        if (0 == perc) {
            OUT_DEBUG("%d%%", (int)percent);
        } else {
            OUT_DEBUG(".. %d%%", (int)percent);
        }
        lastPercent = perc;
    }
}

static bool has_session_error_message(const char *error_message)
{
    return error_message != NULL && error_message[0] != '\0';
}

static void qof_session_save_hook(QofSession *session, void *user_data)
{
    (void)user_data;
    qof_session_save(session, operationOngoingCallback);
}

static const char *qof_session_get_error_hook(QofSession *session, void *user_data)
{
    (void)user_data;
    return qof_session_get_error_message(session);
}

static Account *lookup_account(Account *root_account, const char *account_name)
{
    if (root_account == NULL || account_name == NULL) {
        return NULL;
    }

    Account *account = gnc_account_lookup_by_full_name(root_account, account_name);
    if (account != NULL) {
        return account;
    }

    account = lookup_account_by_path(root_account, account_name);
    if (account != NULL) {
        return account;
    }

    return gnc_account_lookup_by_name(root_account, account_name);
}

static Account *lookup_account_by_path(Account *root_account, const char *account_name)
{
    const char *separator = gnc_get_account_separator_string();
    if (separator == NULL || separator[0] == '\0' || strstr(account_name, separator) == NULL) {
        separator = ":";
    }

    if (strstr(account_name, separator) == NULL) {
        return NULL;
    }

    gchar **parts = g_strsplit(account_name, separator, -1);
    Account *current = root_account;

    for (guint i = 0; parts[i] != NULL; i++) {
        if (parts[i][0] == '\0') {
            current = NULL;
            break;
        }

        const char *current_name = xaccAccountGetName(current);
        if (i == 0 && g_strcmp0(current_name, parts[i]) == 0) {
            continue;
        }

        current = gnc_account_lookup_by_name(current, parts[i]);
        if (current == NULL) {
            break;
        }
    }

    g_strfreev(parts);
    return current;
}

/* Callbacks -----------------------------------------------------------------*/
/*** end of file ***/
