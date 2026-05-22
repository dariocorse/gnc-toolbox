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
#ifndef OUTPUT_H_
#define OUTPUT_H_

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>

/* Definitions ---------------------------------------------------------------*/
/* Macros --------------------------------------------------------------------*/
#define OUT_FATAL(...) out(OUT_LEVEL_FATAL, __VA_ARGS__)
#define OUT_ERROR(...) out(OUT_LEVEL_ERROR, __VA_ARGS__)
#define OUT_WARNING(...) out(OUT_LEVEL_WARNING, __VA_ARGS__)
#define OUT_SUMMARY(...) out(OUT_LEVEL_SUMMARY, __VA_ARGS__)
#define OUT_INFO(...) out(OUT_LEVEL_INFO, __VA_ARGS__)
#define OUT_VERBOSE(...) out(OUT_LEVEL_VERBOSE, __VA_ARGS__)
#define OUT_DEBUG(...) out(OUT_LEVEL_DEBUG, __VA_ARGS__)

/* Typedefs ------------------------------------------------------------------*/
typedef enum {
    OUT_LEVEL_FATAL = 0,
    OUT_LEVEL_ERROR,
    OUT_LEVEL_WARNING,
    OUT_LEVEL_SUMMARY,
    OUT_LEVEL_INFO,
    OUT_LEVEL_VERBOSE,
    OUT_LEVEL_DEBUG,
} OutLevel_e;

/* Constants -----------------------------------------------------------------*/
/* Variables -----------------------------------------------------------------*/
/* Function prototypes -------------------------------------------------------*/
void out_setLevel(const OutLevel_e level);
bool out(const OutLevel_e level, const char *fmt, ...);

/* Callback prototypes -------------------------------------------------------*/
#endif /* OUTPUT_H_ */
/*** end of file ***/
