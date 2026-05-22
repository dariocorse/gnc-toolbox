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
#ifndef HEADERMACRO_H_
#define HEADERMACRO_H_

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <output.h>
#include <match_params.h>

/* Definitions ---------------------------------------------------------------*/
/* Macros --------------------------------------------------------------------*/
/* Typedefs ------------------------------------------------------------------*/
typedef void (*csv_processField_fp)(int rowIndex, int fieldIndex, void *s, size_t len);
typedef void (*csv_processRow_fp)(int rowIndex, char terminationChar);

/* Constants -----------------------------------------------------------------*/
/* Variables -----------------------------------------------------------------*/
/* Function prototypes -------------------------------------------------------*/
void csv_load(const char *file, const CSVParseParam_t *csv_params, csv_processField_fp processField,
              csv_processRow_fp processRow);

void csv_printMessage(OutLevel_e level, const char *msg);
void csv_printMessagef(OutLevel_e level, const char *format, ...);
/* Callback prototypes -------------------------------------------------------*/
#endif /* HEADERMACRO_H_ */
/*** end of file ***/
