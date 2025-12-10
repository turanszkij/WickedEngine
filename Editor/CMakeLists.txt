cmake_minimum_required(VERSION 3.19)

# workaround for source dir containing regex meta characters
string(REGEX REPLACE "([.+?*])" "\\\\\\1" SDIR "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS *.cpp)
list(FILTER SOURCE_FILES EXCLUDE REGEX ${SDIR}/main_.*)
list(FILTER SOURCE_FILES EXCLUDE REGEX ${SDIR}/stdafx.*)
list(APPEND SOURCE_FILES main_${PLATFORM}.cpp)


if (WIN32)
    list (APPEND SOURCE_FILES
        Editor.rc
        Editor.manifest
    )

    add_executable(Editor WIN32 ${SOURCE_FILES})

    target_link_libraries(Editor PUBLIC
        WickedEngine
    )

    # Work around clang bug, see
    # https://github.com/llvm/llvm-project/issues/120394
    # https://gitlab.kitware.com/cmake/cmake/-/issues/27064
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_link_options(Editor PRIVATE "SHELL:-Xlinker /manifestuac:no")
    endif()

    set_property(TARGET Editor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set(LIB_DXCOMPILER "dxcompiler.dll")
else ()
    add_executable(Editor ${SOURCE_FILES})

    target_link_libraries(Editor PUBLIC
        WickedEngine
    )
    set(LIB_DXCOMPILER "libdxcompiler.so")

    # needed for proper names in crash stacktrace
    set_target_properties(Editor
        PROPERTIES
        ENABLE_EXPORTS ON
    )

endif ()

if(WICKED_ENABLE_IPO)
    set_target_properties(Editor PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION ON
        INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF
    )
endif()

target_precompile_headers(Editor PRIVATE "stdafx.h")

# Needed for terrain system
add_dependencies(Editor Content)

# Copy content to build folder:
add_custom_command(
    TARGET Editor POST_BUILD
    # Copy shader compiler library in the source folder
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${WICKED_ROOT_DIR}/WickedEngine/${LIB_DXCOMPILER} ${CMAKE_CURRENT_BINARY_DIR}
    #COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/languages ${CMAKE_CURRENT_BINARY_DIR}/languages
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/fonts ${CMAKE_CURRENT_BINARY_DIR}/fonts
)

include(GNUInstallDirs)
set(EDITOR_INSTALL_FOLDER "${CMAKE_INSTALL_LIBDIR}/WickedEngine/Editor")

# Editor executable
install(TARGETS Editor RUNTIME DESTINATION ${EDITOR_INSTALL_FOLDER})

# install editor assets
install(DIRECTORY
        #"${CMAKE_CURRENT_SOURCE_DIR}/languages"
        "${CMAKE_CURRENT_SOURCE_DIR}/fonts"
        DESTINATION ${EDITOR_INSTALL_FOLDER})
# optional .ini .ico .lua
install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/config.ini
        ${CMAKE_CURRENT_SOURCE_DIR}/wicked.ico
        ${CMAKE_CURRENT_SOURCE_DIR}/startup.lua
        DESTINATION ${EDITOR_INSTALL_FOLDER})
