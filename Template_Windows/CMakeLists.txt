cmake_minimum_required(VERSION 3.8)
project(Template_Windows)

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS "*.cpp" "*.h")

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set(LIB_DXCOMPILER "dxcompiler.dll")

list(APPEND SOURCE_FILES
${CMAKE_CURRENT_SOURCE_DIR}/TemplateWindows.rc
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC
    WickedEngine_Windows
)

set_target_properties(${PROJECT_NAME} PROPERTIES
WIN32_EXECUTABLE YES
)

set(PLATFORM "Windows")
add_compile_definitions(WIN32 _WINDOWS)

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
