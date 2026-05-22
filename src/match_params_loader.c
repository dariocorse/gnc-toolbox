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
#include <match_params.h>
#include <csv_helper.h>
#include <utilities.h>

/* Private definitions -------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private typedefs ----------------------------------------------------------*/
typedef enum {
    FILE_PARSING_STATUS_CSVPARAMS = 0,
    FILE_PARSING_STATUS_FILTERS,
    FILE_PARSING_STATUS_BASEACCOUNT,
    FILE_PARSING_STATUS_UNMATCHED,
    FILE_PARSING_STATUS_COLUMNS,
    FILE_PARSING_STATUS_MATCHES
} FileParsingStatus_e;

/* Public constants ----------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static ParserSetting_t *parserSettings;

static FileParsingStatus_e fileParsingStatus;
static bool ignoreLine;

static MatchEntry_t *entry;
static AccountSplit_t *split;
static TxFilter_t *filter;
static bool filterDateParsed;
static bool parseError;

/* Private function prototypes -----------------------------------------------*/
static void processField(int rowIndex, int fieldIndex, void *s, size_t len);
static void processRow(int rowIndex, char terminationChar);

static void parseFilter(int fieldIndex, char *s, size_t len);
static void parseColumn(int fieldIndex, char *s, size_t len);
static void parseMatch(int fieldIndex, char *s, size_t len);
static void parseBaseAccountData(int fieldIndex, char *s, size_t len);
static void parseUnmatchedAccountData(int fieldIndex, char *s, size_t len);
static void parseCSVParams(int fieldIndex, char *s, size_t len);
static bool regexIsValid(const char *pattern, size_t len);
static bool validateParserSettings(const char *file);

/* Public functions ----------------------------------------------------------*/
ParserSetting_t *mpl_load(const char *file)
{
    parserSettings = g_new0(ParserSetting_t, 1);
    // Sign control now handled per-column via multipliers
    parserSettings->concat = g_string_new(" - ");
    parserSettings->csvParams.delimiter = ',';
    parserSettings->csvParams.quote = '"';
    parserSettings->csvParams.decimal_separator = '.';
    parserSettings->csvParams.thousand_separator = ',';
    parserSettings->csvParams.skip_header_rows = 0;

    ignoreLine = false;
    filter = NULL;
    filterDateParsed = false;
    parseError = false;
    fileParsingStatus = FILE_PARSING_STATUS_CSVPARAMS;

    // Create default CSV parameters for configuration file parsing
    CSVParseParam_t config_csv_params = {.delimiter = ',',
                                         .quote = '"',
                                         .decimal_separator = '.',
                                         .thousand_separator = ',',
                                         .skip_header_rows = 0};
    csv_load(file, &config_csv_params, processField, processRow);

    if (!parseError && !validateParserSettings(file)) {
        parseError = true;
    }

    if (parseError) {
        free_parser_settings_t(parserSettings);
        parserSettings = NULL;
    }

    return parserSettings;
}

