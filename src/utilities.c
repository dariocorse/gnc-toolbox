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

#include <utilities.h>
#include <output.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>

static gboolean book_ref_is_non_file_uri(const char *book_ref)
{
    const gchar *scheme = g_uri_peek_scheme(book_ref);
    return scheme != NULL && g_ascii_strcasecmp(scheme, "file") != 0;
}

static gchar *book_ref_to_local_path(const char *book_ref, GError **error)
{
    if (book_ref == NULL) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "Book reference cannot be NULL");
        return NULL;
    }

    if (g_str_has_prefix(book_ref, "file://")) {
        GError *uri_error = NULL;
        gchar *path = g_filename_from_uri(book_ref, NULL, &uri_error);
        if (path != NULL) {
            return path;
        }

        // Backward-compatible fallback for legacy pseudo-URIs like file://../book.gnucash
        const gchar *legacy_path = book_ref + strlen("file://");
        if (legacy_path[0] == '/' || legacy_path[0] == '.') {
            if (uri_error != NULL) {
                g_error_free(uri_error);
            }
            return g_strdup(legacy_path);
        }

        if (uri_error != NULL) {
            g_propagate_error(error, uri_error);
        } else {
            g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "Invalid file URI '%s'", book_ref);
        }
        return NULL;
    }

    if (book_ref_is_non_file_uri(book_ref)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                    "Book reference '%s' is not file-backed", book_ref);
        return NULL;
    }

    return g_strdup(book_ref);
}

// gboolean gstring_matches_pattern(GString *str, const gchar *pattern) {
//     if (!str || !pattern) return FALSE;
//     return g_pattern_match_simple(pattern, str->str);
// }

gboolean gstring_matches_pattern(GString *str, const gchar *pattern)
{
    if (str == NULL || pattern == NULL) {
        return FALSE;
    }

    // Use GLib's pattern matching for shell-style wildcards (* and ?)
    GPatternSpec *spec = g_pattern_spec_new(pattern);
    gboolean match = g_pattern_spec_match_string(spec, str->str);

    // Free the memory allocated for the pattern spec
    g_pattern_spec_free(spec);

    return match;
}

gboolean gstring_matches_regex(GString *str, const gchar *pattern)
{
    if (str == NULL || pattern == NULL) {
        return FALSE;
    }

    GError *error = NULL;
    GRegex *regex = g_regex_new(pattern, 0, 0, &error);
    if (regex == NULL) {
        if (error != NULL) {
            g_error_free(error);
        }
        return FALSE;
    }

    gboolean match = g_regex_match(regex, str->str, 0, NULL);
    g_regex_unref(regex);
    return match;
}

gboolean gstring_contains(GString *haystack, GString *needle)
{
    if (haystack == NULL || needle == NULL) {
        return FALSE;
    }

    // Convert GString* to const gchar* with length information
    const gchar *haystack_str = haystack->str;
    const gchar *needle_str = needle->str;

    // Use g_strstr_len to search for needle in haystack
    return g_strstr_len(haystack_str, haystack->len, needle_str) != NULL;
}

gboolean startsWith(const char *str, const size_t len, const char *prefix)
{
    if (str == NULL || prefix == NULL) {
        return FALSE; // Or g_return_val_if_fail(prefix != NULL, FALSE);
    }

    size_t prefixLen = strlen(prefix);
    if (len < prefixLen) {
        return FALSE;
    }
    return strncmp(str, prefix, prefixLen) == 0;
}

GString *getGStringFromData(void *s, size_t len)
{
    GString *str = g_string_sized_new(len);

    if (s != NULL && len > 0) {
        g_string_append_len(str, s, len);
    }

    return str;
}

char *getCStringFromData(void *s, size_t len)
{
    if (s == NULL || len == 0) {
        // Return a heap-allocated empty string
        char *empty = g_new0(char, 1); // zero-filled
        return empty;
    }

    char *temp_str = g_new(char, len + 1);
    memcpy(temp_str, s, len);
    temp_str[len] = '\0'; // Ensure the string is zero-terminated
    return temp_str;
}

