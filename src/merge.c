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

#include <merge.h>
#include <gnc_utilities.h>
#include <output.h>

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
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Public constants ----------------------------------------------------------*/
/* Public functions ----------------------------------------------------------*/

GList *getTransactionToImport(Account *fromAccount, Account *toAccount)
{
    OUT_DEBUG("Check transactions to merge\n");
    GList *list = NULL;
    SplitList *splits = xaccAccountGetSplitList(fromAccount);

    int total = 0;
    int exactMatch = 0;
    int partialMatch = 0;
    int negMatch = 0;
    int toBeCreated = 0;

    for (GList *elem = splits; elem; elem = g_list_next(elem)) {
        // From the split we get the transaction
        Split *split = (Split *)elem->data;
        total++;

        Transaction *tx = xaccSplitGetParent(split);
        time64 transactionDate = xaccTransGetDate(tx);

        gnc_numeric amount = xaccSplitGetAmount(split);

        Split *matchingSplit;
        Split_Match_Result_e matchResult =
            find_split_in_account(toAccount, xaccTransGetDescription(tx), transactionDate,
                                  gnc_numeric_neg(amount), &matchingSplit);

        bool createTx = true;
        switch (matchResult) {
        case SPLIT_MATCH_EXACT:
            OUT_DEBUG("Exact matching split found:\n");
            printSplit(OUT_LEVEL_DEBUG, split);
            printSplit(OUT_LEVEL_DEBUG, matchingSplit);
            OUT_DEBUG("\n");
            createTx = false;
            exactMatch++;
            break;
        case SPLIT_MATCH_DATE_AMOUNT:
            OUT_INFO("Matching split with different description found:\n");
            printSplit(OUT_LEVEL_INFO, split);
            printSplit(OUT_LEVEL_INFO, matchingSplit);
            OUT_INFO("\n");
            createTx = false;
            partialMatch++;
            break;
        case SPLIT_MATCH_DATE_NEGAMOUNT:
            OUT_WARNING("Split not found, but negated one found!!! Tx will be created!\n");
            printSplit(OUT_LEVEL_WARNING, split);
            printSplit(OUT_LEVEL_WARNING, matchingSplit);
            OUT_DEBUG("\n");
            negMatch++;
            break;
        case SPLIT_MATCH_NONE:
            // Nothing to do
            break;
        }

        if (createTx) {
            toBeCreated++;
            Data_t *data = g_new0(Data_t, 1);
            GDateTime *date_time = g_date_time_new_from_unix_local(transactionDate);
            data->date = date_time;

            data->description = g_string_new(xaccTransGetDescription(tx));
            data->number = g_string_new(xaccTransGetNum(tx));
            data->amountCents = (amount.num * 100) / amount.denom;

            // Add data to the list
            list = g_list_append(list, data);

            if (NULL == list) {
                OUT_FATAL("Error creating list!\n");
            }
        }
    }

    OUT_INFO("Transactions to merge summary:\n");
    OUT_INFO(" total                  %5d\n", total);
    OUT_INFO(" excluded exact match   %5d\n", exactMatch);
    OUT_INFO(" excluded partial match %5d\n", partialMatch);
    OUT_WARNING(" included neg match     %5d\n", negMatch);
    OUT_INFO(" to be created          %5d\n", toBeCreated);

    return list;
}
/* Private functions ---------------------------------------------------------*/
/* Callbacks -----------------------------------------------------------------*/
/*** end of file ***/
