#!/bin/bash

# gbglow Build Script
# Builds the Game Boy Color emulator

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

require_tool() {
    local tool="$1"
    local package_hint="$2"

    if ! command -v "$tool" >/dev/null 2>&1; then
        echo -e "${RED}Missing required tool: ${tool}${NC}"
        if [ -n "$package_hint" ]; then
            echo -e "${YELLOW}Install it first, for example: ${package_hint}${NC}"
        fi
        echo -e "${YELLOW}On Ubuntu 24.04, you can also run: sudo bash ./install_deps_ubuntu.sh${NC}"
        exit 1
    fi
}

echo -e "${GREEN}=== gbglow Build Script ===${NC}"

require_tool cmake "sudo apt install cmake"
require_tool pkg-config "sudo apt install pkg-config"
require_tool cppcheck "sudo apt install cppcheck"

if ! pkg-config --exists sdl2; then
    echo -e "${RED}Missing SDL2 development package detected via pkg-config.${NC}"
    echo -e "${YELLOW}Install it first, for example: sudo apt install libsdl2-dev${NC}"
    echo -e "${YELLOW}On Ubuntu 24.04, you can also run: sudo bash ./install_deps_ubuntu.sh${NC}"
    exit 1
fi

# Parse arguments
if [ "$1" = "--clean" ] || [ "$1" = "-c" ]; then
    if [ -d "build" ]; then
        echo -e "${YELLOW}Cleaning previous build...${NC}"
        rm -rf build
    fi
fi

# Create build directory if needed
mkdir -p build

# Navigate to build directory
cd build

# Run CMake configuration
echo -e "${YELLOW}Configuring with CMake...${NC}"
cmake ..

# Build the project
echo -e "${YELLOW}Building project...${NC}"
cmake --build . -j$(nproc)

# Run tests
echo -e "${YELLOW}Running tests...${NC}"
ctest --output-on-failure

# Run static analysis
echo -e "${YELLOW}Running static analysis...${NC}"
cd ..
CPPCHECK_EXIT=0
cppcheck --enable=all --inline-suppr --quiet \
    --suppress=missingIncludeSystem \
    --suppress=missingInclude \
    --suppress=unmatchedSuppression \
    --suppressions-list=cppcheck.suppressions \
    --error-exitcode=1 \
    -I src/ src/ 2>&1 || CPPCHECK_EXIT=$?
cd build
if [ $CPPCHECK_EXIT -ne 0 ]; then
    echo -e "${RED}Static analysis found issues — see output above.${NC}"
    exit 1
fi
echo -e "${GREEN}Static analysis clean.${NC}"

echo -e "${GREEN}=== Build Complete! ===${NC}"
echo -e "${GREEN}Executable: build/gbglow${NC}"
echo -e "${YELLOW}Usage: ./build/gbglow <rom_file>${NC}"
echo -e "${YELLOW}Tip:   ./build.sh --clean for a full rebuild${NC}"
