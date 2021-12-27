#!/bin/bash

#The MIT License (MIT)

#Copyright (c) 2015-2019 Jonathan Olds

#Permission is hereby granted, free of charge, to any person obtaining a copy
#of this software and associated documentation files (the "Software"), to deal
#in the Software without restriction, including without limitation the rights
#to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#copies of the Software, and to permit persons to whom the Software is
#furnished to do so, subject to the following conditions:

#The above copyright notice and this permission notice shall be included in all
#copies or substantial portions of the Software.

#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#SOFTWARE.

#windows build (for github "windows-latest")
#this is for 64bit mingw and msys2 install. all is done on the command line.


#fail on first error
set -e

pacman -S --needed --noconfirm git mingw-w64-x86_64-toolchain autoconf libtool mingw-w64-x86_64-cpputest mingw-w64-x86_64-qt5 mingw-w64-x86_64-cmake mingw-w64-x86_64-libvorbis zip p7zip unzip mingw-w64-x86_64-zeromq mingw-w64-x86_64-libusb

#get script path
SCRIPT=$(realpath $0)
SCRIPTPATH=$(dirname $SCRIPT)
cd $SCRIPTPATH/..


#rtl-sdr
FOLDER="rtl-sdr"
URL="https://github.com/osmocom/rtl-sdr"
if [ ! -d "$FOLDER" ] ; then
    git clone $URL $FOLDER
    cd "$FOLDER"
else
    cd "$FOLDER"
    git pull $URL
fi
mkdir -p build && cd build
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
cp /mingw64/bin/libicuin69.dll $PWD
cp /mingw64/bin/libicuuc69.dll $PWD
cp /mingw64/bin/libpcre2-16-0.dll $PWD
cp /mingw64/bin/libzstd.dll $PWD
cp /mingw64/bin/libharfbuzz-0.dll $PWD
cp /mingw64/bin/libpng16-16.dll $PWD
cp /mingw64/bin/libfreetype-6.dll $PWD
cp /mingw64/bin/libgraphite2.dll $PWD
cp /mingw64/bin/libglib-2.0-0.dll $PWD
cp /mingw64/bin/libicudt69.dll $PWD
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
# SDRReceiver ${PACKAGE_VERSION}

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