GDateTime *getDateTimeFromText(const char *text, const char *format)
{
    if (!text || !format)
        return NULL;

    int day = -1, month = -1, year = -1;

    // Detect separator from format (non-alphanumeric char)
    char sep = 0;
    for (const char *p = format; *p; ++p) {
        if (!g_ascii_isalnum(*p)) {
            sep = *p;
            break;
        }
    }
    if (!sep)
        sep = '\0'; // e.g. "YYYYMMDD"

    // Split both format and text
    gchar **fmt_parts = g_strsplit(format, sep ? (gchar[]){sep, 0} : "", -1);
    gchar **txt_parts = sep ? g_strsplit(text, (gchar[]){sep, 0}, -1)
                            : g_strsplit_set(text, "", 3); // fallback: take substrings

    gsize fmt_count = g_strv_length(fmt_parts);
    gsize txt_count = g_strv_length(txt_parts);

    if (fmt_count != txt_count) {
        g_strfreev(fmt_parts);
        g_strfreev(txt_parts);
        return NULL;
    }

    for (gsize i = 0; i < fmt_count; ++i) {
        if (g_strcmp0(fmt_parts[i], "DD") == 0) {
            day = atoi(txt_parts[i]);
        } else if (g_strcmp0(fmt_parts[i], "MM") == 0) {
            month = atoi(txt_parts[i]);
        } else if (g_strcmp0(fmt_parts[i], "YYYY") == 0) {
            year = atoi(txt_parts[i]);
        } else if (g_strcmp0(fmt_parts[i], "YY") == 0) {
            year = atoi(txt_parts[i]);
            year += (year >= 70) ? 1900 : 2000; // Y2K-style pivot
        } else {
            // Unknown token — treat as error
            g_strfreev(fmt_parts);
            g_strfreev(txt_parts);
            return NULL;
        }
    }

    g_strfreev(fmt_parts);
    g_strfreev(txt_parts);

    // Validate
    if (year < 1900 || year > 2100 || month < 1 || month > 12 || day < 1 || day > 31) {
        OUT_WARNING("Invalid date components: %04d-%02d-%02d from text '%s'\n", year, month, day,
                    text);
        return NULL;
    }

    return g_date_time_new_local(year, month, day, 0, 0, 0);
}

void printData(const OutLevel_e level, const Data_t *data)
{
    if (data == NULL) {
        out(level, "(null data)\n");
        return;
    }

    gchar *datetime_str = (data->date != NULL) ? g_date_time_format(data->date, "%Y-%m-%d %H:%M:%S")
                                               : g_strdup("(no-date)");
    const char *number = (data->number != NULL) ? data->number->str : "";
    const char *description =
        (data->description != NULL) ? data->description->str : "(no description)";

    out(level, "%s %10s %s %8.2f\n", datetime_str, number, description, data->amountCents / 100.0);

    // Release allocated memory
    g_free(datetime_str);
}

// Input validation functions
gboolean validate_file_exists_and_readable(const char *filepath, GError **error)
{
    if (filepath == NULL) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "File path cannot be NULL");
        return FALSE;
    }

    // Check if file exists
    if (!g_file_test(filepath, G_FILE_TEST_EXISTS)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOENT, "File '%s' does not exist", filepath);
        return FALSE;
    }

    // Check if it's a regular file
    if (!g_file_test(filepath, G_FILE_TEST_IS_REGULAR)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "'%s' is not a regular file",
                    filepath);
        return FALSE;
    }

    // Check if file is readable
    if (access(filepath, R_OK) != 0) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                    "File '%s' is not readable: %s", filepath, g_strerror(errno));
        return FALSE;
    }

    return TRUE;
}

gboolean validate_file_writable(const char *filepath, GError **error)
{
    if (filepath == NULL) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "File path cannot be NULL");
        return FALSE;
    }

    // If file doesn't exist, check if parent directory is writable
    if (!g_file_test(filepath, G_FILE_TEST_EXISTS)) {
        gchar *dirname = g_path_get_dirname(filepath);
        gboolean result = validate_directory_exists(dirname, error);
        if (result) {
            // Check if directory is writable
            if (access(dirname, W_OK) != 0) {
                g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                            "Directory '%s' is not writable: %s", dirname, g_strerror(errno));
                result = FALSE;
            }
        }
        g_free(dirname);
        return result;
    }

    // File exists, check if it's writable
    if (access(filepath, W_OK) != 0) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                    "File '%s' is not writable: %s", filepath, g_strerror(errno));
        return FALSE;
    }

    return TRUE;
}

