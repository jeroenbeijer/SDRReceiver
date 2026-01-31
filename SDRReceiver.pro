DEFINES += SDR_VERSION=\\\"v2.0.0\\\"

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = SDRReceiver
TEMPLATE = app

INSTALL_PATH = /opt/sdrreceiver

CONFIG += c++17
#CONFIG += sdrplay

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    audiobufferpool.cpp \
    audiobufferpoolaccess.cpp \
    audiobufferqueue.cpp \
    complexsamplefanoutpool.cpp \
    dsp/firhalfband.cpp \
    dsp/firhilbert.cpp \
    dsp/halfbanddecimator.cpp \
    jonti/dsp.cpp \
    jonti/fftwrapper.cpp \
    gnuradio/firfilter.cpp \
    kiss_fft130/kiss_fft.c \
    main.cpp \
    dsp/oscillator.cpp \
    qcustomplot.cpp \
    mainwindow.cpp \
    radio.cpp \
    rtlsdr.cpp \
    vfo.cpp \
    zmqpublisher.cpp \
    filereader.cpp \
    rtlworkerthread.cpp

HEADERS += \
    audiobufferpool.h \
    audiobufferpoolaccess.h \
    audiobufferqueue.h \
    complexsamplefanoutpool.h \
    dsp/firhalfband.h \
    dsp/firhilbert.h \
    dsp/halfbanddecimator.h \
    jonti/dsp.h \
    jonti/fftwrapper.h \
    gnuradio/firfilter.h \
    kiss_fft130/kiss_fft.h \
    kiss_fft130/_kiss_fft_guts.h \
    dsp/oscillator.h \
    qcustomplot.h \
    mainwindow.h \
    radio.h \
    rtlsdr.h \
    vfo.h \
    zmqpublisher.h \
    filereader.h \
    rtlworkerthread.h

# Only include SDRplay files if SDRplay is enabled
CONFIG(sdrplay) {
    SOURCES += sdrplay.cpp
    HEADERS += sdrplay.h
    DEFINES += HAVE_SDRPLAY
}

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

CONFIG(release, debug|release) {

    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE += -Ofast

    # ---- Portable x86_64 (Windows + public Linux) ----
    contains(QMAKE_HOST.arch, x86_64) {
      #  QMAKE_CXXFLAGS_RELEASE += -march=x86-64
      #  QMAKE_CXXFLAGS_RELEASE += -mno-avx2 -mno-fma
    }

    # ---- ARM 64-bit (Raspberry Pi 4) ----
    contains(QMAKE_HOST.arch, arm64)|contains(QMAKE_HOST.arch, aarch64) {
        QMAKE_CXXFLAGS_RELEASE += -march=armv8-a+simd
    }

    # ---- ARM 32-bit ----
    contains(QMAKE_HOST.arch, arm) {
        QMAKE_CXXFLAGS_RELEASE += -march=armv7-a -mfpu=neon -mfloat-abi=hard
    }
}




win32 {
#message("windows")
LIBS += -llibzmq -llibrtlsdr
} else {
#message("not windows")
LIBS += -lzmq -lrtlsdr
}

CONFIG(sdrplay) {
        win32 {
        #message("windows")
        INCLUDEPATH += "C:/sdrplay/API/inc"
        LIBS += -L"C:/sdrplay/API/x64" -lsdrplay_api
        } else {
        #message("not windows")
        INCLUDEPATH += /usr/local/include/sdrplay
        LIBS += -lsdrplay_api
        }
}

DISTFILES += \
    CMakeLists.txt




