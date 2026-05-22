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
#include <csv_helper.h>
#include <output.h>
#include <matcher.h>
#include <stdbool.h>
#include <data.h>
#include <utilities.h>

#include <string.h>
#include <math.h>

#include <glib.h>

/* Private definitions -------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private typedefs ----------------------------------------------------------*/
/* Public constants ----------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static const ParserSetting_t *parserSettings;
static Data_t *data;
static GList *list;

/* Private function prototypes -----------------------------------------------*/
static void processField(int rowIndex, int fieldIndex, void *s, size_t len);
static void processRow(int rowIndex, char terminationChar);

/* Public functions ----------------------------------------------------------*/
GList *m_match(const char *file, const ParserSetting_t *parser)
{
    parserSettings = parser;
    list = NULL;

    csv_load(file, &parserSettings->csvParams, processField, processRow);

    return list;
}

/* Private functions ---------------------------------------------------------*/
static void processField(int rowIndex, int fieldIndex, void *s, size_t len)
{
    do {
        // Check if row should be skipped based on configuration
        if (rowIndex < parserSettings->csvParams.skip_header_rows) {
            csv_printMessage(OUT_LEVEL_DEBUG, "skipping header row");
            break;
        }
        // Initialize row data
        if (0 == fieldIndex) {
            data = g_new0(Data_t, 1);
            data->amountCents = G_MININT;
        }
        // Check if column is defined and validate bounds
        int num_cols = g_list_length(parserSettings->cols);
        if (fieldIndex < 0 || fieldIndex >= num_cols) {
            csv_printMessagef(OUT_LEVEL_WARNING,
                              "CSV has extra column %d, but rule file defines only %d column(s); "
                              "extra value ignored",
                              fieldIndex + 1, num_cols);
            break;
        }

        // Process column based on parser settings with bounds checking
        GList *element = g_list_nth(parserSettings->cols, fieldIndex);
        if (NULL == element) {
            csv_printMessage(OUT_LEVEL_WARNING, "failed to get column element");
            break;
        }
        ColParams_t *c = (ColParams_t *)element->data;
        if (NULL == c) {
            csv_printMessage(OUT_LEVEL_WARNING, "null column parameters");
            break;
        }
        switch (c->type) {
        case COL_TYPE_NONE:
            // Nothing to do
            break;
        case COL_TYPE_DATE: {
            // Validate input length and content
            if (len == 0) {
                csv_printMessagef(OUT_LEVEL_WARNING, "Date field is empty; expected format %s",
                                  c->format);
                break;
            }

            // Create zero-terminated string
            char *temp_str = getCStringFromData(s, len);
            if (!temp_str) {
                csv_printMessage(OUT_LEVEL_ERROR, "Failed to allocate memory for date string");
                break;
            }

            // Validate date format and content
            GDateTime *datetime = getDateTimeFromText(temp_str, c->format);
            if (datetime) {
                data->date = datetime;
            } else {
                csv_printMessagef(OUT_LEVEL_ERROR, "Invalid date; expected format %s", c->format);
            }

            // Cleanup
            g_free(temp_str);
        } break;
        case COL_TYPE_NUM:
            // Validate transaction number field
            if (len > 0) {
                data->number = getGStringFromData(s, len);
                if (!data->number) {
                    csv_printMessage(OUT_LEVEL_ERROR,
                                     "Failed to allocate memory for transaction number");
                }
            } else {
                csv_printMessage(OUT_LEVEL_DEBUG, "Empty transaction number field");
            }
            break;
        case COL_TYPE_DESCR:
            // Validate description field
            if (len > 0) {
                if (NULL == data->description) {
                    // Create new description string
                    data->description = getGStringFromData(s, len);
                    if (!data->description) {
                        csv_printMessage(OUT_LEVEL_ERROR,
                                         "Failed to allocate memory for description");
                    }
                } else {
                    // Append to existing description with separator
                    if (parserSettings->concat && parserSettings->concat->str) {
                        data->description =
                            g_string_append_len(data->description, parserSettings->concat->str,
                                                parserSettings->concat->len);
                    }
                    data->description = g_string_append_len(data->description, s, len);
                }
            } else {
                csv_printMessage(OUT_LEVEL_DEBUG, "Empty description field");
            }
            break;
        case COL_TYPE_AMOUNT: {
            // Initialize amount sum if this is the first AMOUNT column
            if (data->amountCents == G_MININT) {
                data->amountCents = 0;
                data->hasValidAmount = FALSE;
            }

            // Handle empty amount field as zero (no error)
            if (len == 0) {
                csv_printMessage(OUT_LEVEL_DEBUG, "Empty amount field, treating as 0.00");
                csv_printMessagef(OUT_LEVEL_DEBUG, "Running total: %.2f",
                                  data->amountCents / 100.0);
                break;
            }

            // Create zero-terminated string
            char *temp_str = getCStringFromData(s, len);
            if (!temp_str) {
                csv_printMessage(OUT_LEVEL_ERROR, "Failed to allocate memory for amount string");
                break;
            }

            // Check if the string is effectively empty (whitespace only)
            gchar *trimmed = g_strstrip(g_strdup(temp_str));
            if (strlen(trimmed) == 0) {
                csv_printMessage(OUT_LEVEL_DEBUG,
                                 "Empty amount field (whitespace), treating as 0.00");
                csv_printMessagef(OUT_LEVEL_DEBUG, "Running total: %.2f",
                                  data->amountCents / 100.0);
                g_free(trimmed);
                g_free(temp_str);
                break;
            }
            g_free(trimmed);

            // Validate amount string format before parsing
            if (!is_valid_amount_string(temp_str, parserSettings->csvParams.decimal_separator,
                                        parserSettings->csvParams.thousand_separator)) {
                csv_printMessagef(
                    OUT_LEVEL_ERROR,
                    "Invalid amount; expected decimal separator '%c' and thousand separator '%c'",
                    parserSettings->csvParams.decimal_separator,
                    parserSettings->csvParams.thousand_separator);
                g_free(temp_str);
                break;
            }

            gint currentAmountCents = 0;
            gboolean parsed = parse_amount_to_cents(
                temp_str, parserSettings->csvParams.decimal_separator,
                parserSettings->csvParams.thousand_separator, &currentAmountCents);

            if (!parsed) {
                csv_printMessage(OUT_LEVEL_ERROR,
                                 "Invalid amount; value must fit in signed 32-bit cents");
            } else {
                // Apply column-specific multiplier for sign control
                if (c->multiplier != 0) {
                    currentAmountCents *= c->multiplier;
                }

                // Log the individual amount and add to running sum
                csv_printMessagef(OUT_LEVEL_DEBUG, "Found amount: %.2f (multiplier: %d)",
                                  currentAmountCents / 100.0, c->multiplier);
                data->amountCents += currentAmountCents;
                data->hasValidAmount = TRUE; // Mark that we found at least one valid amount

                csv_printMessagef(OUT_LEVEL_DEBUG, "Running total: %.2f",
                                  data->amountCents / 100.0);
            }

            // Cleanup
            g_free(temp_str);
        } break;
        }
    } while (false);
}

