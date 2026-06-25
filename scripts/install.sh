#!/usr/bin/env bash
set -e

if command -v apt >/dev/null 2>&1; then
    sudo apt update
    sudo apt install -y build-essential cmake pkg-config \
        libsqlite3-dev libcurl4-openssl-dev
elif command -v dnf >/dev/null 2>&1; then
    sudo dnf install -y gcc make cmake pkg-config \
        sqlite-devel libcurl-devel
elif command -v pacman >/dev/null 2>&1; then
    sudo pacman -Syu --needed base-devel cmake pkgconf \
        sqlite curl
else
    echo "Unsupported distro. Please install sqlite3 dev package, libcurl dev package, cmake, make, gcc manually."
    exit 1
fi

echo "Dependencies installed."