cmake_minimum_required(VERSION 3.25)
project(TA1-USB-01-I VERSION 0.1 LANGUAGES CXX)

include(ProcessorCount)
ProcessorCount(N)
if(NOT N EQUAL 0)
    set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Building for Linux")
    set(QT_PATH /usr/lib/x86_64-linux-gnu/cmake/Qt5)

    find_package(Qt5 COMPONENTS Core Gui Widgets SerialPort)

    set(SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/serial/UartTransfer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mil-std-1553b/unix/ltmk.c
    )

    set(FORMS
    )

    set(HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/serial/UartTransfer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mil-std-1553b/unix/ltmk.h
    )

    add_compile_options(-finput-charset=UTF-8)
    add_link_options(-Wl,-rpath-link,/usr/lib)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "Building for Windows")
    find_package(Qt5 COMPONENTS Core Gui Widgets SerialPort)

    if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        enable_language("RC")
        set (WIN32_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/icon.rc)
    endif()

    set(SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/serial/UartTransfer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mil-std-1553b/win32/WDMTMKv2.cpp
    )

    set(FORMS
    )

    set(HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/serial/UartTransfer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mil-std-1553b/win32/WDMTMKv2.h
    )
endif()



qt5_wrap_ui(UI_HEADERS ${FORMS})
qt5_add_resources(RESOURCES resources.qrc)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${FORMS} ${UI_HEADERS} ${RESOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/serial
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mil-std-1553b/win32
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mil-std-1553b/unix
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
    ${CMAKE_CURRENT_SOURCE_DIR}/docs
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::SerialPort
)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)
