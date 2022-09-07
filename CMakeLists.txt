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

add_library(nesenlib STATIC ${SOURCES} ${TOOLS} ${HEADERS})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})

include_directories("ext")

# Dear Imgui
set(ImGui-SFML_DIR "ext/imgui-sfml")
include_directories(${ImGui-SFML_DIR})
set(IMGUI_SOURCES
	${ImGui-SFML_DIR}/imgui.cpp
	${ImGui-SFML_DIR}/imgui_widgets.cpp
	${ImGui-SFML_DIR}/imgui_draw.cpp
	${ImGui-SFML_DIR}/imgui-SFML.cpp
)

add_executable(${EXECUTABLE_NAME} ${IMGUI_SOURCES} src/NESen.cpp)
add_executable(cpu_nestest src/tests/cpu_nestest.cpp)
add_executable(instr_test src/tests/instr_test.cpp)
add_executable(harte_test src/tests/harte_test.cpp)

target_link_libraries(${EXECUTABLE_NAME} nesenlib)
target_link_libraries(cpu_nestest nesenlib)
target_link_libraries(instr_test nesenlib)
target_link_libraries(harte_test nesenlib)

set_property(TARGET ${EXECUTABLE_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET cpu_nestest PROPERTY CXX_STANDARD 20)
set_property(TARGET instr_test PROPERTY CXX_STANDARD 20)
set_property(TARGET harte_test PROPERTY CXX_STANDARD 20)

set_property(TARGET harte_test PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

set(BUILD_SHARED_LIBS  FALSE) # Statically link SFML
add_subdirectory("ext/SFML/")
target_include_directories(${EXECUTABLE_NAME} PUBLIC "ext/SFML/include")
target_link_libraries(${EXECUTABLE_NAME} sfml-system sfml-window sfml-graphics sfml-audio)

target_link_libraries(${EXECUTABLE_NAME} opengl32)

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