/* Private functions ---------------------------------------------------------*/
static void processField(int rowIndex, int fieldIndex, void *s, size_t len)
{
    (void)rowIndex;

    char *data = (char *)s;
    if (0 == fieldIndex) {
        if (2 <= len && '#' == data[0] && '#' == data[1]) {
            // Comment line, we ignore it
            ignoreLine = true;
            entry = NULL;
        } else {
            if (FILE_PARSING_STATUS_CSVPARAMS == fileParsingStatus &&
                !startsWith(s, len, "CSVPARAMS")) {
                // First valid row doesn't start with CSVPARAMS: just move to parsing filters
                fileParsingStatus = FILE_PARSING_STATUS_FILTERS;
            }
            if (FILE_PARSING_STATUS_FILTERS == fileParsingStatus &&
                !startsWith(s, len, "FILTEROUT")) {
                // First valid row doesn't start with FILTEROUT: just move to parsing base account
                // info
                fileParsingStatus = FILE_PARSING_STATUS_BASEACCOUNT;
            }
            if (FILE_PARSING_STATUS_COLUMNS == fileParsingStatus &&
                startsWith(s, len, "UNMATCHED")) {
                fileParsingStatus = FILE_PARSING_STATUS_UNMATCHED;
            }
        }
    }
    if (!ignoreLine) {
        switch (fileParsingStatus) {
        case FILE_PARSING_STATUS_CSVPARAMS:
            parseCSVParams(fieldIndex, s, len);
            break;
        case FILE_PARSING_STATUS_FILTERS:
            parseFilter(fieldIndex, s, len);
            break;
        case FILE_PARSING_STATUS_BASEACCOUNT:
            parseBaseAccountData(fieldIndex, s, len);
            break;
        case FILE_PARSING_STATUS_UNMATCHED:
            parseUnmatchedAccountData(fieldIndex, s, len);
            break;
        case FILE_PARSING_STATUS_COLUMNS:
            parseColumn(fieldIndex, s, len);
            break;
        case FILE_PARSING_STATUS_MATCHES:
            parseMatch(fieldIndex, s, len);
            break;
        }
    }
}

static void processRow(int rowIndex, char terminationChar)
{
    (void)rowIndex;
    (void)terminationChar;

    if (ignoreLine) {
        ignoreLine = false;
        return;
    } else {
        switch (fileParsingStatus) {
        case FILE_PARSING_STATUS_CSVPARAMS:
            fileParsingStatus = FILE_PARSING_STATUS_FILTERS;
            break;
        case FILE_PARSING_STATUS_FILTERS:
            if (filter->type == MATCH_TYPE_DATE_LESS_OR_EQUALS && !filterDateParsed) {
                csv_printMessage(OUT_LEVEL_ERROR, "Invalid DATE_LEQ filter: field 3 must be a date "
                                                  "that matches the format in field 4");
                parseError = true;
                if (filter->value.gstring != NULL) {
                    g_string_free(filter->value.gstring, TRUE);
                }
                g_free(filter);
                filter = NULL;
                break;
            }
            parserSettings->filters = g_list_append(parserSettings->filters, filter);
            filter = NULL;
            break;
        case FILE_PARSING_STATUS_BASEACCOUNT:
            // Nothing to do
            fileParsingStatus = FILE_PARSING_STATUS_COLUMNS;
            break;
        case FILE_PARSING_STATUS_UNMATCHED:
            if (parserSettings->unmatchedAccount == NULL ||
                parserSettings->unmatchedAccount->len == 0) {
                csv_printMessage(OUT_LEVEL_ERROR,
                                 "Invalid UNMATCHED row: expected UNMATCHED,<ACCOUNT_NAME>");
                parseError = true;
            }
            fileParsingStatus = FILE_PARSING_STATUS_COLUMNS;
            break;
        case FILE_PARSING_STATUS_COLUMNS:
            // Nothing to do
            fileParsingStatus = FILE_PARSING_STATUS_MATCHES;
            break;
        case FILE_PARSING_STATUS_MATCHES: {
            // Check match parameters are correctly formatted
            guint splitCount = g_list_length(entry->accountSplits);
            if (0 >= splitCount) {
                csv_printMessage(
                    OUT_LEVEL_FATAL,
                    "Invalid match row: add at least one destination account after the match text");
            }
            if (1 == splitCount) {
                // Set percentage to 100%
                split->percentage = 100;
            } else {
                // Check that the percentage sum is 100
                int sum = 0;
                for (GList *split = entry->accountSplits; NULL != split; split = split->next) {
                    AccountSplit_t *s = (AccountSplit_t *)split->data;
                    sum += s->percentage;
                }
                if (100 != sum) {
                    csv_printMessagef(
                        OUT_LEVEL_FATAL,
                        "Invalid match row: split percentages sum to %d, expected 100", sum);
                }
            }
            parserSettings->matches = g_list_append(parserSettings->matches, entry);
        } break;
        }
    }
}

