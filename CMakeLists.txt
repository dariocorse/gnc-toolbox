cmake_minimum_required(VERSION 3.16)

project(gnc-toolbox
    VERSION 1.0.0
    DESCRIPTION "GnuCash Toolbox - CSV Import, Merge and Analysis Tools"
    LANGUAGES C)

include(GNUInstallDirs)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build type handling
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find dependencies
find_package(PkgConfig REQUIRED)

# Find GLib
pkg_search_module(GLIB REQUIRED glib-2.0)
if(NOT GLIB_FOUND)
    message(FATAL_ERROR "GLib 2.0 not found")
endif()

# Find GnuCash (more robust detection)
find_library(GNUCASH_ENGINE_LIB
    NAMES gnc-engine
    PATHS /usr/lib /usr/local/lib
    PATH_SUFFIXES gnucash x86_64-linux-gnu/gnucash aarch64-linux-gnu/gnucash
    REQUIRED)
get_filename_component(GNUCASH_ENGINE_LIB_DIR "${GNUCASH_ENGINE_LIB}" DIRECTORY)

find_library(CSV_LIB csv REQUIRED)

# Enable testing
enable_testing()

# Create main executable
add_executable(gnc-toolbox
    main.c
    src/cli.c
    src/match_params_loader.c
    src/matcher.c
    src/utilities.c
    src/gnc_utilities.c
    src/merge.c
    src/output.c
    src/csv_helper.c
)

# Create test executable
add_executable(test_gnc-toolbox EXCLUDE_FROM_ALL
    tests/test_main.c
    tests/test_utilities.c
    tests/test_cli.c
    tests/test_csv_helper.c
    tests/test_date_parsing.c
    tests/test_number_parsing.c
    tests/test_csv_params.c
    tests/test_gnc_utilities.c
    tests/test_input_validation.c
    tests/test_integration.c
    tests/test_book_integration.c
    src/cli.c
    src/match_params_loader.c
    src/matcher.c
    src/utilities.c
    src/gnc_utilities.c
    src/merge.c
    src/output.c
    src/csv_helper.c
)

# Set properties for main executable
set_target_properties(gnc-toolbox PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    INSTALL_RPATH "${GNUCASH_ENGINE_LIB_DIR}"
)

# Set properties for test executable
set_target_properties(test_gnc-toolbox PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

# Include directories for main executable
target_include_directories(gnc-toolbox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${GLIB_INCLUDE_DIRS}
)

# Include directories for test executable
target_include_directories(test_gnc-toolbox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${GLIB_INCLUDE_DIRS}
)

# Compile definitions for main executable
target_compile_definitions(gnc-toolbox PRIVATE
    ${GLIB_CFLAGS_OTHER}
    PROJECT_VERSION="${PROJECT_VERSION}"
)

# Compile definitions for test executable
target_compile_definitions(test_gnc-toolbox PRIVATE
    ${GLIB_CFLAGS_OTHER}
    PROJECT_VERSION="${PROJECT_VERSION}"
)

# Link libraries for main executable
target_link_libraries(gnc-toolbox PRIVATE
    ${GLIB_LIBRARIES}
    ${GNUCASH_ENGINE_LIB}
    ${CSV_LIB}
)

# Link libraries for test executable
target_link_libraries(test_gnc-toolbox PRIVATE
    ${GLIB_LIBRARIES}
    ${GNUCASH_ENGINE_LIB}
    ${CSV_LIB}
    m  # Link math library for isfinite, etc.
)

# Add tests
set(BOOK_INTEGRATION_TESTS
    /book_integration/create_save_reload
    /book_integration/import_pipeline_roundtrip
    /book_integration/import_unmatched_transaction_uses_review_account
    /book_integration/import_rejects_missing_match
    /book_integration/import_missing_destination_account_creates_no_partial_tx
    /book_integration/merge_pipeline_roundtrip
    /book_integration/accepts_qualified_match_account_paths
)

add_test(NAME unit_tests COMMAND test_gnc-toolbox -x /book_integration)

foreach(TEST_PATH IN LISTS BOOK_INTEGRATION_TESTS)
    string(REPLACE "/book_integration/" "book_integration." TEST_NAME "${TEST_PATH}")
    add_test(NAME ${TEST_NAME} COMMAND test_gnc-toolbox -p ${TEST_PATH})
    set_tests_properties(${TEST_NAME} PROPERTIES
        TIMEOUT 60
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endforeach()

# Set test environment
set_tests_properties(unit_tests PROPERTIES
    TIMEOUT 60
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Install targets
install(TARGETS gnc-toolbox
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
    LICENSE
    README.md
    CHANGELOG.md
    BUILDING.md
    PACKAGING.md
    USAGE.md
    TESTING.md
    CONTRIBUTING.md
    SECURITY.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

install(DIRECTORY examples/
    DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
    FILES_MATCHING
        PATTERN "*.md"
        PATTERN "*.csv"
)

install(FILES man/gnc-toolbox.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
)

# Uninstall target
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    IMMEDIATE @ONLY
)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

# Package configuration
string(TOLOWER "${CMAKE_SYSTEM_NAME}" CPACK_SYSTEM_NAME_LOWER)
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" CPACK_SYSTEM_PROCESSOR_LOWER)

set(CPACK_PACKAGE_NAME "gnc-toolbox")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "Dario Corsetti")
set(CPACK_PACKAGE_CONTACT "Dario Corsetti <dev@dariocorsetti.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/dariocorsetti/gnc-toolbox")
set(CPACK_PACKAGE_LICENSE "AGPL-3.0-or-later")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_FILE_NAME
    "${PROJECT_NAME}-${PROJECT_VERSION}-${CPACK_SYSTEM_NAME_LOWER}-${CPACK_SYSTEM_PROCESSOR_LOWER}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-source")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/package")
set(CPACK_GENERATOR "TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
    "/build/"
    "/\\\\.codex$"
    "/\\\\.git/"
    "/\\\\.claude/"
    "CMakeLists\\\\.txt\\\\.user"
    "/test_file\\\\.txt$"
    "/test_readonly\\\\.txt$"
    "/test_dir/"
    "/.*\\\\.tar\\\\.gz$"
    "/.*\\\\.tar\\\\.bz2$"
    "/.*\\\\.tar\\\\.xz$"
    "/.*\\\\.tar\\\\.Z$"
    ".*~$"
)
include(CPack)
