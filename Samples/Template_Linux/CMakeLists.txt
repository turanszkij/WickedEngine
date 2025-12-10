cmake_minimum_required(VERSION 3.19)
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
)

set(LIB_DXCOMPILER "libdxcompiler.so")

add_executable(Template_Linux ${SOURCE_FILES})

target_link_libraries(Template_Linux PUBLIC
    $<IF:$<BOOL:${INSTALLED_ENGINE}>,WickedEngine::WickedEngine,WickedEngine>
)

if(WICKED_ENABLE_IPO)
set_target_properties(Template_Linux PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION ON
    INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF
)
endif()

if (${INSTALLED_ENGINE})
    get_property(LIBDXCOMPILER_PATH
        TARGET WickedEngine::dxcompiler
        PROPERTY IMPORTED_LOCATION)
    get_filename_component(WICKED_LIBFOLDER ${LIBDXCOMPILER_PATH} DIRECTORY)
else()
    set(LIBDXCOMPILER_PATH "${WICKED_ROOT_DIR}/WickedEngine/${LIB_DXCOMPILER}")
endif()
message("libdxcompiler found at ${LIBDXCOMPILER_PATH}")

add_custom_command(
    TARGET Template_Linux POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBDXCOMPILER_PATH} ${CMAKE_CURRENT_BINARY_DIR}
)
