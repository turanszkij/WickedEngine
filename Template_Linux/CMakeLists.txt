cmake_minimum_required(VERSION 3.8)
project(Template_Linux)

set (SOURCE_FILES
    main.cpp
    stdafx.h    
)

set(LIB_DXCOMPILER "libdxcompiler.so")

add_executable(Template_Linux ${SOURCE_FILES})

target_link_libraries(Template_Linux PUBLIC
    WickedEngine
)

add_custom_command(
    TARGET Template_Linux POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/WickedEngine/${LIB_DXCOMPILER} ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/Editor/startup.lua ${CMAKE_CURRENT_BINARY_DIR}/startup.lua
)