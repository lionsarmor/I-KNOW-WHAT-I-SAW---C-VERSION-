# ESP-IDF project wrapper. Build from this directory with:
#   idf.py set-target esp32      (or esp32s3, esp32c3 = RISC-V, ...)
#   idf.py build flash monitor
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(iknowwhatisaw)
