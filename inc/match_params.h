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

#ifndef MATCH_PARAMS_H
#define MATCH_PARAMS_H

#include <glib.h>

typedef struct {
    GString *accountName;
    int percentage;
} AccountSplit_t;

typedef enum {
    MATCH_TYPE_CONTAINS,
    MATCH_TYPE_PATTERN,
    MATCH_TYPE_REGEX,
    MATCH_TYPE_DATE_LESS_OR_EQUALS
} MatchType_e;

typedef union {
    GString *gstring;
    GDateTime *gdatetime;
} MatchData_t;

typedef struct {
    MatchType_e type;
    MatchData_t data;
    GList *accountSplits;
} MatchEntry_t;

typedef enum {
    COL_TYPE_NONE,
    COL_TYPE_DATE,
    COL_TYPE_NUM,
    COL_TYPE_DESCR,
    COL_TYPE_AMOUNT
} ColType_e;

typedef struct {
    ColType_e type;
    int multiplier;
    char format[20];
} ColParams_t;

typedef struct {
    char delimiter;          // CSV field delimiter (e.g., ';', ',')
    char quote;              // CSV quote character (e.g., '"')
    char decimal_separator;  // Decimal point character (e.g., '.', ',')
    char thousand_separator; // Thousand separator (e.g., ',', '.', ' ')
    int skip_header_rows;    // Number of header rows to skip
} CSVParseParam_t;

typedef struct {
    MatchType_e type;
    MatchData_t value;
} TxFilter_t;

typedef struct {
    CSVParseParam_t csvParams;
    GList *cols;
    GList *matches;
    GList *filters;
    GString *baseAccount;
    GString *unmatchedAccount;
    GString *concat;
} ParserSetting_t;

#endif // MATCH_PARAMS_H
