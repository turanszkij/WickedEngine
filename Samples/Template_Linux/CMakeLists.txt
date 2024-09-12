cmake_minimum_required(VERSION 3.8)
project(Template_Linux)

# if the engine is installed system-wise,
# you can set INSTALLED_ENGINE to ON
# moving your project folder wherever you want
set(INSTALLED_ENGINE OFF)
# you may also want to remove the copy mechanism of the startup.lua file

if (${INSTALLED_ENGINE})
    find_package(WickedEngine REQUIRED)
endif()

set(SOURCE_FILES
    main.cpp
    stdafx.h
)

set(LIB_DXCOMPILER "libdxcompiler.so")

add_executable(Template_Linux ${SOURCE_FILES})

target_link_libraries(Template_Linux PUBLIC
    # These are cmake generator expressions. They seem more dauting than they are
    # Basically if the variable is ON it will give WickedEngine,
    # if it's off, it will give WickedEngine::WickedEngine
    $<$<NOT:$<BOOL:${INSTALLED_ENGINE}>>:WickedEngine>
    $<$<BOOL:${INSTALLED_ENGINE}>:WickedEngine::WickedEngine>
)

if (${INSTALLED_ENGINE})
    get_property(LIBDXCOMPILER_PATH
        TARGET WickedEngine::dxcompiler
        PROPERTY IMPORTED_LOCATION)
    get_filename_component(WICKED_LIBFOLDER ${LIBDXCOMPILER_PATH} DIRECTORY)
    set(STARTUP_LUA "${WICKED_LIBFOLDER}/Editor/startup.lua")
else()
    set(LIBDXCOMPILER_PATH "${WICKED_ROOT_DIR}/WickedEngine/${LIB_DXCOMPILER}")
    set(STARTUP_LUA "${WICKED_ROOT_DIR}/Editor/startup.lua")
endif()
message("libdxcompiler found at ${LIBDXCOMPILER_PATH}")
message("startup lua found at ${STARTUP_LUA}")

add_custom_command(
    TARGET Template_Linux POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBDXCOMPILER_PATH} ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${STARTUP_LUA} ${CMAKE_CURRENT_BINARY_DIR}/startup.lua
)
