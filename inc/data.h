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
#ifndef DATA_H_
#define DATA_H_

/* Includes ------------------------------------------------------------------*/
#include <glib.h>
#include <glib/gdatetime.h>

/* Definitions ---------------------------------------------------------------*/
/* Macros --------------------------------------------------------------------*/
/* Typedefs ------------------------------------------------------------------*/
typedef struct {
    GDateTime *date;
    GString *number;
    GString *description;
    gint amountCents;        // G_MININT = invalid, sum of all AMOUNT columns
    gboolean hasValidAmount; // Track if any non-empty amount was found
} Data_t;

/* Constants -----------------------------------------------------------------*/
/* Variables -----------------------------------------------------------------*/
/* Function prototypes -------------------------------------------------------*/
/* Callback prototypes -------------------------------------------------------*/
#endif /* DATA_H_ */
/*** end of file ***/
