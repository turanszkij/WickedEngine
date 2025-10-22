project(Template)

file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "src/*.cpp" "src/*.h" "src/*.hpp")

add_executable(${PROJECT_NAME} ${SourceFiles})

target_link_libraries(${PROJECT_NAME} WickedEngine_Windows)

if(WIN32)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
    if(MSVC)
        add_compile_options(/MP)
    endif()
endif()

