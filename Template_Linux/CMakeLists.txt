cmake_minimum_required(VERSION 3.8)
project(Template_Linux)

find_package(WickedEngine REQUIRED)

set (SOURCE_FILES
    main.cpp
    stdafx.cpp
    stdafx.h    
    Icon.h
    App.cpp
    App.h
)

add_executable(Template_Linux ${SOURCE_FILES})

target_link_libraries(Template_Linux PUBLIC
    WickedEngine::WickedEngine
)