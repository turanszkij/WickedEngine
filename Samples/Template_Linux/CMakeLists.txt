cmake_minimum_required(VERSION 3.19)
project(Template_Linux)

set(SOURCE_FILES
    main.cpp
)

set(LIB_DXCOMPILER "libdxcompiler.so")

add_executable(Template_Linux ${SOURCE_FILES})

target_link_libraries(Template_Linux PUBLIC
    WickedEngine
)

if(WICKED_ENABLE_IPO)
set_target_properties(Template_Linux PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION ON
    INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF
)
endif()

set(LIBDXCOMPILER_PATH "${WICKED_ROOT_DIR}/WickedEngine/${LIB_DXCOMPILER}")
message(STATUS "libdxcompiler found at ${LIBDXCOMPILER_PATH}")

add_custom_command(
    TARGET Template_Linux POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBDXCOMPILER_PATH} ${CMAKE_CURRENT_BINARY_DIR}
)
