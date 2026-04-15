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

    if [[ "$tool" == */* ]]; then
        if [ -x "$tool" ]; then
            return
        fi
    elif command -v "$tool" >/dev/null 2>&1; then
        return
    fi

    echo -e "${RED}Missing required tool: ${tool}${NC}"
    if [ -n "$package_hint" ]; then
        echo -e "${YELLOW}Install it first, for example: ${package_hint}${NC}"
    fi
    echo -e "${YELLOW}On Ubuntu 24.04, you can also run: sudo bash ./install_deps_ubuntu.sh${NC}"
    exit 1
}

bootstrap_cppcheck() {
    local install_dir="$1"
    local source_dir="$2"
    local build_dir="$3"
    local version="$4"

    if [ -x "$install_dir/bin/cppcheck" ]; then
        return
    fi

    require_tool curl "sudo apt install curl"

    echo -e "${YELLOW}Bootstrapping pinned cppcheck ${version}...${NC}"
    mkdir -p .tools
    if [ ! -d "$source_dir" ]; then
        curl -L "https://github.com/danmar/cppcheck/archive/refs/tags/${version}.tar.gz" \
            | tar -xz -C .tools
    fi

    cmake -S "$source_dir" \
        -B "$build_dir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$PWD/$install_dir"
    cmake --build "$build_dir" -j"$(nproc)"
    cmake --install "$build_dir"
}

CPPCHECK_VERSION="${CPPCHECK_VERSION:-2.20.0}"
CPPCHECK_INSTALL_DIR=".tools/cppcheck-${CPPCHECK_VERSION}"
CPPCHECK_SOURCE_DIR=".tools/cppcheck-${CPPCHECK_VERSION}"
CPPCHECK_BUILD_DIR=".tools/cppcheck-build"
DEFAULT_CPPCHECK_BIN="$CPPCHECK_INSTALL_DIR/bin/cppcheck"
CPPCHECK_BIN="${CPPCHECK_BIN:-}"
CLEAN_BUILD=0
BOOTSTRAP_CPPCHECK=0
ENABLE_NATIVE_TUNING=0

for arg in "$@"; do
    case "$arg" in
        --clean|-c)
            CLEAN_BUILD=1
            ;;
        --bootstrap-cppcheck)
            BOOTSTRAP_CPPCHECK=1
            ;;
        --native-tuning)
            ENABLE_NATIVE_TUNING=1
            ;;
        *)
            echo -e "${RED}Unknown argument: ${arg}${NC}"
            echo -e "${YELLOW}Usage: ./build.sh [--clean|-c] [--bootstrap-cppcheck] [--native-tuning]${NC}"
            exit 1
            ;;
    esac
done

if [ "$BOOTSTRAP_CPPCHECK" -eq 1 ]; then
    bootstrap_cppcheck "$CPPCHECK_INSTALL_DIR" "$CPPCHECK_SOURCE_DIR" "$CPPCHECK_BUILD_DIR" "$CPPCHECK_VERSION"
    CPPCHECK_BIN="$DEFAULT_CPPCHECK_BIN"
elif [ -z "$CPPCHECK_BIN" ] && [ -x "$DEFAULT_CPPCHECK_BIN" ]; then
    CPPCHECK_BIN="$DEFAULT_CPPCHECK_BIN"
fi

CPPCHECK_BIN="${CPPCHECK_BIN:-cppcheck}"

echo -e "${GREEN}=== gbglow Build Script ===${NC}"

require_tool cmake "sudo apt install cmake"
require_tool pkg-config "sudo apt install pkg-config"
require_tool "$CPPCHECK_BIN" "sudo apt install cppcheck"
echo -e "${YELLOW}Using cppcheck: ${CPPCHECK_BIN}${NC}"

if ! pkg-config --exists sdl2; then
    echo -e "${RED}Missing SDL2 development package detected via pkg-config.${NC}"
    echo -e "${YELLOW}Install it first, for example: sudo apt install libsdl2-dev${NC}"
    echo -e "${YELLOW}On Ubuntu 24.04, you can also run: sudo bash ./install_deps_ubuntu.sh${NC}"
    exit 1
fi

if [ "$CLEAN_BUILD" -eq 1 ]; then
    if [ -d "build" ]; then
        echo -e "${YELLOW}Cleaning previous build...${NC}"
        rm -rf build
    fi
fi

CMAKE_ARGS=()

if [ "$ENABLE_NATIVE_TUNING" -eq 1 ]; then
    echo -e "${YELLOW}Enabling host-specific release tuning...${NC}"
    CMAKE_ARGS+=("-DGBGLOW_ENABLE_NATIVE_TUNING=ON")
fi

# Create build directory if needed
mkdir -p build

# Navigate to build directory
cd build

# Run CMake configuration
echo -e "${YELLOW}Configuring with CMake...${NC}"
cmake .. "${CMAKE_ARGS[@]}"

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
"$CPPCHECK_BIN" --enable=all --inline-suppr --quiet \
    --suppress=missingIncludeSystem \
    --suppress=missingInclude \
    --suppress=normalCheckLevelMaxBranches \
    --suppress=checkersReport \
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
echo -e "${YELLOW}Tip:   ./build.sh --bootstrap-cppcheck --clean to match CI locally${NC}"
echo -e "${YELLOW}Tip:   ./build.sh --native-tuning for a host-specific personal build${NC}"
