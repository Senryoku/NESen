cmake_minimum_required(VERSION 2.6)
project(NESen)

if(CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(NESen 1)
set(NESen 0)
include_directories("${PROJECT_BINARY_DIR}")
include_directories("${CMAKE_SOURCE_DIR}/src")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} --std=c++11 -Wall")

# Define sources and executable
set(EXECUTABLE_NAME "NESen")
set(SOURCES
	src/CPU.cpp
	src/Common.cpp
	src/NESen.cpp
)
add_executable(${EXECUTABLE_NAME} ${SOURCES})

# Detect and add SFML
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})
#Find any version 2.X of SFML
#See the FindSFML.cmake file for additional details and instructions
find_package(SFML 2 REQUIRED system window graphics)
if(SFML_FOUND)
  include_directories(${SFML_INCLUDE_DIR})
  target_link_libraries(${EXECUTABLE_NAME} ${SFML_LIBRARIES})
endif()

# Install target
install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)

add_custom_target(run
    COMMAND NESen
    DEPENDS NESen
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)