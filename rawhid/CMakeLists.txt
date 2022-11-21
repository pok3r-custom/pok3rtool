CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(rawhid)

### =================== SOURCES =================== ###

SET(SOURCES
    hid.h
    hid_libusb.c
    hiddevice.h
    hiddevice.cpp
)

### =================== CONFIG =================== ###

IF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    # Find IOKit on Mac OS
    FIND_LIBRARY(IOKIT_LIBRARY IOKit REQUIRED)
    FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
ENDIF()

SET(LIBUSB_1_FIND_REQUIRED TRUE)

### =================== BUILD =================== ###

INCLUDE(
    ../FindLibUSB-1.0.cmake
)

ADD_LIBRARY(rawhid STATIC ${SOURCES})
SET_PROPERTY(TARGET rawhid PROPERTY CXX_STANDARD 11)
TARGET_LINK_LIBRARIES(rawhid PRIVATE chaos-static)
TARGET_COMPILE_OPTIONS(rawhid PRIVATE
    $<$<COMPILE_LANGUAGE:C>:
        -std=gnu99
    >
)

IF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    # Mac OS
    TARGET_LINK_LIBRARIES(rawhid PRIVATE ${IOKIT_LIBRARY} ${COREFOUNDATION_LIBRARY})
ENDIF()

# Linux/BSD/Windows/macOS
TARGET_INCLUDE_DIRECTORIES(rawhid PRIVATE ${LIBUSB_1_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(rawhid PRIVATE ${LIBUSB_1_LIBRARIES})

# install
install(TARGETS rawhid DESTINATION lib)
