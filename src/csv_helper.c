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

#include <csv.h>

#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <glib.h>

/* Private definitions -------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private typedefs ----------------------------------------------------------*/
typedef struct {
    int fields;
    int rows;
} counts_t;

typedef enum { CSV_PARSING_NONE, CSV_PARSING_FIELD, CSV_PARSING_ROW } CSVParsing_e;

/* Private constants ---------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static counts_t counts;
static csv_processField_fp processFieldCB;
static csv_processRow_fp processROWCB;
static void *fieldData;
static size_t fieldLen;
static CSVParsing_e parsingState;
static const CSVParseParam_t *current_csv_params;
static const char *current_file;

/* Private function prototypes -----------------------------------------------*/
static void fieldRead(void *s, size_t len, void *data);
static void rowCompleted(int c, void *data);

/* Public constants ----------------------------------------------------------*/
/* Public functions ----------------------------------------------------------*/
void csv_load(const char *file, const CSVParseParam_t *csv_params, csv_processField_fp processField,
              csv_processRow_fp processRow)
{
    struct csv_parser p;
    unsigned char options = 0;

    if (csv_init(&p, options) != 0) {
        OUT_FATAL("Failed to initialize csv parser\n");
    }

    // Use configurable CSV parameters
    current_csv_params = csv_params;
    csv_set_delim(&p, csv_params->delimiter);
    csv_set_quote(&p, csv_params->quote);

    FILE *fp = fopen(file, "rb");
    if (!fp) {
        OUT_FATAL("Failed to open %s: %s\n", file, strerror(errno));
    }

    counts.fields = 0;
    counts.rows = 0;
    processFieldCB = processField;
    processROWCB = processRow;
    parsingState = CSV_PARSING_NONE;
    current_file = file;

    char buf[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buf, 1, 1024, fp)) > 0) {
        if (csv_parse(&p, buf, bytes_read, fieldRead, rowCompleted, NULL) != bytes_read) {
            OUT_ERROR("Error while parsing file: %s\n", csv_strerror(csv_error(&p)));
        }
    }

    csv_fini(&p, fieldRead, rowCompleted, NULL);

    if (ferror(fp)) {
        fclose(fp);
        OUT_FATAL("Error while reading file %s\n", file);
    }

    fclose(fp);

    csv_free(&p);
    current_file = NULL;
}

void csv_printMessage(OutLevel_e level, const char *msg)
{
    const char *file = current_file != NULL ? current_file : "(no file)";

    switch (parsingState) {
    case CSV_PARSING_NONE:
        out(level, "CSV: %s\n", msg);
        break;
    case CSV_PARSING_FIELD:
        out(level, "CSV '%s' row %d, field %d, value '%.*s': %s\n", file, counts.rows + 1,
            counts.fields + 1, (int)fieldLen, fieldData, msg);
        break;
    case CSV_PARSING_ROW:
        out(level, "CSV '%s' row %d: %s\n", file, counts.rows + 1, msg);
        break;
    }
}

void csv_printMessagef(OutLevel_e level, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    gchar *msg = g_strdup_vprintf(format, args);
    va_end(args);

    if (msg == NULL) {
        csv_printMessage(level, "Failed to format CSV message");
        return;
    }

    csv_printMessage(level, msg);
    g_free(msg);
}

/* Private functions ---------------------------------------------------------*/
static void fieldRead(void *s, size_t len, void *d)
{
    (void)d;

    fieldData = s;
    fieldLen = len;
    parsingState = CSV_PARSING_FIELD;
    if (processFieldCB) {
        processFieldCB(counts.rows, counts.fields, s, len);
    }
    parsingState = CSV_PARSING_NONE;
    counts.fields++;
}

static void rowCompleted(int c, void *d)
{
    (void)d;

    parsingState = CSV_PARSING_ROW;
    if (processROWCB) {
        processROWCB(counts.rows, c);
    }
    parsingState = CSV_PARSING_NONE;
    counts.fields = 0;
    counts.rows++;
}

/* Callbacks -----------------------------------------------------------------*/
/*** end of file ***/
