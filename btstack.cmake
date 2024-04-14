cmake_minimum_required(VERSION 3.13.0)

find_package(Git REQUIRED)
find_package(Python REQUIRED)

execute_process(
        COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND_ECHO STDOUT
)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/external/components)
execute_process(
        COMMAND ${CMAKE_COMMAND} -E env IDF_PATH=${CMAKE_SOURCE_DIR}/external ${Python_EXECUTABLE} ./integrate_btstack.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external/btstack/port/esp32
)
message("btstack initialized")
