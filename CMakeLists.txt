
cmake_minimum_required(VERSION 3.19.3)

project(COMMAND_LINE_TEXT_EDITOR)

include(cmake/CompilerWarnings.cmake)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(
    SRC_FILES
    ${SRC_DIR}/console.h
    ${SRC_DIR}/console.cpp
    ${SRC_DIR}/console_text_editor.h
    ${SRC_DIR}/console_text_editor.cpp
    ${SRC_DIR}/text_editor.h
    ${SRC_DIR}/text_editor.cpp
    ${SRC_DIR}/main.cpp
)

set(EXECUTABLE_NAME "e")

add_executable(${EXECUTABLE_NAME} ${SRC_FILES})

set_project_warnings(${EXECUTABLE_NAME} OFF)

set_target_properties(
    ${EXECUTABLE_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
