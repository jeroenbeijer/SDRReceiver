#!/bin/bash
set -e

# -------------------------------------------------
# Config
# -------------------------------------------------

PACKAGE_NAME=SDRReceiver
PACKAGE_VERSION=2.0

BUILD_TYPE=Release
BUILD_DIR=build-mingw64
STAGE_DIR="$(pwd)/release"
INSTALL_PREFIX=.


# -------------------------------------------------
# Ensure MinGW64 shell
# -------------------------------------------------

if [[ "$MSYSTEM" != "MINGW64" ]]; then
    echo "ERROR: Run this from the MSYS2 MinGW64 shell"
    exit 1
fi

# -------------------------------------------------
# Dependencies
# -------------------------------------------------

pacman -S --needed --noconfirm \
    git \
    mingw-w64-x86_64-toolchain \
    mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-ninja \
    mingw-w64-x86_64-qt6-base \
    mingw-w64-x86_64-qt6-multimedia \
    mingw-w64-x86_64-qt6-svg \
    mingw-w64-x86_64-zeromq \
    mingw-w64-x86_64-rtl-sdr \
    mingw-w64-x86_64-libusb \
    zip unzip p7zip




# -------------------------------------------------
# Repo setup
# -------------------------------------------------

SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
cd "$SCRIPTPATH"

if git rev-parse --is-shallow-repository | grep -q true; then
    git fetch --prune --unshallow --tags || true
else
    git fetch --prune --tags || true
fi

echo "PACKAGE_NAME=${PACKAGE_NAME}"
echo "PACKAGE_VERSION=${PACKAGE_VERSION}"

# -------------------------------------------------
# Build (CMake + Qt6)
# -------------------------------------------------


rm -rf "$BUILD_DIR" "$STAGE_DIR" 
mkdir -p "$BUILD_DIR"

cmake -S . -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_INSTALL_PREFIX="$STAGE_DIR" \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++

cmake --build "$BUILD_DIR"

# -------------------------------------------------
# Stage install (THIS IS THE FIX)
# -------------------------------------------------

rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR"

cmake --install "$BUILD_DIR"

# -------------------------------------------------
# Deploy Qt runtime
# -------------------------------------------------

cd "$STAGE_DIR/bin"

windeployqt6.exe \
    --release \
    --no-translations \
    --compiler-runtime \
    SDRReceiver.exe

# Extra non-Qt DLLs
cp /mingw64/bin/libzmq.dll .
cp /mingw64/bin/librtlsdr.dll .
cp /mingw64/bin/libusb-1.0.dll .

# Toolchain / system / third-party DLLs (explicit, as before)
cp /mingw64/bin/libstdc++-6.dll .
cp /mingw64/bin/libgcc_s_seh-1.dll .
cp /mingw64/bin/libwinpthread-1.dll .
cp /mingw64/bin/zlib1.dll .
cp /mingw64/bin/libdouble-conversion.dll .
cp /mingw64/bin/libicuin78.dll .
cp /mingw64/bin/libicuuc78.dll .
cp /mingw64/bin/libpcre2-16-0.dll .
cp /mingw64/bin/libzstd.dll .
cp /mingw64/bin/libharfbuzz-0.dll .
cp /mingw64/bin/libpng16-16.dll .
cp /mingw64/bin/libfreetype-6.dll .
cp /mingw64/bin/libgraphite2.dll .
cp /mingw64/bin/libglib-2.0-0.dll .
cp /mingw64/bin/libicudt78.dll .
cp /mingw64/bin/libb2-1.dll .
cp /mingw64/bin/libbrotlidec.dll .
cp /mingw64/bin/libintl-8.dll .
cp /mingw64/bin/libpcre2-8-0.dll .
cp /mingw64/bin/libbrotlicommon.dll .
cp /mingw64/bin/libiconv-2.dll .
cp /mingw64/bin/libzmq.dll .
cp /mingw64/bin/librtlsdr.dll .
cp /mingw64/bin/libsodium-26.dll .
cp /mingw64/bin/libusb-1.0.dll .
cp /mingw64/bin/libmd4c.dll .



# -------------------------------------------------
# README
# -------------------------------------------------

cd "$STAGE_DIR"

cat <<EOF > readme.md
# SDRReceiver ${PACKAGE_VERSION}

OS: Windows
Architecture: x86_64
Build date (UTC): $(date -u)

Built with Qt 6 + CMake (MSYS2 MinGW64).
EOF

# -------------------------------------------------
# Package
# -------------------------------------------------

PACKAGE_ROOT="${PACKAGE_NAME}"

rm -rf "$PACKAGE_ROOT"
mkdir "$PACKAGE_ROOT"

# Move runtime files into package root
cp -r "$STAGE_DIR/bin/"* "$PACKAGE_ROOT/"
cp "$STAGE_DIR/readme.md" "$PACKAGE_ROOT/"

zip -r ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1_win_qt6_$(uname -m).zip "$PACKAGE_ROOT"

echo "done"
