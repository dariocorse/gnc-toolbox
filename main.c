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

#include <output.h>
#include <merge.h>
#include <match_params.h>
#include <match_params_loader.h>

#include <gnc_utilities.h>
#include <matcher.h>
#include <data.h>
#include <utilities.h>
#include <cli.h>

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    int count;
    int amountCents;
} Count_t;

// --------------------- Private function prototypes -----------
static bool validateDestinationAccounts(QofSession *session, const ParserSetting_t *settings,
                                        const char *config_file, const char *book_ref);
static bool matchFilter(Data_t *transaction, MatchType_e type, MatchData_t value);

// --------------------- Public functions ---------------------

static void addTxToCount(Data_t *tx, Count_t *count)
{
    count->count++;
    count->amountCents += tx->amountCents;
}

int main(int argc, char *argv[])
{
    // Parse command line arguments using new CLI module
    CliArgs_t args;
    cli_init_args(&args);

    if (!cli_parse_args(argc, argv, &args)) {
        cli_cleanup_args(&args);
        exit(EXIT_FAILURE);
    }

    // Handle special operations
    if (args.operation == CLI_OPERATION_HELP || args.help_requested) {
        cli_print_help(args.operation);
        cli_cleanup_args(&args);
        exit(EXIT_SUCCESS);
    }

    if (args.operation == CLI_OPERATION_VERSION || args.version_requested) {
        cli_print_version();
        cli_cleanup_args(&args);
        exit(EXIT_SUCCESS);
    }

    // Configure output verbosity
    cli_configure_output_level(args.verbosity);

    // Validate arguments
    if (!cli_validate_args(&args)) {
        OUT_ERROR("Use 'gnc-toolbox help' for usage information.\n");
        cli_cleanup_args(&args);
        exit(EXIT_FAILURE);
    }

    // Load configuration file
    ParserSetting_t *parserSettings = mpl_load(args.config_file);
    if (parserSettings == NULL) {
        OUT_ERROR("Failed to load rule file '%s'. Check the CSV row and field errors above.\n",
                  args.config_file);
        cli_cleanup_args(&args);
        exit(EXIT_FAILURE);
    }

    if (OUT_VERBOSE("Parser settings:\n")) {
        for (GList *entry = parserSettings->cols; NULL != entry; entry = entry->next) {
            ColParams_t *c = (ColParams_t *)entry->data;
            if (c != NULL) {
                OUT_VERBOSE(" header col type %d\n", c->type);
            } else {
                OUT_WARNING(" null column parameters found\n");
            }
        }
    }

    // Init gnc engine and load book
    init(argc, argv);
    QofSession *session = loadBook(args.book_ref);
    if (!session) {
        OUT_FATAL("Failed to load GnuCash book. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    if (OUT_VERBOSE("Accounts splits:\n")) {
        for (GList *entry = parserSettings->matches; NULL != entry; entry = entry->next) {
            MatchEntry_t *m = (MatchEntry_t *)entry->data;
            if (m != NULL && m->data.gstring != NULL) {
                OUT_VERBOSE("  '%s'\n", m->data.gstring->str);
                for (GList *split = m->accountSplits; NULL != split; split = split->next) {
                    AccountSplit_t *s = (AccountSplit_t *)split->data;
                    if (s != NULL && s->accountName != NULL) {
                        OUT_VERBOSE("    '%s' %d%%\n", s->accountName->str, s->percentage);
                        if (!accountExists(session, s->accountName->str)) {
                            OUT_WARNING("      destination account '%s' from rule file '%s' was "
                                        "not found in book '%s'\n",
                                        s->accountName->str, args.config_file, args.book_ref);
                        }
                    } else {
                        OUT_WARNING("    null account split data found\n");
                    }
                }
            } else {
                OUT_WARNING("  null match entry found\n");
            }
        }
    }

    GList *data = NULL;

    Account *baseAccount = getAccount(session, parserSettings->baseAccount->str);
    if (NULL == baseAccount) {
        OUT_ERROR("Base account '%s' from rule file '%s' was not found in book '%s'. Use the exact "
                  "account name or full path like Parent:Child.\n",
                  parserSettings->baseAccount->str, args.config_file, args.book_ref);
        closebook(session, false);
        end();
        free_parser_settings_t(parserSettings);
        cli_cleanup_args(&args);
        exit(EXIT_FAILURE);
    }

    if (parserSettings->unmatchedAccount != NULL &&
        !accountExists(session, parserSettings->unmatchedAccount->str)) {
        OUT_ERROR("Unmatched account '%s' from rule file '%s' was not found in book '%s'. Create "
                  "it in GnuCash or update the UNMATCHED row.\n",
                  parserSettings->unmatchedAccount->str, args.config_file, args.book_ref);
        closebook(session, false);
        end();
        free_parser_settings_t(parserSettings);
        cli_cleanup_args(&args);
        exit(EXIT_FAILURE);
    }

    if (!validateDestinationAccounts(session, parserSettings, args.config_file, args.book_ref)) {
        closebook(session, false);
        end();
        free_parser_settings_t(parserSettings);
        cli_cleanup_args(&args);
        exit(EXIT_FAILURE);
    }

    if (args.operation == CLI_OPERATION_IMPORT) {
        // Load actual data
        OUT_VERBOSE("======== CSV data ===========\n");
        data = m_match(args.csv_file, parserSettings);
    } else if (args.operation == CLI_OPERATION_MERGE) {
        QofSession *otherSession = loadBook(args.source_book_ref);
        if (!otherSession) {
            OUT_FATAL("Failed to load source GnuCash book '%s'. Exiting.\n", args.source_book_ref);
            exit(EXIT_FAILURE);
        }
        Account *accountFrom = getAccount(otherSession, args.source_account);
        if (accountFrom != NULL) {
            data = getTransactionToImport(accountFrom, baseAccount);
        }
        // Close otherSession used only to evaluate what to import
        closebook(otherSession, false);

        if (NULL == accountFrom) {
            OUT_ERROR("Source account '%s' was not found in source book '%s'. Use the exact "
                      "account name or full path like Parent:Child.\n",
                      args.source_account, args.source_book_ref);
            closebook(session, false);
            end();
            free_parser_settings_t(parserSettings);
            cli_cleanup_args(&args);
            exit(EXIT_FAILURE);
        }
    }

    int txTotalCents = 0;
    if (OUT_VERBOSE("======== Transactions to create: ========\n"))
        for (GList *row = data; NULL != row; row = row->next) {
            Data_t *tx = (Data_t *)row->data;
            txTotalCents += tx->amountCents;
            printData(OUT_LEVEL_VERBOSE, tx);
        }

    int txToImport = 0;
    if (data != NULL) {
        txToImport = g_list_length(data);
    }
    OUT_SUMMARY("TOTAL to create: %d transactions. Amount %8.2f\n", txToImport,
                txTotalCents / 100.0);

    if (txToImport > 0) {
        // Let's try matching the transactions
        OUT_INFO("======== matching ===========\n");

        Count_t ignored = {0, 0};
        Count_t filtered = {0, 0};
        Count_t duplicates = {0, 0};
        Count_t matched = {0, 0};
        Count_t unmatched = {0, 0};
        Count_t error = {0, 0};

        for (GList *row = data; NULL != row; row = row->next) {
            Data_t *tx = (Data_t *)row->data;

            // Sign control is now handled per-column via multipliers

            // Data to be filtered?
            bool createTx = true;
            if (parserSettings->filters != NULL) {
                for (GList *entry = parserSettings->filters; NULL != entry; entry = entry->next) {
                    TxFilter_t *filter = (TxFilter_t *)entry->data;
                    if (matchFilter(tx, filter->type, filter->value)) {
                        const char *description =
                            tx->description != NULL ? tx->description->str : "(no description)";
                        if (filter->type == MATCH_TYPE_DATE_LESS_OR_EQUALS) {
                            gchar *filter_date =
                                g_date_time_format(filter->value.gdatetime, "%Y-%m-%d");
                            OUT_DEBUG("Tx %s matches filter %d %s\n", description, filter->type,
                                      filter_date != NULL ? filter_date : "(invalid date)");
                            g_free(filter_date);
                        } else {
                            const char *filter_value = filter->value.gstring != NULL
                                                           ? filter->value.gstring->str
                                                           : "(no filter value)";
                            OUT_DEBUG("Tx %s matches filter %d %s\n", description, filter->type,
                                      filter_value);
                        }
                        createTx = false;
                        break;
                    }
                }
            }

            if (!createTx) {
                printData(OUT_LEVEL_INFO, tx);
                OUT_INFO("  filtered out\n");
                addTxToCount(tx, &filtered);
            } else {
                // Check if already registered
                if (tx_in_account(baseAccount, tx)) {
                    if (args.no_duplicates) {
                        printData(OUT_LEVEL_INFO, tx);
                        OUT_INFO("  ignoring already stored transaction\n");
                        createTx = false;
                        addTxToCount(tx, &ignored);
                    } else {
                        printData(OUT_LEVEL_WARNING, tx);
                        OUT_WARNING("  transaction already stored but saved again\n");
                        addTxToCount(tx, &duplicates);
                    }
                }
            }

            if (createTx) {
                // Look for matching
                MatchEntry_t *match = NULL;
                for (GList *entry = parserSettings->matches; NULL != entry; entry = entry->next) {
                    MatchEntry_t *m = (MatchEntry_t *)entry->data;

                    if (matchFilter(tx, m->type, m->data)) {
                        match = m;
                        break;
                    }
                }

                if (NULL == match) {
                    printData(OUT_LEVEL_WARNING, tx);
                    addTxToCount(tx, &unmatched);
                    if (parserSettings->unmatchedAccount == NULL) {
                        OUT_ERROR("  NO MATCHES and no UNMATCHED account is configured in rule "
                                  "file '%s'. Transaction was not imported.\n",
                                  args.config_file);
                        addTxToCount(tx, &error);
                        continue;
                    }

                    OUT_WARNING(
                        "  NO MATCHES: importing to unmatched account '%s' for manual review\n",
                        parserSettings->unmatchedAccount->str);
                } else {
                    printData(OUT_LEVEL_VERBOSE, tx);
                    OUT_VERBOSE("  matches:\n");
                    for (GList *split = match->accountSplits; NULL != split; split = split->next) {
                        AccountSplit_t *s = (AccountSplit_t *)split->data;
                        OUT_VERBOSE("    - %s %d\n", s->accountName->str, s->percentage);
                    }
                    addTxToCount(tx, &matched);
                }

                MatchEntry_t unmatchedMatch = {0};
                AccountSplit_t unmatchedSplit = {0};
                GList *unmatchedSplits = NULL;
                MatchEntry_t *matchToCreate = match;

                if (matchToCreate == NULL) {
                    unmatchedSplit.accountName = parserSettings->unmatchedAccount;
                    unmatchedSplit.percentage = 100;
                    unmatchedMatch.accountSplits = unmatchedSplits =
                        g_list_append(NULL, &unmatchedSplit);
                    matchToCreate = &unmatchedMatch;
                }

                if (!createTransaction(session, baseAccount, tx, matchToCreate)) {
                    addTxToCount(tx, &error);
                }

                if (unmatchedSplits != NULL) {
                    g_list_free(unmatchedSplits);
                }
            }
        }

        OUT_SUMMARY("== Summary  number    amount ==\n");
        OUT_SUMMARY("  filtered   %5d  %8.2f\n", filtered.count, filtered.amountCents / 100.0);
        if (args.no_duplicates) {
            OUT_SUMMARY("  ignored    %5d  %8.2f\n", ignored.count, ignored.amountCents / 100.0);
        }
        if (!args.no_duplicates) {
            OUT_SUMMARY("  duplicates %5d  %8.2f\n", duplicates.count,
                        duplicates.amountCents / 100.0);
        }
        OUT_SUMMARY("  matched    %5d  %8.2f\n", matched.count, matched.amountCents / 100.0);
        OUT_SUMMARY("  unmatched  %5d  %8.2f\n", unmatched.count, unmatched.amountCents / 100.0);
        OUT_SUMMARY("  errors     %5d  %8.2f\n", error.count, error.amountCents / 100.0);
        OUT_SUMMARY("TOTAL        %5d  %8.2f\n", g_list_length(data), txTotalCents / 100.0);
    }

    // Clean up memory
    if (data != NULL) {
        for (GList *node = data; node != NULL; node = node->next) {
            Data_t *tx_data = (Data_t *)node->data;
            free_data_t(tx_data);
        }
        g_list_free(data);
    }

    // Clean up parser settings
    free_parser_settings_t(parserSettings);

    // Clean up CLI arguments
    cli_cleanup_args(&args);

    // End gnc session and save unless is a dry run
    bool save_ok = closebook(session, !args.dry_run);
    end();

    // Just a message to signal the application has started
    OUT_DEBUG("End!\n");

    if (!save_ok) {
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

// --------------------- Private functions --------------------

static bool validateDestinationAccounts(QofSession *session, const ParserSetting_t *settings,
                                        const char *config_file, const char *book_ref)
{
    if (settings == NULL) {
        OUT_ERROR("Rule file '%s' did not load parser settings.\n", config_file);
        return false;
    }

    for (GList *entry = settings->matches; entry != NULL; entry = entry->next) {
        MatchEntry_t *match = (MatchEntry_t *)entry->data;
        const char *missing_account = NULL;
        if (!matchAccountsExist(session, match, &missing_account)) {
            OUT_ERROR("Destination account '%s' from rule file '%s' was not found in book '%s'. "
                      "Create it in GnuCash or update the matching rule before importing.\n",
                      missing_account != NULL ? missing_account : "(invalid account split)",
                      config_file, book_ref);
            return false;
        }
    }

    return true;
}

static bool matchFilter(Data_t *transaction, MatchType_e type, MatchData_t value)
{
    switch (type) {
    case MATCH_TYPE_CONTAINS:
        if (gstring_contains(transaction->description, value.gstring)) {
            return true;
        }
        break;
    case MATCH_TYPE_PATTERN:
        if (gstring_matches_pattern(transaction->description, value.gstring->str)) {
            return true;
        }
        break;
    case MATCH_TYPE_REGEX:
        if (gstring_matches_regex(transaction->description, value.gstring->str)) {
            return true;
        }
        break;
    case MATCH_TYPE_DATE_LESS_OR_EQUALS:
        if (g_date_time_compare(transaction->date, value.gdatetime) <= 0) {
            return true;
        }
        break;
    }
    return false;
}

// End of file
