cmake_minimum_required(VERSION 2.8.9)

project(iotserver)
set(CMAKE_BUILD_TYPE Debug)
option(LOG_ENABLE "Enable logging in file" OFF)

MESSAGE("${CMAKE_PROJECT_NAME}-${PROJECT_NAME}: PROJECT_SOURCE_DIR is ${PROJECT_SOURCE_DIR}")
MESSAGE("${CMAKE_PROJECT_NAME}-${PROJECT_NAME}: CMAKE_SOURCE_DIR is ${CMAKE_SOURCE_DIR}")

if(LOG_ENABLE)
  add_definitions(-DLOG_ENABLE)
endif()
#Bring the headers, such as Student.h into the project
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)
#Can manually add the sources using the set command as follows:
#set(SOURCES src/mainapp.cpp src/Student.cpp)

#However, the file(GLOB...) allows for wildcard additions:
file(GLOB SOURCES "src/*.c")
add_executable(iotserver ${SOURCES})
target_link_libraries (iotserver mosquitto)
target_link_libraries (iotserver jansson)

install(TARGETS iotserver DESTINATION bin)
