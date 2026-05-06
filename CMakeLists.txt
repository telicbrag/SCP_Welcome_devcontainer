# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.10)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(scp_welcome)

#idf_component_register(SRCS
#        "scp_welcome_main.c"
#        "EepromInfo.c"
#        INCLUDE_DIRS "."
#        REQUIRES esp_driver_i2c esp_flash)
