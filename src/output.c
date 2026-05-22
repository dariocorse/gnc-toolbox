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
#include <output.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

/* Private definitions -------------------------------------------------------*/
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

#define BRIGHT_RED "\033[91m"
#define BRIGHT_GREEN "\033[92m"
#define BRIGHT_YELLOW "\033[93m"
#define BRIGHT_BLUE "\033[94m"
#define BRIGHT_MAGENTA "\033[95m"
#define BRIGHT_CYAN "\033[96m"
#define BRIGHT_WHITE "\033[97m"

#define BOLD "\033[1m"
#define UNDERLINE "\033[4m"
#define REVERSED "\033[7m"

#define RED_TEXT_YELLOW_BG "\033[31;43m"
#define WHITE_TEXT_RED_BG "\033[37;41m"

/* Private macros ------------------------------------------------------------*/
/* Private typedefs ----------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
const char *levelColor[] = {
    WHITE_TEXT_RED_BG, //    OUT_LEVEL_FATAL,
    RED,               //    OUT_LEVEL_ERROR,
    YELLOW,            //    OUT_LEVEL_WARNING,
    GREEN,             //    OUT_LEVEL_SUMMARY,
    GREEN,             //    OUT_LEVEL_INFO,
    BRIGHT_WHITE,      //    OUT_LEVEL_VERBOSE,
    WHITE              //    OUT_LEVEL_DEBUG,
};

/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static OutLevel_e outLevel = OUT_LEVEL_WARNING;

/* Private function prototypes -----------------------------------------------*/
static FILE *stream_for_level(OutLevel_e level);
static bool should_use_color(FILE *stream);

/* Public constants ----------------------------------------------------------*/
/* Public functions ----------------------------------------------------------*/
void out_setLevel(const OutLevel_e level)
{
    outLevel = level;
}

bool out(const OutLevel_e level, const char *fmt, ...)
{
    if (level > outLevel) {
        return false;
    }

    va_list args;
    va_start(args, fmt);

    FILE *stream = stream_for_level(level);
    if (should_use_color(stream)) {
        char format[strlen(fmt) + 25];
        strcpy(format, levelColor[level]);
        strcat(format, fmt);
        strcat(format, RESET);
        vfprintf(stream, format, args);
    } else {
        vfprintf(stream, fmt, args);
    }

    va_end(args);

    if (OUT_LEVEL_FATAL == level) {
        exit(EXIT_FAILURE);
    }

    return true;
}

/* Private functions ---------------------------------------------------------*/
static FILE *stream_for_level(OutLevel_e level)
{
    return level <= OUT_LEVEL_WARNING ? stderr : stdout;
}

static bool should_use_color(FILE *stream)
{
    return getenv("NO_COLOR") == NULL && isatty(fileno(stream));
}

/* Callbacks -----------------------------------------------------------------*/
/*** end of file ***/