gboolean validate_directory_exists(const char *dirpath, GError **error)
{
    if (dirpath == NULL) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "Directory path cannot be NULL");
        return FALSE;
    }

    if (!g_file_test(dirpath, G_FILE_TEST_EXISTS)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOENT, "Directory '%s' does not exist",
                    dirpath);
        return FALSE;
    }

    if (!g_file_test(dirpath, G_FILE_TEST_IS_DIR)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOTDIR, "'%s' is not a directory", dirpath);
        return FALSE;
    }

    return TRUE;
}

gboolean validate_gnucash_book_ref(const char *book_ref, GError **error)
{
    if (book_ref == NULL) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "Book reference cannot be NULL");
        return FALSE;
    }

    // Non-file URIs are passed through to GnuCash; backend availability is validated on open.
    if (book_ref_is_non_file_uri(book_ref)) {
        return TRUE;
    }

    GError *local_error = NULL;
    gchar *local_path = book_ref_to_local_path(book_ref, &local_error);
    if (local_path == NULL) {
        g_propagate_error(error, local_error);
        return FALSE;
    }

    if (!validate_file_exists_and_readable(local_path, error)) {
        g_free(local_path);
        return FALSE;
    }

    // Check for common file-backed GnuCash book extensions
    if (g_str_has_suffix(local_path, ".gnucash") || g_str_has_suffix(local_path, ".xac") ||
        g_str_has_suffix(local_path, ".xml")) {
        g_free(local_path);
        return TRUE;
    }

    g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                "File-backed book '%s' does not have a recognized GnuCash file extension "
                "(.gnucash, .xac, .xml)",
                local_path);
    g_free(local_path);
    return FALSE;
}

gboolean is_valid_account_name(const char *account_name)
{
    if (account_name == NULL || strlen(account_name) == 0) {
        return FALSE;
    }

    // Check for reasonable length (GnuCash typically allows up to 2048 chars)
    if (strlen(account_name) > 2048) {
        return FALSE;
    }

    // Check for invalid characters that might cause issues
    // GnuCash generally allows most characters, but we'll be conservative
    const char *invalid_chars = "<>|*?\"";
    if (strpbrk(account_name, invalid_chars) != NULL) {
        return FALSE;
    }

    return TRUE;
}

gboolean is_valid_amount_string(const char *amount_str, const char decimal_sep,
                                const char thousand_sep)
{
    if (amount_str == NULL || strlen(amount_str) == 0) {
        return FALSE;
    }

    const char *p = amount_str;
    gboolean has_digit = FALSE;
    gboolean has_decimal = FALSE;
    int decimal_places = 0;

    // Skip leading whitespace
    while (isspace(*p))
        p++;

    // Optional sign
    if (*p == '+' || *p == '-')
        p++;

    // Check digits and decimal point
    while (*p) {
        if (isdigit(*p)) {
            has_digit = TRUE;
            if (has_decimal)
                decimal_places++;
        } else if (*p == decimal_sep) {
            if (has_decimal)
                return FALSE; // Multiple decimal points
            has_decimal = TRUE;
        } else if (*p == thousand_sep || *p == ' ' || *p == '\t') {
            // Skip thousand separators and whitespace
            // But ensure we don't have digits after whitespace and before decimal
        } else {
            return FALSE; // Invalid character
        }
        p++;
    }

    // Must have at least one digit and reasonable decimal places
    return has_digit && decimal_places <= 4;
}

