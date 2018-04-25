## Pok3rTool CMakeLists.txt
cmake_minimum_required(VERSION 3.0)

project(Pok3rTool)

add_subdirectory(libchaos)
add_subdirectory(rawhid)

### =================== SOURCES =================== ###

set(POK3RLIB_SOURCES
    kbscan.h
    kbscan.cpp

    kbproto.h
    kbproto.cpp
    proto_pok3r.h
    proto_pok3r.cpp
    proto_cykb.h
    proto_cykb.cpp
    proto_qmk.h
    proto_qmk.cpp

    keycodes.h
    keymap.h
    keymap.cpp

    updatepackage.h
    updatepackage.cpp
)

set(POK3RTOOL_SOURCES
    main.cpp
)

set(FILES
    README.md
    99-pok3r.rules
    test.sh
)

### =================== BUILD =================== ###

add_custom_target(pok3rtool-dummy SOURCES ${FILES})

# Pok3rLib
add_library(pok3rlib SHARED ${POK3RLIB_SOURCES})
set_property(TARGET pok3rlib PROPERTY CXX_STANDARD 11)
target_include_directories(pok3rlib PRIVATE ${LIBUSB_1_INCLUDE_DIRECTORIES})
target_compile_definitions(pok3rlib PRIVATE ${LIBUSB_1_DEFINITIONS})
target_link_libraries(pok3rlib chaos-shared rawhid ${LIBUSB_1_LIBRARIES})

# Pok3rTool
add_executable(pok3rtool ${POK3RTOOL_SOURCES})
set_property(TARGET pok3rtool PROPERTY CXX_STANDARD 11)
target_link_libraries(pok3rtool chaos-shared pok3rlib)

if(WIN32)
    add_custom_command(TARGET pok3rtool POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:chaos-shared>"
            "$<TARGET_FILE_DIR:pok3rtool>"
    )
endif()

if(MINGW)
    # Apparently Qt's mingw doesn't come with some static libraries
    get_filename_component(MINGW_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
    add_custom_command(TARGET pok3rtool POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_DIR}/../i686-w64-mingw32/lib/libgcc_s_dw2-1.dll"
            "$<TARGET_FILE_DIR:pok3rtool>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_DIR}/../i686-w64-mingw32/lib/libstdc++-6.dll"
            "$<TARGET_FILE_DIR:pok3rtool>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_DIR}/../i686-w64-mingw32/lib/libwinpthread-1.dll"
            "$<TARGET_FILE_DIR:pok3rtool>"
    )
endif()
