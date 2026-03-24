#!/bin/bash

# gbglow Run Script
# Builds (if needed) and runs the emulator

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if ROM file is provided
if [ $# -eq 0 ]; then
    echo -e "${RED}Error: No ROM file specified${NC}"
    echo -e "${YELLOW}Usage: $0 <rom_file>${NC}"
    echo -e "${YELLOW}Example: $0 pokemon_red.gb${NC}"
    exit 1
fi

ROM_FILE="$1"

# Check if ROM file exists
if [ ! -f "$ROM_FILE" ]; then
    echo -e "${RED}Error: ROM file not found: $ROM_FILE${NC}"
    exit 1
fi

# Build if executable doesn't exist
if [ ! -f "build/gbglow" ]; then
    echo -e "${YELLOW}Executable not found. Building...${NC}"
    ./build.sh
fi

# Run the emulator
echo -e "${GREEN}=== Starting gbglow ===${NC}"
echo -e "${YELLOW}ROM: $ROM_FILE${NC}"
echo ""
echo -e "${GREEN}Controls:${NC}"
echo "  Arrow keys = D-pad"
echo "  Z = A button"
echo "  X = B button"
echo "  Enter = Start"
echo "  Shift = Select"
echo "  ESC = Exit"
echo "  F11 = Toggle debugger"
echo "  F12 = Screenshot"
echo ""

./build/gbglow "$ROM_FILE"
