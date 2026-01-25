#!/bin/bash
set -e

# -------------------------------------------------
# Config
# -------------------------------------------------

PACKAGE_NAME=SDRReceiver
PACKAGE_VERSION=2.0
MAINTAINER=https://github.com/jeroenbeijer
PACKAGE_SOURCE=https://github.com/jeroenbeijer/SDRReceiver

BUILD_TYPE=Release
INSTALL_PREFIX=/opt/${PACKAGE_NAME}

# -------------------------------------------------
# Prep
# -------------------------------------------------

if [[ ! $(sudo echo 0) ]]; then exit 1; fi

sudo apt-get update

sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    qt6-base-dev \
    qt6-multimedia-dev \
    libqt6svg6-dev \
    libzmq3-dev \
    librtlsdr-dev \
    libusb-dev \
    lsb-release

SCRIPT_PATH=$(realpath "$0")
ROOT_DIR=$(dirname "$SCRIPT_PATH")

cd "$ROOT_DIR"

git fetch --prune --unshallow --tags || true

echo "PACKAGE_NAME=${PACKAGE_NAME}"
echo "PACKAGE_VERSION=${PACKAGE_VERSION}"
echo "INSTALL_PREFIX=${INSTALL_PREFIX}"

# -------------------------------------------------
# Build (CMake + Qt6)
# -------------------------------------------------

BUILD_DIR=build-linux
INSTALL_ROOT=${ROOT_DIR}/${PACKAGE_NAME}_${PACKAGE_VERSION}-1

rm -rf "$BUILD_DIR" "$INSTALL_ROOT"
mkdir -p "$BUILD_DIR"


cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"

cmake --build "$BUILD_DIR" --parallel

DESTDIR="$INSTALL_ROOT" cmake --install "$BUILD_DIR"

# -------------------------------------------------
# Debian control file
# -------------------------------------------------

mkdir -p "${INSTALL_ROOT}/DEBIAN"

cat <<EOF > "${INSTALL_ROOT}/DEBIAN/control"
Package: ${PACKAGE_NAME}
Source: ${PACKAGE_SOURCE}
Section: base
Priority: extra
Maintainer: ${MAINTAINER}
Version: ${PACKAGE_VERSION}
Architecture: $(dpkg --print-architecture)
Depends: qt6-base-dev, qt6-multimedia-dev, qt6-svg-dev, libzmq3-dev, libusb-dev, librtlsdr-dev
Provides: ${PACKAGE_NAME}
Description: SDR Receiver for JAERO
EOF

chmod 755 "${INSTALL_ROOT}/DEBIAN"

# -------------------------------------------------
# Wrapper script
# -------------------------------------------------

mkdir -p "${INSTALL_ROOT}/usr/local/bin"

cat <<EOF > "${INSTALL_ROOT}/usr/local/bin/SDRReceiver"
#!/bin/bash
exec ${INSTALL_PREFIX}/bin/SDRReceiver "\$@"
EOF

chmod +x "${INSTALL_ROOT}/usr/local/bin/SDRReceiver"

# -------------------------------------------------
# Build .deb
# -------------------------------------------------

dpkg-deb --build "${INSTALL_ROOT}"

sudo apt install --reinstall ./${PACKAGE_NAME}_${PACKAGE_VERSION}-1.deb -y
sudo ldconfig

# -------------------------------------------------
# Package artifacts
# -------------------------------------------------

ARTIFACT_DIR=${ROOT_DIR}/SDRReceiver/bin/SDRReceiver
mkdir -p "$ARTIFACT_DIR"

cp ${PACKAGE_NAME}_${PACKAGE_VERSION}-1.deb "$ARTIFACT_DIR"

cat <<EOF > "${ARTIFACT_DIR}/install.sh"
#!/bin/bash
sudo apt install ./*.deb
sudo ldconfig
EOF

cat <<EOF > "${ARTIFACT_DIR}/uninstall.sh"
#!/bin/bash
sudo dpkg --remove ${PACKAGE_NAME}
sudo ldconfig
EOF

chmod +x "${ARTIFACT_DIR}/install.sh"
chmod +x "${ARTIFACT_DIR}/uninstall.sh"

cat <<EOF > "${ARTIFACT_DIR}/readme.md"
# ${PACKAGE_NAME} ${PACKAGE_VERSION}

OS: $(lsb_release -d | cut -f 2)
Build date: $(date -u)

Built with Qt 6 and CMake.
EOF

tar -czvf ${PACKAGE_NAME}_${PACKAGE_VERSION}_linux_$(uname -m).tar.gz SDRReceiver

echo "done"
