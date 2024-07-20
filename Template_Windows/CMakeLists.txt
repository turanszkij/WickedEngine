cmake_minimum_required(VERSION 3.8)
project(Template_Windows)

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS "*.cpp" "*.h")

set(LIB_DXCOMPILER "libdxcompiler.dll")

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC
    # These are cmake generator expressions. They seem more dauting than they are
    # Basically if the variable is ON it will give WickedEngine,
    # if it's off, it will give WickedEngine::WickedEngine
    $<$<NOT:$<BOOL:${INSTALLED_ENGINE}>>:WickedEngine>
    $<$<BOOL:${INSTALLED_ENGINE}>:WickedEngine::WickedEngine>
)


set(LIBDXCOMPILER_PATH "${WICKED_ROOT_DIR}/WickedEngine/${LIB_DXCOMPILER}")
set(STARTUP_LUA "${WICKED_ROOT_DIR}/Editor/startup.lua")
message("libdxcompiler found at ${LIBDXCOMPILER_PATH}")
message("startup lua found at ${STARTUP_LUA}")

# this may not work on windows
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBDXCOMPILER_PATH} ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${STARTUP_LUA} ${CMAKE_CURRENT_BINARY_DIR}/startup.lua
)
