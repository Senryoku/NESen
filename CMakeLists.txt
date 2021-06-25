cmake_minimum_required(VERSION 3.0)
project(NESen VERSION 0.2)

if(CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()

if (MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

include_directories("${PROJECT_BINARY_DIR}")
include_directories("${CMAKE_SOURCE_DIR}/src")


# Define sources and executable
set(EXECUTABLE_NAME "NESen")
aux_source_directory("src/core" SOURCES)
aux_source_directory("src/tools" SOURCES)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})

#dear imgui
set(ImGui-SFML_DIR "ext/imgui-sfml")
include_directories(${ImGui-SFML_DIR})
set(IMGUI_SOURCES
	${ImGui-SFML_DIR}/imgui.cpp
	${ImGui-SFML_DIR}/imgui_widgets.cpp
	${ImGui-SFML_DIR}/imgui_draw.cpp
	${ImGui-SFML_DIR}/imgui-SFML.cpp
)

add_executable(${EXECUTABLE_NAME} ${IMGUI_SOURCES} ${SOURCES} src/NESen.cpp)
add_executable(cpu_nestest ${SOURCES} src/tests/cpu_nestest.cpp)
add_executable(instr_test ${SOURCES} src/tests/instr_test.cpp)

set_property(TARGET ${EXECUTABLE_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET cpu_nestest PROPERTY CXX_STANDARD 20)
set_property(TARGET instr_test PROPERTY CXX_STANDARD 20)

# Detect and add SFML
set(SFML_DIR "ext/SFML/")
find_package(SFML COMPONENTS system window graphics audio REQUIRED)
if(SFML_FOUND)
  include_directories(${SFML_DIR}/include)
  target_link_libraries(${EXECUTABLE_NAME} sfml-system sfml-window sfml-graphics sfml-audio)
  target_link_libraries(instr_test sfml-system sfml-window sfml-graphics sfml-audio)
endif()

target_link_libraries(${EXECUTABLE_NAME} opengl32)
  
add_custom_target(run
    DEPENDS ${EXECUTABLE_NAME}
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/ext/SFML/lib/sfml-system-2.dll" $<TARGET_FILE_DIR:${EXECUTABLE_NAME}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/ext/SFML/lib/sfml-window-2.dll" $<TARGET_FILE_DIR:${EXECUTABLE_NAME}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/ext/SFML/lib/sfml-graphics-2.dll" $<TARGET_FILE_DIR:${EXECUTABLE_NAME}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/ext/SFML/lib/sfml-audio-2.dll" $<TARGET_FILE_DIR:${EXECUTABLE_NAME}>
    COMMAND ${EXECUTABLE_NAME}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(tests
    DEPENDS cpu_nestest
    COMMAND cpu_nestest
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(run_instr_test
    DEPENDS instr_test
    COMMAND instr_test
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
