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

#linux build (for github "ubuntu-latest")


#fail on first error
set -e

#we will need sudo later may as well do a sudo now
if [[ ! $(sudo echo 0) ]]; then exit; fi

#install dependancies and build tools
sudo apt-get install qt5-default cpputest build-essential qtmultimedia5-dev cmake libvorbis-dev libogg-dev libqt5multimedia5-plugins checkinstall libqcustomplot-dev libqt5svg5-dev libzmq3-dev  librtlsdr-dev libusb-dev -y

#get script path
SCRIPT=$(realpath $0)
SCRIPTPATH=$(dirname $SCRIPT)

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

#build for release
#cd SDRReceiver
qmake CONFIG-="CI"
make
make INSTALL_ROOT=$PWD/${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1 install
SDR_INSTALL_PATH=$(cat SDRReceiver.pro | sed -n -e 's/^INSTALL_PATH[|( ).]= //p')
SDR_INSTALL_PATH=${SDR_INSTALL_PATH//$'\r'/}
echo 'SDR_INSTALL_PATH='${SDR_INSTALL_PATH}

#add control
cat <<EOT > control
Package: ${PACKAGE_NAME}
Source: ${PACKAGE_SOURCE}
Section: base
Priority: extra
Depends: qt5-default (>= 5.12), qtmultimedia5-dev, libvorbis-dev, libogg-dev, libqt5multimedia5-plugins, libqcustomplot-dev, libqt5svg5-dev, libzmq3-dev, libusb-dev, librtlsdr-dev
Provides: ${PACKAGE_NAME}
Maintainer: ${MAINTAINER}
Version: ${PACKAGE_VERSION%_*}
License: MIT
Architecture: $(dpkg --print-architecture)
Description: SDR Receiver for JAERO
EOT
echo "" >> control
mkdir -p ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/DEBIAN
cp control ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/DEBIAN
#add path command
mkdir -p ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/usr/local/bin
cat <<EOT > ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/usr/local/bin/SDRReceiver
#!/bin/bash
/opt/SDRReceiver/bin/SDRReceiver
EOT
chmod +x ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/usr/local/bin/SDRReceiver


#build and install package
dpkg-deb --build ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1
sudo apt install ./${PACKAGE_NAME}*.deb -y
sudo ldconfig
cd ..

#package
mkdir SDRReceiver/bin
mkdir SDRReceiver/bin/sdreceiver
cp SDRReceiver/*.deb SDRReceiver/bin/sdreceiver
cd SDRReceiver/bin
cat <<EOT > sdreceiver/install.sh
#!/bin/bash
#installs built packages
sudo apt install ./*.deb
sudo ldconfig
EOT
chmod +x sdreceiver/install.sh
cat <<EOT > sdreceiver/uninstall.sh
#!/bin/bash
#removes built packages
sudo dpkg --remove libacars-dev libcorrect-dev libaeroambe-dev SDRReceiver libzmq3-dev libusb-dev librtlsdr-dev
sudo ldconfig
EOT
chmod +x sdreceiver/uninstall.sh
cat <<EOT > sdreceiver/readme.md
# SDRReceiver ${PACKAGE_VERSION}

### OS: $(lsb_release -d | cut -f 2)
### Build Date: $(date -u)

Cheers,<br>
ci-linux-build.sh
EOT
#compress

tar -czvf ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1_linux_$(uname -m).tar.gz sdreceiver
echo "done"
