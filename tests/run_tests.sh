#!/bin/bash
# Run Vista tests
# Usage: ./run_tests.sh [--verbose]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== Vista Test Runner ===${NC}"
echo ""

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Build directory not found. Creating...${NC}"
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure if needed
if [ ! -f "Makefile" ] && [ ! -f "build.ninja" ]; then
    echo -e "${YELLOW}Configuring project...${NC}"
    cmake -DBUILD_TESTS=ON ..
fi

# Build tests
echo -e "${YELLOW}Building tests...${NC}"
if [ -f "build.ninja" ]; then
    ninja test_config
else
    make test_config
fi

echo ""
echo -e "${YELLOW}Running tests...${NC}"
echo ""

# Run tests
if [ "$1" == "--verbose" ]; then
    ctest --output-on-failure --verbose
else
    ctest --output-on-failure
fi

EXIT_CODE=$?

echo ""
if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
else
    echo -e "${RED}Some tests failed!${NC}"
fi

exit $EXIT_CODE