gboolean parse_amount_to_cents(const char *amount_str, char decimal_sep, char thousand_sep,
                               gint *amount_cents)
{
    if (amount_str == NULL || amount_cents == NULL) {
        return FALSE;
    }

    const unsigned char *p = (const unsigned char *)amount_str;
    while (g_ascii_isspace(*p)) {
        p++;
    }

    int sign = 1;
    if (*p == '-' || *p == '+') {
        sign = (*p == '-') ? -1 : 1;
        p++;
    }

    gboolean has_digit = FALSE;
    gboolean has_decimal = FALSE;
    gint64 whole_units = 0;
    int fractional_digits[4] = {0, 0, 0, 0};
    int fractional_count = 0;

    while (*p != '\0') {
        if (g_ascii_isdigit(*p)) {
            has_digit = TRUE;
            if (has_decimal) {
                if (fractional_count >= 4) {
                    return FALSE;
                }
                fractional_digits[fractional_count++] = *p - '0';
            } else {
                whole_units = whole_units * 10 + (*p - '0');
                if (whole_units > ((gint64)G_MAXINT / 100) + 1) {
                    return FALSE;
                }
            }
        } else if (*p == (unsigned char)decimal_sep) {
            if (has_decimal) {
                return FALSE;
            }
            has_decimal = TRUE;
        } else if (!has_decimal && *p == (unsigned char)thousand_sep) {
            if (!has_digit || !g_ascii_isdigit(*(p + 1))) {
                return FALSE;
            }
        } else if (g_ascii_isspace(*p)) {
            while (g_ascii_isspace(*p)) {
                p++;
            }
            if (*p != '\0') {
                return FALSE;
            }
            break;
        } else {
            return FALSE;
        }

        p++;
    }

    if (!has_digit) {
        return FALSE;
    }

    int fractional_cents = 0;
    if (fractional_count == 1) {
        fractional_cents = fractional_digits[0] * 10;
    } else if (fractional_count >= 2) {
        fractional_cents = fractional_digits[0] * 10 + fractional_digits[1];
    }

    if (fractional_count > 2 && fractional_digits[2] >= 5) {
        fractional_cents++;
    }

    gint64 cents = whole_units * 100 + fractional_cents;
    if (cents > G_MAXINT) {
        return FALSE;
    }

    cents *= sign;
    if (cents == G_MININT || cents < G_MININT || cents > G_MAXINT) {
        return FALSE;
    }

    *amount_cents = (gint)cents;
    return TRUE;
}

// Number parsing with locale support
double parse_amount_with_locale(const char *amount_str, char decimal_sep, char thousand_sep)
{
    if (amount_str == NULL) {
        return 0.0;
    }

    char *normalized = normalize_amount_string(amount_str, decimal_sep, thousand_sep);
    if (normalized == NULL) {
        return 0.0;
    }

    double result = strtod(normalized, NULL);
    free(normalized);
    return result;
}

char *normalize_amount_string(const char *amount_str, char decimal_sep, char thousand_sep)
{
    if (amount_str == NULL) {
        return NULL;
    }

    size_t len = strlen(amount_str);
    char *result = malloc(len + 1);
    if (result == NULL) {
        return NULL;
    }

    const char *read_ptr = amount_str;
    char *write_ptr = result;
    gboolean found_decimal = FALSE;
    gboolean is_negative = FALSE;

    // Skip leading whitespace
    while (isspace(*read_ptr))
        read_ptr++;

    // Handle negative sign
    if (*read_ptr == '-') {
        is_negative = TRUE;
        *write_ptr++ = *read_ptr++;
    } else if (*read_ptr == '+') {
        read_ptr++; // Skip positive sign
    }

    // Process the number
    while (*read_ptr) {
        if (*read_ptr == decimal_sep) {
            if (!found_decimal) {
                // This is the decimal separator
                *write_ptr++ = '.'; // Always normalize to '.'
                found_decimal = TRUE;
            } else {
                break; // End of number
            }
        } else if (*read_ptr == thousand_sep && !found_decimal) {
            // This is a thousand separator before decimal point - skip it
            // But validate it's in the right position (not at start/end, surrounded by digits)
            if (read_ptr > amount_str && isdigit(*(read_ptr - 1)) && *(read_ptr + 1) != '\0' &&
                isdigit(*(read_ptr + 1))) {
                // Valid thousand separator - skip it
            } else {
                // Invalid position for thousand separator
                free(result);
                return NULL;
            }
        } else if (isdigit(*read_ptr)) {
            *write_ptr++ = *read_ptr;
        } else if (isspace(*read_ptr)) {
            break; // End of number
        }
        read_ptr++;
    }

    *write_ptr++ = '\0';

    // Validate the result
    if (write_ptr == result || (write_ptr == result + 1 && is_negative)) {
        // Empty result or just a sign
        free(result);
        return NULL;
    }

    return result;
}