static void parseFilter(int fieldIndex, char *s, size_t len)
{
    switch (fieldIndex) {
    case 0:
        // Create new filter
        filter = g_new0(TxFilter_t, 1);
        break;
    case 1:
        // Look for filter type
        if (startsWith(s, len, "CONTAINS")) {
            filter->type = MATCH_TYPE_CONTAINS;
        } else if (startsWith(s, len, "REGEX")) {
            filter->type = MATCH_TYPE_REGEX;
        } else if (startsWith(s, len, "PATTERN") || startsWith(s, len, "GLOB")) {
            filter->type = MATCH_TYPE_PATTERN;
        } else if (startsWith(s, len, "DATE_LEQ")) {
            filter->type = MATCH_TYPE_DATE_LESS_OR_EQUALS;
            filterDateParsed = false;
        } else {
            csv_printMessage(
                OUT_LEVEL_FATAL,
                "Unknown filter type: expected CONTAINS, PATTERN, GLOB, REGEX, or DATE_LEQ");
        }
        break;
    case 2:
        // Set filter value
        if (filter->type == MATCH_TYPE_REGEX && !regexIsValid(s, len)) {
            csv_printMessage(OUT_LEVEL_ERROR,
                             "Invalid REGEX filter: value is not a valid GLib regular expression; "
                             "use PATTERN for * and ? wildcards");
            parseError = true;
        }
        filter->value.gstring = g_string_new_len(s, len);
        break;
    case 3:
        if (filter->type == MATCH_TYPE_DATE_LESS_OR_EQUALS) {
            // Get format and populate GDateTime
            char *format_str = getCStringFromData(s, len);

            // Get GDateTime
            GDateTime *datetime = getDateTimeFromText(filter->value.gstring->str, format_str);
            if (datetime) {
                g_string_free(filter->value.gstring, TRUE);
                filter->value.gdatetime = datetime;
                filterDateParsed = true;
            }

            // Clear
            g_free(format_str);
        }
        break;
    }
}

static void parseBaseAccountData(int fieldIndex, char *s, size_t len)
{
    if (0 == fieldIndex) {
        parserSettings->baseAccount = g_string_new_len(s, len);
    } else if (1 == fieldIndex) {
        g_string_free(parserSettings->concat, TRUE);
        parserSettings->concat = g_string_new_len(s, len);
    }
}

static void parseUnmatchedAccountData(int fieldIndex, char *s, size_t len)
{
    if (0 == fieldIndex) {
        return;
    }

    if (1 == fieldIndex) {
        char *account_name = g_strndup(s, len);
        if (!is_valid_account_name(account_name)) {
            csv_printMessage(
                OUT_LEVEL_ERROR,
                "Invalid UNMATCHED account name: remove unsupported characters < > | * ? \"");
            parseError = true;
            g_free(account_name);
            return;
        }

        safe_g_string_free(&parserSettings->unmatchedAccount);
        parserSettings->unmatchedAccount = g_string_new_len(s, len);
        g_free(account_name);
    }
}

static void parseColumn(int fieldIndex, char *s, size_t len)
{
    (void)fieldIndex;

    ColParams_t *col = g_new0(ColParams_t, 1);
    if (startsWith(s, len, "IGNORE") || startsWith(s, len, "NONE")) {
        col->type = COL_TYPE_NONE;
    } else if (startsWith(s, len, "NUM")) {
        col->type = COL_TYPE_NUM;
    } else if (startsWith(s, len, "DESCR")) {
        col->type = COL_TYPE_DESCR;
    } else if (startsWith(s, len, "-AMOUNT")) {
        col->type = COL_TYPE_AMOUNT;
        col->multiplier = -1; // Negate this column's values
    } else if (startsWith(s, len, "AMOUNT")) {
        col->type = COL_TYPE_AMOUNT;
        col->multiplier = 1; // Keep this column's values as-is
    } else if (startsWith(s, len, "DATE")) {
        col->type = COL_TYPE_DATE;
        // Look for the format
        strcpy(col->format, "DD-MM-YYYY");
        if (startsWith(s, len, "DATE:FORMAT=")) {
            size_t copy_len = (len > 12) ? (len - 12) : 0;
            if (copy_len >= sizeof(col->format)) {
                copy_len = sizeof(col->format) - 1;
            }
            memcpy(col->format, s + 12, copy_len);
            col->format[copy_len] = '\0';
        }
        OUT_DEBUG("Date format='%s'\n", col->format);
    } else {
        csv_printMessage(OUT_LEVEL_FATAL,
                         "Unknown column type: expected DATE, DATE:FORMAT=<pattern>, DESCR, NUM, "
                         "AMOUNT, -AMOUNT, IGNORE, or NONE");
    }

    parserSettings->cols = g_list_append(parserSettings->cols, col);
}

