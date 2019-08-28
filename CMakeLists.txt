## Pok3rTool CMakeLists.txt
cmake_minimum_required(VERSION 3.0)

project(Pok3rTool)

add_subdirectory(libchaos)
add_subdirectory(rawhid)

set(JSON_BuildTests OFF CACHE BOOL "disable json tests")
set(JSON_MultipleHeaders ON CACHE BOOL "enable json multiple headers")
add_subdirectory(nlohmann_json)

### =================== SOURCES =================== ###

set(GEN_KEYMAPS_HEADER ${CMAKE_CURRENT_BINARY_DIR}/gen_keymaps.h)

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

    ${GEN_KEYMAPS_HEADER}
)

set(POK3RTOOL_SOURCES
    main.cpp

    updatepackage.h
    updatepackage.cpp

    ${GEN_KEYMAPS_HEADER}
)

set(FILES
    README.md
    99-pok3r.rules
    test.sh
)

### =================== BUILD =================== ###

#add_custom_target(pok3rtool-dummy SOURCES ${FILES})

# Pok3rLib
add_library(pok3rlib STATIC ${POK3RLIB_SOURCES})
set_property(TARGET pok3rlib PROPERTY CXX_STANDARD 11)
target_link_libraries(pok3rlib chaos-static rawhid nlohmann_json ${LIBUSB_1_LIBRARIES})
#set_property(TARGET chaos-static PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(pok3rlib PRIVATE ${LIBUSB_1_INCLUDE_DIRECTORIES} ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(pok3rlib PRIVATE ${LIBUSB_1_DEFINITIONS})

# Pok3rTool
add_executable(pok3rtool ${POK3RTOOL_SOURCES})
set_property(TARGET pok3rtool PROPERTY CXX_STANDARD 11)
target_link_libraries(pok3rtool pok3rlib)
target_include_directories(pok3rtool PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if(WIN32)
    add_custom_command(TARGET pok3rtool POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:chaos-shared>"
            "$<TARGET_FILE_DIR:pok3rtool>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:rawhid>"
            "$<TARGET_FILE_DIR:pok3rtool>"
    )
endif()

# install tool and lib
install(TARGETS pok3rtool DESTINATION bin)
install(TARGETS pok3rlib DESTINATION lib)

if(MINGW)
    # Apparently Qt's mingw doesn't come with some static libraries
    get_filename_component(MINGW_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
    set(MINGW_LIBS_DIR "${MINGW_DIR}/../x86_64-w64-mingw32/lib")
    set(MINGW_LIBGCC "${MINGW_LIBS_DIR}/libgcc_s_seh-1.dll")
    set(MINGW_LIBSTD "${MINGW_LIBS_DIR}/libstdc++-6.dll")
    set(MINGW_LIBWPT "${MINGW_LIBS_DIR}/libwinpthread-1.dll")

    add_custom_command(TARGET pok3rtool POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_LIBGCC}"
            "$<TARGET_FILE_DIR:pok3rtool>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_LIBSTD}"
            "$<TARGET_FILE_DIR:pok3rtool>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_LIBWPT}"
            "$<TARGET_FILE_DIR:pok3rtool>"
    )

    # intall mingw libraries
    install(FILES "${MINGW_LIBGCC}" DESTINATION lib)
    install(FILES "${MINGW_LIBSTD}" DESTINATION lib)
    install(FILES "${MINGW_LIBWPT}" DESTINATION lib)
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
        file(APPEND ${output} "static const unsigned char ${filename}[] = {${filedata}};\nstatic const unsigned ${filename}_size = sizeof(${filename});\n")
        set(filenames "${filenames}${filename},")
        set(filename_sizes "${filename_sizes}${filename}_size,")
    endforeach()

    file(APPEND ${output} "static const unsigned char *${name}[] = {${filenames}};\nstatic const unsigned ${name}_sizes[] = {${filename_sizes}};\nstatic const unsigned ${name}_size = sizeof(${name}_sizes);")
endfunction()

create_resources("keymaps" "${GEN_KEYMAPS_HEADER}" "keymaps_json")
