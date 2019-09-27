cmake_minimum_required(VERSION 3.12.0)
project(Tests)

file(GLOB_RECURSE SRCS *.cpp *.pb.cc)
file(GLOB_RECURSE HDRS *.h *.pb.h)

add_executable(${PROJECT_NAME} ${SRCS} ${HDRS})

target_link_libraries(${PROJECT_NAME} PRIVATE WickedEngine_Windows)