static void parseMatch(int fieldIndex, char *s, size_t len)
{
    if (0 == fieldIndex) {
        // Create entry
        entry = g_new0(MatchEntry_t, 1);
        entry->accountSplits = NULL;

        // Check match type
        if (len > 1 && *s == '$') {
            // Shell-style pattern match.
            entry->type = MATCH_TYPE_PATTERN;
            entry->data.gstring = g_string_new_len(s + 1, len - 1);
        } else {
            // Contains
            entry->type = MATCH_TYPE_CONTAINS;
            entry->data.gstring = g_string_new_len(s, len);
        }
    } else {
        if (0 != fieldIndex % 2) {
            // Account name validation
            if (NULL == entry) {
                csv_printMessage(OUT_LEVEL_FATAL,
                                 "Invalid match row: account name appeared before match text");
                return;
            }

            // Validate account name length and content
            if (len == 0) {
                csv_printMessage(OUT_LEVEL_FATAL, "Invalid match row: account name is empty");
                return;
            }

            // Create null-terminated string for validation
            char *account_name = g_strndup(s, len);
            if (!is_valid_account_name(account_name)) {
                csv_printMessage(
                    OUT_LEVEL_FATAL,
                    "Invalid account name: remove unsupported characters < > | * ? \"");
                g_free(account_name);
                return;
            }

            split = g_new0(AccountSplit_t, 1);
            split->accountName = g_string_new_len(s, len);
            entry->accountSplits = g_list_append(entry->accountSplits, split);

            if (NULL == entry->accountSplits) {
                csv_printMessage(OUT_LEVEL_FATAL, "Internal error while storing account split");
            }

            g_free(account_name);
        } else {
            // Account split percentage validation
            if (NULL == entry || NULL == split) {
                csv_printMessage(OUT_LEVEL_FATAL,
                                 "Invalid match row: percentage appeared before an account name");
                return;
            }

            // Validate percentage field length
            if (len == 0) {
                csv_printMessage(OUT_LEVEL_FATAL, "Invalid match row: percentage is empty");
                return;
            }

            if (len > 3) {
                csv_printMessage(OUT_LEVEL_FATAL,
                                 "Invalid match row: percentage must be an integer from 1 to 100");
                return;
            }

            // Create null-terminated string and validate content
            char *perc_str = g_strndup(s, len);

            // Check if string contains only digits
            for (size_t i = 0; i < len; i++) {
                if (!g_ascii_isdigit(s[i])) {
                    csv_printMessage(OUT_LEVEL_FATAL,
                                     "Invalid match row: percentage must contain digits only");
                    g_free(perc_str);
                    return;
                }
            }

            int perc = atoi(perc_str);
            g_free(perc_str);

            // Validate percentage range
            if (perc <= 0 || perc > 100) {
                csv_printMessage(OUT_LEVEL_FATAL,
                                 "Invalid match row: percentage must be in the range 1-100");
                return;
            }

            split->percentage = perc;
        }
    }
}

