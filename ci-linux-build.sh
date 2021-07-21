#!/bin/bash

#linux build (for github "ubuntu-latest")


#fail on first error
set -e

#we will need sudo later may as well do a sudo now
if [[ ! $(sudo echo 0) ]]; then exit; fi

#install dependancies and build tools
sudo apt-get install qt5-default cpputest build-essential qtmultimedia5-dev cmake libvorbis-dev libogg-dev libqt5multimedia5-plugins checkinstall libqcustomplot-dev libqt5svg5-dev libzmq3-dev rtl-sdr libusb -y

#get script path
SCRIPT=$(realpath $0)
SCRIPTPATH=$(dirname $SCRIPT)

#SDRReceiver
cd $SCRIPTPATH
#needed for github actions
git fetch --prune --unshallow --tags || true
git status > /dev/null 2>&1
PACKAGE_VERSION=$(git describe --tags --match 'v*' --dirty 2> /dev/null | tr -d v)
PACKAGE_NAME=sdrreceiver
MAINTAINER=https://github.com/jeroenbeijer
PACKAGE_SOURCE=https://github.com/jeroenbeijer/SDRReceiver
echo "PACKAGE_NAME="$PACKAGE_NAME
echo "PACKAGE_VERSION="$PACKAGE_VERSION
echo "MAINTAINER="$MAINTAINER
echo "PACKAGE_SOURCE="$PACKAGE_SOURCE
cd SDRReceiver
#run unit tests
qmake CONFIG+="CI"
make
./SDRReceiver -v
rm SDRReceiver
#build for release
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
Depends: qt5-default (>= 5.12), qtmultimedia5-dev, libvorbis-dev, libogg-dev, libqt5multimedia5-plugins, libqcustomplot-dev, libqt5svg5-dev, libzmq3-dev libusb rtlsdr
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
cat <<EOT > ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/usr/local/bin/sdrreceiver
#!/bin/bash
/opt/sdrreceiver/sdrreceiver
EOT
chmod +x ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/usr/local/bin/sdrreceiver


#build and install package
dpkg-deb --build ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1
sudo apt install ./${PACKAGE_NAME}*.deb -y
sudo ldconfig
cd ../..

#package
mkdir sdrreceiver/bin
mkdir sdrreceiver/bin/sdrreceiver
cp sdrreceiver/sdrreceiver/*.deb sdrreceiver/bin/sdrreceiver
cd sdrreceiver/bin
cat <<EOT > sdrreceiver/install.sh
#!/bin/bash
#installs built packages
sudo apt install ./*.deb
sudo ldconfig
EOT
chmod +x sdrreceiver/install.sh
cat <<EOT > sdrreceiver/uninstall.sh
#!/bin/bash
#removes built packages
sudo dpkg --remove libacars-dev libcorrect-dev libaeroambe-dev sdrreceiver libusb rtl-sdr
sudo ldconfig
EOT
chmod +x sdrreceiver/uninstall.sh
cat <<EOT > sdrreceiver/readme.md
# SDRReceiver ${PACKAGE_VERSION}

### OS: $(lsb_release -d | cut -f 2)
### Build Date: $(date -u)

Cheers,<br>
ci-linux-build.sh
EOT
#compress
tar -czvf ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1_linux_$(uname -m).tar.gz sdrreceiver
echo "done"
