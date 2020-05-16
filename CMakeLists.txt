cmake_minimum_required(VERSION 3.13)
project(WickedEngine)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE WK_SOURCES "WickedEngine/*.cpp")
file(GLOB_RECURSE WK_SOURCES_C "WickedEngine/*.c")
file(GLOB_RECURSE WK_HEADERS "WickedEngine/*.h")


set (WK_INC_DIRS "")
foreach (_headerFile ${WK_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND WK_INC_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES WK_INC_DIRS)

add_definitions(-DNDEBUG -DCMAKE_GENERATOR_PLATFORM=x64)
add_executable(WickedEngine ${WK_SOURCES} ${WK_SOURCES_C})
target_include_directories(WickedEngine PRIVATE ${WK_INC_DIRS})
