cmake_minimum_required(VERSION 3.19)

project(Editor)

# workaround for source dir containing regex meta characters
string(REGEX REPLACE "([.+?*])" "\\\\\\1" SDIR "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS *.cpp)
list(FILTER SOURCE_FILES EXCLUDE REGEX ${SDIR}/main_.*)
list(FILTER SOURCE_FILES EXCLUDE REGEX ${SDIR}/stdafx.*)
list(APPEND SOURCE_FILES main_${PLATFORM}.cpp)

if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCE_FILES})
else()
    add_executable(${PROJECT_NAME} ${SOURCE_FILES})
endif()

# Setup WickedEngine
setup_wicked_app(${PROJECT_NAME})

# Copy fonts directory
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E ${COPY_OR_SYMLINK_DIR_CMD}
        ${CMAKE_CURRENT_SOURCE_DIR}/fonts
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/fonts
    COMMENT "Copying fonts to output directory for ${PROJECT_NAME}"
)

if (WIN32)
    list (APPEND SOURCE_FILES
        Editor.rc
    )

    set_property(TARGET Editor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
else ()
    # needed for proper names in crash stacktrace
    set_target_properties(${PROJECT_NAME}
        PROPERTIES
        ENABLE_EXPORTS ON
    )
endif ()

if(WICKED_USE_IPO)
    set_target_properties(Editor PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION ON
        INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF
    )
endif()

target_precompile_headers(Editor PRIVATE "stdafx.h")

# Needed for terrain system
add_dependencies(Editor Content)

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

