cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME "vine")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "/usr/local/bin")
set(SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/vine.c")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -std=c99")

project(${PROJECT_NAME})

add_executable(vine ${SOURCE_FILES})
install(
    TARGETS "vine"
    DESTINATION "/usr/local/bin"
)
