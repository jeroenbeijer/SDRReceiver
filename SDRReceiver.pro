
DEFINES += SDR_VERSION=\\\"v1.0.1\\\"

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = SDRReceiver
TEMPLATE = app

INSTALL_PATH = /opt/sdrreceiver

CONFIG += c++11

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
    jonti/dsp.cpp \
    jonti/fftwrapper.cpp \
    jonti/fftrwrapper.cpp \
    gnuradio//firfilter.cpp \
    halfbanddecimator.cpp \
    kiss_fft130/kiss_fastfir.c \
    kiss_fft130/kiss_fastfir_complex.c \
    kiss_fft130/kiss_fastfir_real.c \
    kiss_fft130/kiss_fft.c \
    kiss_fft130/kiss_fftr.c \
    main.cpp \
    oscillator.cpp \
    qcustomplot.cpp \
    mainwindow.cpp \
    jonti/sdr.cpp \
    sdrj.cpp \
    vfo.cpp \
    zmqpublisher.cpp

HEADERS += \
    jonti/dsp.h \
    jonti/fftrwrapper.h \
    jonti/fftwrapper.h \
    gnuradio/firfilter.h \
    halfbanddecimator.h \
    kiss_fft130/_kiss_fft_guts.h \
    kiss_fft130/kiss_fastfir.h \
    kiss_fft130/kiss_fastfir_complex.h \
    kiss_fft130/kiss_fastfir_real.h \
    kiss_fft130/kiss_fft.h \
    kiss_fft130/kiss_fftr.h \
    oscillator.h \
    qcustomplot.h \
    mainwindow.h \
    jonti/sdr.h \
    sdrj.h \
    vfo.h \
    zmqpublisher.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -Ofast


win32 {
#message("windows")
LIBS += -llibzmq -llibrtlsdr.dll 
} else {
#message("not windows")
LIBS += -lzmq -lrtlsdr
}





