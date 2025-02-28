cmake_minimum_required(VERSION 3.30)
project(alayavim)
set(CMAKE_CXX_STANDARD 17)

include_directories(${PROJECT_SOURCE_DIR}/include)
add_executable(alayavim src/main.cpp src/core.cpp src/filemanager.cpp
        src/utility.cpp)