static void processRow(int rowIndex, char terminationChar)
{
    (void)rowIndex;
    (void)terminationChar;

    if (NULL != data) {
        // Enhanced validation checks for complete transaction data
        gboolean valid_transaction = TRUE;

        // Validate required fields
        if (NULL == data->date) {
            csv_printMessage(OUT_LEVEL_WARNING,
                             "Skipping row: required DATE column is missing or invalid");
            valid_transaction = FALSE;
        }

        // Check if description exists and is not empty
        if (NULL == data->description || data->description->len == 0) {
            csv_printMessage(OUT_LEVEL_DEBUG, "Row has an empty description");
            // Not critical, but worth noting
        }

        // Validate amount - check for both parsing failure and case where all amounts were empty
        if (data->amountCents == G_MININT) {
            csv_printMessage(OUT_LEVEL_WARNING,
                             "Skipping row: required AMOUNT column is missing or invalid");
            valid_transaction = FALSE;
        } else if (!data->hasValidAmount) {
            csv_printMessage(OUT_LEVEL_WARNING, "Skipping row: all AMOUNT columns are empty");
            valid_transaction = FALSE;
        }

        // Validate transaction number if present
        if (data->number && data->number->len > 100) { // Unreasonably long transaction number
            csv_printMessage(OUT_LEVEL_WARNING,
                             "Skipping row: transaction number exceeds 100 characters");
            valid_transaction = FALSE;
        }

        if (valid_transaction) {
            // Add validated data to list
            list = g_list_append(list, data);

            if (NULL == list) {
                csv_printMessage(OUT_LEVEL_FATAL, "Failed to create transaction list");
            }
        } else {
            // Free invalid data to prevent memory leaks
            csv_printMessage(OUT_LEVEL_INFO,
                             "Transaction row was not imported because of the errors above");
            free_data_t(data);
        }

        // Reset data pointer for next row
        data = NULL;
    }
}

/* Callbacks -----------------------------------------------------------------*/
/*** end of file ***/