static void parseCSVParams(int fieldIndex, char *s, size_t len)
{
    char *field_str = getCStringFromData(s, len);

    if (fieldIndex == 0) {
        // Reset to the documented defaults before applying row overrides.
        parserSettings->csvParams.delimiter = ',';
        parserSettings->csvParams.quote = '"';
        parserSettings->csvParams.decimal_separator = '.';
        parserSettings->csvParams.thousand_separator = ',';
        parserSettings->csvParams.skip_header_rows = 0;
    }

    // Parse CSV parameter fields
    if (startsWith(s, len, "DELIMITER=")) {
        parserSettings->csvParams.delimiter = field_str[10]; // Skip "DELIMITER="
        OUT_DEBUG("Set CSV delimiter to: '%c'\n", parserSettings->csvParams.delimiter);
    } else if (startsWith(s, len, "QUOTE=")) {
        parserSettings->csvParams.quote = field_str[6]; // Skip "QUOTE="
        OUT_DEBUG("Set CSV quote to: '%c'\n", parserSettings->csvParams.quote);
    } else if (startsWith(s, len, "DECIMAL_SEP=")) {
        parserSettings->csvParams.decimal_separator = field_str[12]; // Skip "DECIMAL_SEP="
        OUT_DEBUG("Set decimal separator to: '%c'\n", parserSettings->csvParams.decimal_separator);
    } else if (startsWith(s, len, "THOUSAND_SEP=")) {
        parserSettings->csvParams.thousand_separator = field_str[13]; // Skip "THOUSAND_SEP="
        OUT_DEBUG("Set thousand separator to: '%c'\n",
                  parserSettings->csvParams.thousand_separator);
    } else if (startsWith(s, len, "SKIP_ROWS=")) {
        parserSettings->csvParams.skip_header_rows = atoi(field_str + 10); // Skip "SKIP_ROWS="
        OUT_DEBUG("Set skip header rows to: %d\n", parserSettings->csvParams.skip_header_rows);
    }

    g_free(field_str);
}

static bool regexIsValid(const char *pattern, size_t len)
{
    char *pattern_str = getCStringFromData((void *)pattern, len);
    GError *error = NULL;
    GRegex *regex = g_regex_new(pattern_str, 0, 0, &error);

    if (regex != NULL) {
        g_regex_unref(regex);
    }
    if (error != NULL) {
        g_error_free(error);
    }
    g_free(pattern_str);

    return regex != NULL;
}

