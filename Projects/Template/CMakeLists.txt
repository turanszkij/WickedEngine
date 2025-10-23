cmake_minimum_required(VERSION 3.19)

project(Template)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h" "src/*.hpp")

if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCE_FILES})
else()
    add_executable(${PROJECT_NAME} ${SOURCE_FILES})
endif()

setup_wicked_app(${PROJECT_NAME})