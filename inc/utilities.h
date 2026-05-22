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

#ifndef UTILITIES_H
#define UTILITIES_H

#include <glib.h>
#include <data.h>
#include <output.h>
#include <match_params.h>

gboolean gstring_contains(GString *haystack, GString *needle);
gboolean gstring_matches_pattern(GString *str, const gchar *pattern);
gboolean gstring_matches_regex(GString *str, const gchar *pattern);
gboolean startsWith(const char *str, const size_t len, const char *prefix);
char *getCStringFromData();
GString *getGStringFromData(void *s, size_t len);
GDateTime *getDateTimeFromText(const char *text, const char *format);
void printData(const OutLevel_e level, const Data_t *data);

// Input validation functions
gboolean validate_file_exists_and_readable(const char *filepath, GError **error);
gboolean validate_file_writable(const char *filepath, GError **error);
gboolean validate_directory_exists(const char *dirpath, GError **error);
gboolean validate_gnucash_book_ref(const char *book_ref, GError **error);
gboolean is_valid_account_name(const char *account_name);
gboolean is_valid_amount_string(const char *amount_str, const char decimal_sep,
                                const char thousand_sep);
gboolean parse_amount_to_cents(const char *amount_str, char decimal_sep, char thousand_sep,
                               gint *amount_cents);

// GnuCash lock detection for file-backed books
gboolean gnc_is_locked(const gchar *uri);

// Number parsing with locale support
double parse_amount_with_locale(const char *amount_str, char decimal_sep, char thousand_sep);
char *normalize_amount_string(const char *amount_str, char decimal_sep, char thousand_sep);

// Safe memory management functions
void safe_g_free(gpointer *ptr);
void safe_g_string_free(GString **str);
void safe_g_date_time_unref(GDateTime **datetime);
void free_data_t(Data_t *data);
void free_match_entry_t(MatchEntry_t *entry);
void free_parser_settings_t(ParserSetting_t *settings);

#endif // UTILITIES_H
/*** end of file ***/