static bool validateParserSettings(const char *file)
{
    bool valid = true;
    bool has_date = false;
    bool has_description = false;
    bool has_amount = false;

    if (parserSettings->csvParams.delimiter == '\0') {
        OUT_ERROR("Rule file '%s': CSVPARAMS DELIMITER cannot be empty.\n", file);
        valid = false;
    }
    if (parserSettings->csvParams.quote == '\0') {
        OUT_ERROR("Rule file '%s': CSVPARAMS QUOTE cannot be empty.\n", file);
        valid = false;
    }
    if (parserSettings->csvParams.decimal_separator == '\0') {
        OUT_ERROR("Rule file '%s': CSVPARAMS DECIMAL_SEP cannot be empty.\n", file);
        valid = false;
    }
    if (parserSettings->csvParams.thousand_separator == '\0') {
        OUT_ERROR("Rule file '%s': CSVPARAMS THOUSAND_SEP cannot be empty.\n", file);
        valid = false;
    }
    if (parserSettings->csvParams.decimal_separator ==
        parserSettings->csvParams.thousand_separator) {
        OUT_ERROR("Rule file '%s': DECIMAL_SEP and THOUSAND_SEP must be different.\n", file);
        valid = false;
    }
    if (parserSettings->csvParams.skip_header_rows < 0) {
        OUT_ERROR("Rule file '%s': SKIP_ROWS cannot be negative.\n", file);
        valid = false;
    }

    if (parserSettings->baseAccount == NULL || parserSettings->baseAccount->len == 0) {
        OUT_ERROR("Rule file '%s': missing base account row.\n", file);
        valid = false;
    } else if (!is_valid_account_name(parserSettings->baseAccount->str)) {
        OUT_ERROR("Rule file '%s': invalid base account name.\n", file);
        valid = false;
    }

    if (parserSettings->unmatchedAccount != NULL &&
        !is_valid_account_name(parserSettings->unmatchedAccount->str)) {
        OUT_ERROR("Rule file '%s': invalid UNMATCHED account name.\n", file);
        valid = false;
    }

    if (parserSettings->cols == NULL) {
        OUT_ERROR("Rule file '%s': missing column definition row.\n", file);
        valid = false;
    }

    for (GList *node = parserSettings->cols; node != NULL; node = node->next) {
        ColParams_t *col = (ColParams_t *)node->data;
        if (col == NULL) {
            OUT_ERROR("Rule file '%s': invalid empty column definition.\n", file);
            valid = false;
            continue;
        }

        if (col->type == COL_TYPE_DATE) {
            has_date = true;
        } else if (col->type == COL_TYPE_DESCR) {
            has_description = true;
        } else if (col->type == COL_TYPE_AMOUNT) {
            has_amount = true;
        }
    }

    if (!has_date) {
        OUT_ERROR("Rule file '%s': column definitions must include DATE.\n", file);
        valid = false;
    }
    if (!has_description) {
        OUT_ERROR("Rule file '%s': column definitions must include DESCR.\n", file);
        valid = false;
    }
    if (!has_amount) {
        OUT_ERROR("Rule file '%s': column definitions must include AMOUNT or -AMOUNT.\n", file);
        valid = false;
    }

    if (parserSettings->matches == NULL && parserSettings->unmatchedAccount == NULL) {
        OUT_ERROR("Rule file '%s': add at least one match row or an UNMATCHED account.\n", file);
        valid = false;
    }

    for (GList *node = parserSettings->matches; node != NULL; node = node->next) {
        MatchEntry_t *match = (MatchEntry_t *)node->data;
        if (match == NULL) {
            OUT_ERROR("Rule file '%s': invalid empty match row.\n", file);
            valid = false;
            continue;
        }

        if ((match->type == MATCH_TYPE_CONTAINS || match->type == MATCH_TYPE_PATTERN ||
             match->type == MATCH_TYPE_REGEX) &&
            (match->data.gstring == NULL || match->data.gstring->len == 0)) {
            OUT_ERROR("Rule file '%s': match text cannot be empty.\n", file);
            valid = false;
        }

        guint split_count = g_list_length(match->accountSplits);
        if (split_count == 0) {
            OUT_ERROR("Rule file '%s': match row is missing destination accounts.\n", file);
            valid = false;
            continue;
        }

        int percentage_sum = 0;
        for (GList *split_node = match->accountSplits; split_node != NULL;
             split_node = split_node->next) {
            AccountSplit_t *split_data = (AccountSplit_t *)split_node->data;
            if (split_data == NULL || split_data->accountName == NULL ||
                split_data->accountName->len == 0) {
                OUT_ERROR("Rule file '%s': match row has an empty destination account.\n", file);
                valid = false;
                continue;
            }

            if (!is_valid_account_name(split_data->accountName->str)) {
                OUT_ERROR("Rule file '%s': invalid destination account '%s'.\n", file,
                          split_data->accountName->str);
                valid = false;
            }

            if (split_data->percentage <= 0 || split_data->percentage > 100) {
                OUT_ERROR("Rule file '%s': split percentage for '%s' must be in the range 1-100.\n",
                          file, split_data->accountName->str);
                valid = false;
            }
            percentage_sum += split_data->percentage;
        }

        if (percentage_sum != 100) {
            OUT_ERROR("Rule file '%s': match row split percentages sum to %d, expected 100.\n",
                      file, percentage_sum);
            valid = false;
        }
    }

    for (GList *node = parserSettings->filters; node != NULL; node = node->next) {
        TxFilter_t *current_filter = (TxFilter_t *)node->data;
        if (current_filter == NULL) {
            OUT_ERROR("Rule file '%s': invalid empty filter row.\n", file);
            valid = false;
        }
    }

    return valid;
}

/* Callbacks -----------------------------------------------------------------*/
/*** end of file ***/
