#!/bin/bash

cmake -G "Unix Makefiles" \
    -S "$SOURCES_DIR" \
    -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DARM_NONE_EABI_TOOLCHAIN_PATH="$TOOLS_DIR/$GCC_ARM_PATH" \
    -DNRF5_SDK_PATH="$TOOLS_DIR/$NRF_SDK_VER" \
    -DBUILD_DFU=1 \
    -DBUILD_RESOURCES=1 \
    -DENABLE_WATCHFACES="WatchFace::PineTimeStyle,WatchFace::Terminal" \
    -DENABLE_USERAPPS="Apps::Alarm, Apps::Timer, Apps::Steps, Apps::HeartRate,Apps::Music, Apps::StopWatch, Apps::Calendar"
