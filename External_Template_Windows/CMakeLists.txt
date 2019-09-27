cmake_minimum_required(VERSION 3.12.0)
project(External_Template_Windows)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(WickedEngine REQUIRED)

file(GLOB_RECURSE SRCS *.cpp *.pb.cc *.rc)
file(GLOB_RECURSE HDRS *.h *.pb.h)

add_executable(${PROJECT_NAME} WIN32 ${SRCS} ${HDRS})

target_link_libraries(${PROJECT_NAME} PRIVATE ${WICKEDENGINE_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${WICKEDENGINE_INCLUDE_DIR})
