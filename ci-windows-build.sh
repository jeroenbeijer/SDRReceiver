#!/bin/bash

#windows build (for github "windows-latest")
#this is for 64bit mingw and msys2 install. all is done on the command line.


#fail on first error
set -e

pacman -S --needed --noconfirm git mingw-w64-x86_64-toolchain autoconf libtool mingw-w64-x86_64-cpputest mingw-w64-x86_64-qt5 mingw-w64-x86_64-cmake mingw-w64-x86_64-libvorbis zip p7zip unzip mingw-w64-x86_64-zeromq mingw-w64-x86_64-libusb

#get script path
SCRIPT=$(realpath $0)
SCRIPTPATH=$(dirname $SCRIPT)
cd $SCRIPTPATH/..


#rtsdr
git clone https://github.com/osmocom/rtl-sdr
cd rtl-sdr
mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX:PATH=/mingw64/ ..
mingw32-make
mingw32-make DESTDIR=/../ install

#SDRReceiver
cd $SCRIPTPATH
#needed for github actions
git fetch --prune --unshallow --tags || true
git status > /dev/null 2>&1
PACKAGE_VERSION=1.0
PACKAGE_NAME=SDRReceiver
MAINTAINER=https://github.com/jeroenbeijer
PACKAGE_SOURCE=https://github.com/jeroenbeijer/SDRReceiver
echo "PACKAGE_NAME="$PACKAGE_NAME
echo "PACKAGE_VERSION="$PACKAGE_VERSION
echo "MAINTAINER="$MAINTAINER
echo "PACKAGE_SOURCE="$PACKAGE_SOURCE

qmake
mingw32-make

#package
mkdir release/sdrreceiver
cp release/SDRReceiver.exe release/SDRReceiver/
cd release/SDRReceiver
windeployqt.exe --force SDRReceiver.exe
cp /mingw64/bin/libstdc++-6.dll $PWD
cp /mingw64/bin/libgcc_s_seh-1.dll $PWD
cp /mingw64/bin/libwinpthread-1.dll $PWD
cp /mingw64/bin/zlib1.dll $PWD
cp /mingw64/bin/Qt5PrintSupport.dll $PWD
cp /mingw64/bin/libdouble-conversion.dll $PWD
cp /mingw64/bin/libicuin68.dll $PWD
cp /mingw64/bin/libicuuc68.dll $PWD
cp /mingw64/bin/libpcre2-16-0.dll $PWD
cp /mingw64/bin/libzstd.dll $PWD
cp /mingw64/bin/libharfbuzz-0.dll $PWD
cp /mingw64/bin/libpng16-16.dll $PWD
cp /mingw64/bin/libfreetype-6.dll $PWD
cp /mingw64/bin/libgraphite2.dll $PWD
cp /mingw64/bin/libglib-2.0-0.dll $PWD
cp /mingw64/bin/libicudt68.dll $PWD
cp /mingw64/bin/libbz2-1.dll $PWD
cp /mingw64/bin/libbrotlidec.dll $PWD
cp /mingw64/bin/libintl-8.dll $PWD
cp /mingw64/bin/libpcre-1.dll $PWD
cp /mingw64/bin/libbrotlicommon.dll $PWD
cp /mingw64/bin/libiconv-2.dll $PWD
cp /mingw64/bin/libzmq.dll $PWD
cp /mingw64/bin/librtlsdr.dll $PWD
cp /mingw64/bin/libsodium-23.dll $PWD
cp /mingw64/bin/libusb-1.0.dll $PWD
#add readme
cat <<EOT > readme.md
# JAERO ${PACKAGE_VERSION}

### OS Name: $(systeminfo | sed -n -e 's/^OS Name://p' | awk '{$1=$1;print}')
### OS Version: $(systeminfo | sed -n -e 's/^OS Version://p' | awk '{$1=$1;print}')
### System Type: $(systeminfo | sed -n -e 's/^System Type://p' | awk '{$1=$1;print}')
### Build Date: $(date -u)

Cheers,<br>
ci-windows-build.sh
EOT
#compress
cd ..
zip -r ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1_win_$(uname -m).zip SDRReceiver
