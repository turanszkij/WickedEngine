cmake_minimum_required(VERSION 3.12.0)
project(Editor)

file(GLOB_RECURSE SRCS *.cpp *.pb.cc *.rc)
file(GLOB_RECURSE HDRS *.h *.pb.h)

add_executable(${PROJECT_NAME} WIN32 ${SRCS} ${HDRS})

target_link_libraries(${PROJECT_NAME} PRIVATE WickedEngine_Windows)
