#!/bin/bash

# GBCrush Build Script
# Builds the Game Boy Color emulator

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== GBCrush Build Script ===${NC}"

# Clean previous build
if [ -d "build" ]; then
    echo -e "${YELLOW}Cleaning previous build...${NC}"
    rm -rf build
fi

# Create build directory
echo -e "${YELLOW}Creating build directory...${NC}"
mkdir build

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

echo -e "${GREEN}=== Build Complete! ===${NC}"
echo -e "${GREEN}Executable: build/gbglow${NC}"
echo -e "${YELLOW}Usage: ./build/gbglow <rom_file>${NC}"
