cmake_minimum_required(VERSION 3.25)
project(mil-std-1553b-usb-terminal VERSION 0.1 LANGUAGES C CXX)

include(ProcessorCount)
ProcessorCount(N)
if(NOT N EQUAL 0)
    set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(WIN32_OPTION "")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Building for Linux")
    set(QT_PATH /usr/lib/x86_64-linux-gnu/cmake/Qt5)
    find_package(Qt5 COMPONENTS Core Gui Widgets SerialPort Concurrent)
    set(SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs/DriverSettingsDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs/InterfaceParamenetsDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/serial/SerialTransfer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/serial/SerialMonitorWindow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mil-std-1553b/unix/ltmk.c
    )
    set(FORMS
        ${CMAKE_CURRENT_SOURCE_DIR}/ui/MainWindow.ui
        ${CMAKE_CURRENT_SOURCE_DIR}/ui/DriverSettingsDialog.ui
        ${CMAKE_CURRENT_SOURCE_DIR}/ui/InterfaceParamenetsDialog.ui
        ${CMAKE_CURRENT_SOURCE_DIR}/ui/SerialMonitorWindow.ui
    )
    set(HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/service/UndoBlocker.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs/DriverSettingsDialog.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs/InterfaceParamenetsDialog.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/serial/SerialTransfer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/serial/SerialMonitorWindow.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mil-std-1553b/unix/ltmk.h
    )
    add_compile_options(-finput-charset=UTF-8)
    add_link_options(-Wl,-rpath-link,/usr/lib)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "Building for Windows")
    find_package(Qt5 COMPONENTS Core Gui Widgets SerialPort Concurrent)
    set(WIN32_OPTION "WIN32")
    enable_language("RC")
    set (WIN32_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/icon.rc)
    set(SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs/DriverSettingsDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs/InterfaceParamenetsDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/serial/SerialTransfer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/serial/SerialMonitorWindow.cpp
        # ${CMAKE_CURRENT_SOURCE_DIR}/src/mil-std-1553b/win32/WDMTMKv2.cpp
    )
    set(FORMS
        ${CMAKE_CURRENT_SOURCE_DIR}/ui/MainWindow.ui
        ${CMAKE_CURRENT_SOURCE_DIR}/ui/DriverSettingsDialog.ui
        ${CMAKE_CURRENT_SOURCE_DIR}/ui/InterfaceParamenetsDialog.ui
        ${CMAKE_CURRENT_SOURCE_DIR}/ui/SerialMonitorWindow.ui
    )
    set(HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/service/UndoBlocker.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs/DriverSettingsDialog.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs/InterfaceParamenetsDialog.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/serial/SerialTransfer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/serial/SerialMonitorWindow.h
        # ${CMAKE_CURRENT_SOURCE_DIR}/src/mil-std-1553b/win32/WDMTMKv2.h
    )
endif()

qt5_wrap_ui(UI_HEADERS ${FORMS})
qt5_add_resources(RESOURCES resources.qrc)

add_executable(${PROJECT_NAME}
    ${WIN32_OPTION}
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
    ${UI_HEADERS}
    ${RESOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/serial
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mil-std-1553b/win32
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mil-std-1553b/unix
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
    ${CMAKE_CURRENT_SOURCE_DIR}/docs
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::SerialPort
    Qt${QT_VERSION_MAJOR}::Concurrent
)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/mil-std-1553b-usb-terminal
)
