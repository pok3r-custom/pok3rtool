CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(rawhid)

### =================== SOURCES =================== ###

SET(SOURCES
    hid.h
    hiddevice.h
    hiddevice.cpp
)

IF(CMAKE_SYSTEM_NAME MATCHES "Windows")
    SET(SOURCES ${SOURCES}
        hid_WINDOWS.c
    )
ELSE()
    SET(SOURCES ${SOURCES}
        hid_libusb.c
    )
    SET(LIBUSB_1_FIND_REQUIRED TRUE)
    INCLUDE(../FindLibUSB-1.0.cmake)
ENDIF()


### =================== CONFIG =================== ###

IF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    # Find IOKit on Mac OS
    FIND_LIBRARY(IOKIT_LIBRARY IOKit REQUIRED)
    FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
ENDIF()

### =================== BUILD =================== ###

ADD_LIBRARY(rawhid STATIC ${SOURCES})
SET_PROPERTY(TARGET rawhid PROPERTY CXX_STANDARD 11)
TARGET_LINK_LIBRARIES(rawhid PRIVATE chaos-static)
TARGET_COMPILE_OPTIONS(rawhid PRIVATE
    $<$<COMPILE_LANGUAGE:C>:
        -std=gnu99
    >
)

IF(CMAKE_SYSTEM_NAME MATCHES "Windows")
    # Windows
    TARGET_LINK_LIBRARIES(rawhid PRIVATE hid setupapi)
ELSE()
    # macOS/Linux/BSD
    TARGET_INCLUDE_DIRECTORIES(rawhid PRIVATE ${LIBUSB_1_INCLUDE_DIRS})
    TARGET_LINK_LIBRARIES(rawhid PRIVATE ${LIBUSB_1_LIBRARIES})
    IF(NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
        TARGET_LINK_LIBRARIES(rawhid PRIVATE udev)
    ENDIF()
ENDIF()


# install
install(TARGETS rawhid DESTINATION lib)
