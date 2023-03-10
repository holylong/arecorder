cmake_minimum_required(VERSION 3.5)

project(arecorder VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions("-DUNICODE" "-D_UNICODE")

set(FFMPEG_INC_DIR "D:\\commonlib\\include" CACHE STRING "ffmpeg header directory")
set(FFMPEG_LIBS_DIR "D:\\commonlib\\lib" CACHE STRING "ffmpeg library directory")

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check https://doc.qt.io/qt/deployment-android.html for more information.
# They need to be set before the find_package( ...) calls below.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets LinguistTools REQUIRED)

set(TS_FILES arecorder_zh_CN.ts)

set(PROJECT_SOURCES
        main.cpp

        mainwindow.cpp mainwindow.h
        newgroupbox.h newgroupbox.cpp
        mainwindow.ui
        qtoast.h qtoast.cpp

        audio_rec_thread.h audio_rec_thread.cpp
        audio_manager.cpp audio_manager.h
        audio_resample.cpp audio_resample.h

        tracer.h
        config.h
        ffmpeg_util.h
        app.qrc
        ${TS_FILES}
)

include_directories(${FFMPEG_INC_DIR})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(arecorder
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(arecorder SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(arecorder
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(arecorder PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

target_link_directories(arecorder PRIVATE ${FFMPEG_LIBS_DIR})
target_link_libraries(arecorder PRIVATE avdevice avcodec avformat avutil swresample)

set_target_properties(arecorder PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(arecorder)
endif()


if(UNIX)
    target_link_libraries(arecorder PRIVATE ncursesw cairo)
endif(UNIX)
