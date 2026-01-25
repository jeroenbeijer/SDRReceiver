cmake_minimum_required(VERSION 3.16)

project(SDRReceiver LANGUAGES C CXX)

add_definitions(-DQT_NO_PACKAGE_AND_DEPLOYMENT)

# ---- Qt ----
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Widgets PrintSupport)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Sources ----
set(SOURCES
    audiobufferpool.cpp
    audiobufferpoolaccess.cpp
    audiobufferqueue.cpp
    complexsamplefanoutpool.cpp
    dsp/firhalfband.cpp
    dsp/firhilbert.cpp
    dsp/halfbanddecimator.cpp
    jonti/dsp.cpp
    jonti/fftwrapper.cpp
    gnuradio/firfilter.cpp
    kiss_fft130/kiss_fft.c
    main.cpp
    dsp/oscillator.cpp
    qcustomplot.cpp
    mainwindow.cpp
    radio.cpp
    rtlsdr.cpp
    vfo.cpp
    zmqpublisher.cpp
    filereader.cpp
    rtlworkerthread.cpp
)

set(HEADERS
    audiobufferpool.h
    audiobufferpoolaccess.h
    audiobufferqueue.h
    complexsamplefanoutpool.h
    dsp/firhalfband.h
    dsp/firhilbert.h
    dsp/halfbanddecimator.h
    jonti/dsp.h
    jonti/fftwrapper.h
    gnuradio/firfilter.h
    kiss_fft130/kiss_fft.h
    kiss_fft130/_kiss_fft_guts.h
    dsp/oscillator.h
    qcustomplot.h
    mainwindow.h
    radio.h
    rtlsdr.h
    vfo.h
    zmqpublisher.h
    filereader.h
    rtlworkerthread.h
)

set(FORMS
    mainwindow.ui
)

# ---- Optional SDRplay support ----
option(SDRPLAY "Enable SDRplay support" OFF)

if(SDRPLAY)
    list(APPEND SOURCES sdrplay.cpp)
    list(APPEND HEADERS sdrplay.h)
endif()

# ---- Build target ----
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ---- Defines ----
target_compile_definitions(${PROJECT_NAME} PRIVATE
    SDR_VERSION="v2.0.0"
    QT_DEPRECATED_WARNINGS
)

if(SDRPLAY)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_SDRPLAY)
endif()

# ---- Qt linkage ----
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Widgets
    Qt6::PrintSupport
)

# ---- Include paths ----
if(SDRPLAY)
    if(WIN32)
        target_include_directories(${PROJECT_NAME} PRIVATE "C:/sdrplay/API/inc")
    else()
        target_include_directories(${PROJECT_NAME} PRIVATE /usr/local/include/sdrplay)
    endif()
endif()

# ---- External libraries ----
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        zmq
        rtlSdr
    )
    if(SDRPLAY)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            sdrplay_api
        )
        target_link_directories(${PROJECT_NAME} PRIVATE "C:/sdrplay/API/x64")
    endif()
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        zmq
        rtlsdr
    )
    if(SDRPLAY)
        target_link_libraries(${PROJECT_NAME} PRIVATE sdrplay_api)
    endif()
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# -------------------------------------------------
# DSP optimizations (Release builds only)
# -------------------------------------------------

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Enabling DSP optimization flags")

        # Replace Qt default -O2 with -Ofast
        target_compile_options(${PROJECT_NAME} PRIVATE
            -Ofast
        )

        # ---- Portable x86_64 (Windows + public Linux) ----
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
            target_compile_options(${PROJECT_NAME} PRIVATE
                -march=x86-64
                -mno-avx2
                -mno-fma
            )
        endif()

        # ---- ARM 64-bit (Raspberry Pi 4) ----
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
            target_compile_options(${PROJECT_NAME} PRIVATE
                -march=armv8-a+simd
            )
        endif()

        # ---- ARM 32-bit ----
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7|arm")
            target_compile_options(${PROJECT_NAME} PRIVATE
                -march=armv7-a
                -mfpu=neon
                -mfloat-abi=hard
            )
        endif()
    endif()
endif()


