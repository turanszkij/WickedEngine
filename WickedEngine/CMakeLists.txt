cmake_minimum_required(VERSION 3.12.0)

project(WickedEngine_Windows LANGUAGES C CXX)

file(GLOB_RECURSE SRCS *.cpp *.pb.cc *.c)
file(GLOB_RECURSE HDRS *.h *.pb.h)

add_library(${PROJECT_NAME} STATIC ${SRCS} ${HDRS})

target_include_directories(${PROJECT_NAME} 
	PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR} 
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/BULLET)

target_compile_definitions(${PROJECT_NAME} PUBLIC UNICODE UNICODE_)