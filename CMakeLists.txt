# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include_directories(main/headers)

execute_process(
        COMMAND bash -c ". $ENV{HOME}/esp/esp-idf/export.sh 1> /dev/null && echo $IDF_PATH"
        OUTPUT_VARIABLE IDF_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE IDF_PATH_RESULT
)

if(IDF_PATH_RESULT)
    message(FATAL_ERROR "Failed to set IDF_PATH")
endif()

set(ENV{IDF_PATH} ${IDF_PATH})
message("IDF_PATH is set to $ENV{IDF_PATH}")


include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(blank)
