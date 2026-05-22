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
#ifndef CLI_H_
#define CLI_H_

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>

/* Definitions ---------------------------------------------------------------*/
/* Macros --------------------------------------------------------------------*/
/* Typedefs ------------------------------------------------------------------*/

/**
 * @brief Available operations/subcommands
 */
typedef enum {
    CLI_OPERATION_NONE = 0,
    CLI_OPERATION_IMPORT,
    CLI_OPERATION_MERGE,
    CLI_OPERATION_HELP,
    CLI_OPERATION_VERSION
} CliOperation_e;

/**
 * @brief Verbosity levels
 */
typedef enum {
    CLI_VERBOSITY_QUIET = 0,
    CLI_VERBOSITY_NORMAL,
    CLI_VERBOSITY_VERBOSE,
    CLI_VERBOSITY_DEBUG
} CliVerbosity_e;

/**
 * @brief Structure containing all parsed command line arguments
 */
typedef struct {
    // Operation to perform
    CliOperation_e operation;

    // Common options
    char *book_ref;           // GnuCash book reference (path or backend URI)
    char *config_file;        // Configuration file path
    bool dry_run;             // Preview mode without changes
    bool no_duplicates;       // Skip duplicate transactions
    CliVerbosity_e verbosity; // Output verbosity level

    // Import-specific options
    char *csv_file; // CSV file to import (positional for import)

    // Merge-specific options
    char *source_book_ref; // Source book reference for merge
    char *source_account;  // Account name to merge from

    // Internal flags
    bool help_requested;    // Help was requested
    bool version_requested; // Version was requested
} CliArgs_t;

/* Constants -----------------------------------------------------------------*/
/* Variables -----------------------------------------------------------------*/
/* Function prototypes -------------------------------------------------------*/

/**
 * @brief Parse command line arguments using modern subcommand approach
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @param args Pointer to structure to fill with parsed arguments
 * @return true if parsing successful, false on error
 */
bool cli_parse_args(int argc, char *argv[], CliArgs_t *args);

/**
 * @brief Print help message for the specified operation
 * @param operation The operation to show help for (or CLI_OPERATION_HELP for general help)
 */
void cli_print_help(CliOperation_e operation);

/**
 * @brief Print version information
 */
void cli_print_version(void);

/**
 * @brief Print usage examples
 */
void cli_print_examples(void);

/**
 * @brief Validate parsed arguments for consistency
 * @param args Pointer to parsed arguments structure
 * @return true if arguments are valid, false otherwise
 */
bool cli_validate_args(const CliArgs_t *args);

/**
 * @brief Initialize CLI arguments structure with default values
 * @param args Pointer to structure to initialize
 */
void cli_init_args(CliArgs_t *args);

/**
 * @brief Free any dynamically allocated memory in CLI arguments
 * @param args Pointer to structure to cleanup
 */
void cli_cleanup_args(CliArgs_t *args);

/**
 * @brief Configure application output filtering for a parsed verbosity level
 * @param verbosity Parsed CLI verbosity level
 */
void cli_configure_output_level(CliVerbosity_e verbosity);

/* Callback prototypes -------------------------------------------------------*/
#endif /* CLI_H_ */
/*** end of file ***/
