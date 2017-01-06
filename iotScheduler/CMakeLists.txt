cmake_minimum_required(VERSION 2.8.9)
project(iotScheduler)

set(CMAKE_BUILD_TYPE Debug)
#Bring the headers, such as Student.h into the project
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)
#Can manually add the sources using the set command as follows:
#set(SOURCES src/mainapp.cpp src/Student.cpp)

#However, the file(GLOB...) allows for wildcard additions:
file(GLOB SOURCES "src/*.c")
add_executable(iotScheduler ${SOURCES})
target_link_libraries (iotScheduler mosquitto)
target_link_libraries (iotScheduler jansson)

install(TARGETS iotScheduler DESTINATION bin)
