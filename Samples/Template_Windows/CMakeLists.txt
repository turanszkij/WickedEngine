cmake_minimum_required(VERSION 3.19)

project(Template_Windows)

add_executable(${PROJECT_NAME} WIN32 main.cpp)

setup_wicked_app(${PROJECT_NAME})