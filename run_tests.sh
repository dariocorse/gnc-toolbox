#!/bin/bash
##
# Test runner script for gnc-toolbox
# Provides easy ways to run different types of tests
##

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Project directory
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

print_header() {
    echo -e "${BLUE}================================${NC}"
    echo -e "${BLUE}    GnuCash Toolbox Test Suite${NC}"
    echo -e "${BLUE}================================${NC}"
    echo
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

# Function to build the project
build_project() {
    print_info "Building project..."
    cd "$PROJECT_DIR"
    make clean > /dev/null 2>&1 || true
    make debug
    cmake --build "$BUILD_DIR" --target test_gnc-toolbox
    if [ $? -eq 0 ]; then
        print_success "Build completed successfully"
    else
        print_error "Build failed"
        exit 1
    fi
}

# Function to run unit tests
run_unit_tests() {
    print_info "Running unit tests..."
    cd "$PROJECT_DIR"

    if [ -f "$BUILD_DIR/test_gnc-toolbox" ]; then
        "$BUILD_DIR/test_gnc-toolbox"
        if [ $? -eq 0 ]; then
            print_success "All unit tests passed"
        else
            print_error "Some unit tests failed"
            return 1
        fi
    else
        print_error "Test executable not found. Please build first."
        return 1
    fi
}

# Function to run integration tests via CTest
run_ctest() {
    print_info "Running tests via CTest..."
    cd "$BUILD_DIR"

    ctest --output-on-failure
    if [ $? -eq 0 ]; then
        print_success "All CTest tests passed"
    else
        print_error "Some CTest tests failed"
        return 1
    fi
}

# Function to run specific test suites
run_specific_test() {
    local test_pattern="$1"
    print_info "Running tests matching pattern: $test_pattern"
    cd "$PROJECT_DIR"

    if [ -f "$BUILD_DIR/test_gnc-toolbox" ]; then
        "$BUILD_DIR/test_gnc-toolbox" -p "$test_pattern"
        if [ $? -eq 0 ]; then
            print_success "Tests matching '$test_pattern' passed"
        else
            print_error "Tests matching '$test_pattern' failed"
            return 1
        fi
    else
        print_error "Test executable not found. Please build first."
        return 1
    fi
}

# Function to run validation tests
run_validation_tests() {
    print_info "Running manual validation tests..."
    cd "$PROJECT_DIR"

    # Test CLI help
    print_info "Testing CLI help functionality..."
    if ./build/gnc-toolbox --help > /dev/null 2>&1; then
        print_success "CLI help works"
    else
        print_error "CLI help failed"
        return 1
    fi

    # Test invalid arguments
    print_info "Testing error handling..."
    if ./build/gnc-toolbox import 2>&1 | grep -q "requires"; then
        print_success "Error handling works"
    else
        print_error "Error handling failed"
        return 1
    fi

    # Test version
    print_info "Testing version display..."
    if ./build/gnc-toolbox --version 2>&1 | grep -q "gnc-toolbox version"; then
        print_success "Version display works"
    else
        print_error "Version display failed"
        return 1
    fi
}

# Function to check test coverage
check_test_coverage() {
    print_info "Checking test coverage..."

    # Count test functions
    local test_count=$(find tests -name "test_*.c" -exec grep -h "^static void test_" {} \; | wc -l)
    local test_files=$(find tests -name "test_*.c" | wc -l)

    print_info "Test statistics:"
    echo "  Test files: $test_files"
    echo "  Test functions: $test_count"
    echo "  Test data files: $(find tests/data -name "*.csv" -o -name "*.ini" | wc -l)"

    print_success "Test coverage analysis complete"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTION]"
    echo
    echo "Test runner for gnc-toolbox project"
    echo
    echo "Options:"
    echo "  all                    Run all tests (build + unit + integration + validation)"
    echo "  unit                   Run only unit tests"
    echo "  ctest                  Run tests via CTest"
    echo "  validation            Run manual validation tests"
    echo "  coverage              Show test coverage information"
    echo "  specific <pattern>     Run tests matching pattern (e.g., '/date_parsing/*')"
    echo "  build                 Only build the project"
    echo "  clean                 Clean and rebuild, then run all tests"
    echo "  help, -h, --help      Show this help message"
    echo
    echo "Examples:"
    echo "  $0 all                           # Run complete test suite"
    echo "  $0 unit                          # Run only unit tests"
    echo "  $0 specific '/input_validation/*' # Run validation tests only"
    echo "  $0 clean                         # Clean rebuild and test"
}

# Main execution
main() {
    case "${1:-all}" in
        "all")
            print_header
            build_project && \
            run_unit_tests && \
            run_ctest && \
            run_validation_tests && \
            check_test_coverage
            if [ $? -eq 0 ]; then
                echo
                print_success "All tests completed successfully!"
                echo -e "${GREEN}🎉 gnc-toolbox is ready for use!${NC}"
            else
                print_error "Some tests failed"
                exit 1
            fi
            ;;
        "unit")
            print_header
            build_project && run_unit_tests
            ;;
        "ctest")
            print_header
            build_project && run_ctest
            ;;
        "validation")
            print_header
            build_project && run_validation_tests
            ;;
        "coverage")
            print_header
            check_test_coverage
            ;;
        "specific")
            if [ -z "$2" ]; then
                print_error "Please specify a test pattern"
                echo "Example: $0 specific '/date_parsing/*'"
                exit 1
            fi
            print_header
            build_project && run_specific_test "$2"
            ;;
        "build")
            print_header
            build_project
            ;;
        "clean")
            print_header
            print_info "Cleaning previous build..."
            cd "$PROJECT_DIR"
            make clean > /dev/null 2>&1 || true
            build_project && run_unit_tests && run_validation_tests
            ;;
        "help"|"-h"|"--help")
            show_usage
            ;;
        *)
            print_error "Unknown option: $1"
            echo
            show_usage
            exit 1
            ;;
    esac
}

# Execute main function with all arguments
main "$@"