// Safe memory management functions
void safe_g_free(gpointer *ptr)
{
    if (ptr && *ptr) {
        g_free(*ptr);
        *ptr = NULL;
    }
}

void safe_g_string_free(GString **str)
{
    if (str && *str) {
        g_string_free(*str, TRUE);
        *str = NULL;
    }
}

void safe_g_date_time_unref(GDateTime **datetime)
{
    if (datetime && *datetime) {
        g_date_time_unref(*datetime);
        *datetime = NULL;
    }
}

void free_data_t(Data_t *data)
{
    if (data == NULL) {
        return;
    }

    safe_g_date_time_unref(&data->date);
    safe_g_string_free(&data->number);
    safe_g_string_free(&data->description);

    g_free(data);
}

void free_match_entry_t(MatchEntry_t *entry)
{
    if (entry == NULL) {
        return;
    }

    // Free the match data based on type
    switch (entry->type) {
    case MATCH_TYPE_CONTAINS:
    case MATCH_TYPE_PATTERN:
    case MATCH_TYPE_REGEX:
        safe_g_string_free(&entry->data.gstring);
        break;
    case MATCH_TYPE_DATE_LESS_OR_EQUALS:
        safe_g_date_time_unref(&entry->data.gdatetime);
        break;
    }

    // Free account splits list
    if (entry->accountSplits) {
        for (GList *node = entry->accountSplits; node != NULL; node = node->next) {
            AccountSplit_t *split = (AccountSplit_t *)node->data;
            if (split) {
                safe_g_string_free(&split->accountName);
                g_free(split);
            }
        }
        g_list_free(entry->accountSplits);
    }

    g_free(entry);
}

void free_parser_settings_t(ParserSetting_t *settings)
{
    if (settings == NULL) {
        return;
    }

    // Free column parameters
    if (settings->cols) {
        for (GList *node = settings->cols; node != NULL; node = node->next) {
            ColParams_t *col = (ColParams_t *)node->data;
            g_free(col);
        }
        g_list_free(settings->cols);
    }

    // Free match entries
    if (settings->matches) {
        for (GList *node = settings->matches; node != NULL; node = node->next) {
            MatchEntry_t *entry = (MatchEntry_t *)node->data;
            free_match_entry_t(entry);
        }
        g_list_free(settings->matches);
    }

    // Free filters
    if (settings->filters) {
        for (GList *node = settings->filters; node != NULL; node = node->next) {
            TxFilter_t *filter = (TxFilter_t *)node->data;
            if (filter) {
                switch (filter->type) {
                case MATCH_TYPE_CONTAINS:
                case MATCH_TYPE_PATTERN:
                case MATCH_TYPE_REGEX:
                    safe_g_string_free(&filter->value.gstring);
                    break;
                case MATCH_TYPE_DATE_LESS_OR_EQUALS:
                    safe_g_date_time_unref(&filter->value.gdatetime);
                    break;
                }
                g_free(filter);
            }
        }
        g_list_free(settings->filters);
    }

    safe_g_string_free(&settings->baseAccount);
    safe_g_string_free(&settings->unmatchedAccount);
    safe_g_string_free(&settings->concat);

    g_free(settings);
}

gboolean gnc_is_locked(const gchar *uri)
{
    if (!uri) {
        return FALSE;
    }

    // Lock files only apply to local, file-backed books.
    if (book_ref_is_non_file_uri(uri)) {
        return FALSE;
    }

    gchar *path = book_ref_to_local_path(uri, NULL);
    if (path == NULL) {
        return FALSE;
    }

    // Check for GnuCash lock files (.LCK and .LNK)
    gchar *lck = g_strdup_printf("%s.LCK", path);
    gchar *lnk = g_strdup_printf("%s.LNK", path);

    gboolean locked = g_file_test(lck, G_FILE_TEST_EXISTS) || g_file_test(lnk, G_FILE_TEST_EXISTS);

    g_free(lck);
    g_free(lnk);
    g_free(path);
    return locked;
}
