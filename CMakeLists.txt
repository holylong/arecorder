cmake_minimum_required(VERSION 3.5)

project(voicebata VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions("-DUNICODE" "-D_UNICODE")

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

set(TS_FILES voicebata_zh_CN.ts)

set(PROJECT_SOURCES
        main.cpp

        mainwindow.cpp mainwindow.h
        newgroupbox.h newgroupbox.cpp
        mainwindow.ui

        audio_rec_thread.h audio_rec_thread.cpp
        audio_manager.cpp audio_manager.h
        audio_resample.cpp audio_resample.h

        tracer.h
        config.h
        ffmpeg_util.h
        app.qrc
        ${TS_FILES}
)

include_directories(D:/commonlib/include)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(voicebata
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(voicebata SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(voicebata
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(voicebata PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

target_link_directories(voicebata PRIVATE D:/commonlib/ffmpeg-20200721-b5f1e05-win64-dev/lib)
target_link_libraries(voicebata PRIVATE avdevice avcodec avformat avutil swresample)

set_target_properties(voicebata PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(voicebata)
endif()
