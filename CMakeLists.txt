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
    set(MINGW_LIBGCC "${MINGW_DIR}/../i686-w64-mingw32/lib/libgcc_s_dw2-1.dll")
    set(MINGW_LIBSTD "${MINGW_DIR}/../i686-w64-mingw32/lib/libstdc++-6.dll")
    set(MINGW_LIBWINPT "${MINGW_DIR}/../i686-w64-mingw32/lib/libwinpthread-1.dll")
    add_custom_command(TARGET pok3rtool POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_LIBGCC}"
            "$<TARGET_FILE_DIR:pok3rtool>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_LIBSTD}"
            "$<TARGET_FILE_DIR:pok3rtool>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_LIBWINPT}"
            "$<TARGET_FILE_DIR:pok3rtool>"
    )
endif()

# install tool and lib
install(TARGETS pok3rtool DESTINATION bin)
install(TARGETS pok3rlib DESTINATION lib)

if(MINGW)
    install(FILES "${MINGW_LIBGCC}" DESTINATION lib)
    install(FILES "${MINGW_LIBSTD}" DESTINATION lib)
    install(FILES "${LIB_LIBWINPT}" DESTINATION lib)
endif()

# Creates C resources file from files in given directory
function(create_resources dir output name)
    # Create empty output file
    file(WRITE ${output} "")
    # Collect input files
    file(GLOB bins ${dir}/*)

    # Iterate through input files
    foreach(bin ${bins})
        # Get short filename
        string(REGEX MATCH "([^/]+)$" filename ${bin})
        # Replace filename spaces & extension separator for C compatibility
        string(REGEX REPLACE "\\.| |-" "_" filename ${filename})
        # Read hex data from file
        file(READ ${bin} filedata HEX)
        # Convert hex data for C compatibility
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})
        # Append data to output file
        file(APPEND ${output} "const unsigned char ${filename}[] = {${filedata}};\nconst unsigned ${filename}_size = sizeof(${filename});\n")
        set(filenames "${filenames}${filename},")
        set(filename_sizes "${filename_sizes}${filename}_size,")
    endforeach()

    file(APPEND ${output} "const unsigned char *${name}[] = {${filenames}};\nconst unsigned ${name}_sizes[] = {${filename_sizes}};\nconst unsigned ${name}_size = sizeof(${name}_sizes);")
endfunction()

create_resources("keymaps" "gen_keymaps.h" "keymaps_json")
