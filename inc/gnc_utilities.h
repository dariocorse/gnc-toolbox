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
#ifndef GNC_UTILITIES_H_
#define GNC_UTILITIES_H_

/* Includes ------------------------------------------------------------------*/

#include <output.h>
#include <match_params.h>
#include <data.h>

#include <gnucash/Account.h>

#include <stdbool.h>
/* Definitions ---------------------------------------------------------------*/
typedef enum {
    SPLIT_MATCH_NONE,
    SPLIT_MATCH_DATE_NEGAMOUNT,
    SPLIT_MATCH_DATE_AMOUNT,
    // SPLIT_MATCH_NUM_DATE_AMOUT,
    SPLIT_MATCH_EXACT,
} Split_Match_Result_e;

typedef void (*GncSessionSaveHook)(QofSession *session, void *user_data);
typedef const char *(*GncSessionErrorHook)(QofSession *session, void *user_data);

/* Macros --------------------------------------------------------------------*/
/* Typedefs ------------------------------------------------------------------*/
/* Constants -----------------------------------------------------------------*/
/* Variables -----------------------------------------------------------------*/

/* Function prototypes -------------------------------------------------------*/
void init(int argc, char *argv[]);
QofSession *loadBook(const char *url);

bool accountExists(QofSession *session, const char *account);
bool matchAccountsExist(QofSession *session, const MatchEntry_t *match,
                        const char **missing_account);

bool tx_in_account(Account *account, Data_t *data);
bool createTransaction(QofSession *session, Account *account, Data_t *data, MatchEntry_t *match);

Split_Match_Result_e find_split_in_account(Account *account, const gchar *description, time64 date,
                                           gnc_numeric amount, Split **foundSplit);

void printSplit(OutLevel_e level, Split *split);
Account *getAccount(QofSession *session, const char *accountName);

bool save_session_with_hooks(QofSession *session, GncSessionSaveHook save_hook,
                             GncSessionErrorHook error_hook, void *user_data,
                             const char **error_message);

bool closebook(QofSession *session, bool save);
void end();

/* Callback prototypes -------------------------------------------------------*/

#endif /* GNC_UTILITIES_H_ */
/*** end of file ***/
