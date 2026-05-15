#!/bin/bash

set -euo pipefail

if [ "${EUID}" -ne 0 ]; then
    echo "This script must be run with sudo or as root."
    echo "Example: sudo bash ./install_deps_ubuntu.sh"
    exit 1
fi

if [ ! -r /etc/os-release ]; then
    echo "Cannot determine operating system. This installer only supports Ubuntu 24.04."
    exit 1
fi

. /etc/os-release

if [ "${ID:-}" != "ubuntu" ] || [ "${VERSION_ID:-}" != "24.04" ]; then
    echo "Unsupported distribution: ${PRETTY_NAME:-${ID:-unknown}}. This installer only supports Ubuntu 24.04."
    exit 1
fi

echo "Updating package index..."
apt-get update

echo "Installing gbglow build dependencies..."
apt-get install -y \
    build-essential \
    cmake \
    cppcheck \
    git \
    libsdl2-dev \
    pkg-config \
    zenity

echo "Done. You can now run ./build.sh"