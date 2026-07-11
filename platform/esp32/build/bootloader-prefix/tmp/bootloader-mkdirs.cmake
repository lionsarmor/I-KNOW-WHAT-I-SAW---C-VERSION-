# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/legion/esp/esp-idf/components/bootloader/subproject"
  "/home/legion/Desktop/I KNOW WHAT I SAW - C VERSION /platform/esp32/build/bootloader"
  "/home/legion/Desktop/I KNOW WHAT I SAW - C VERSION /platform/esp32/build/bootloader-prefix"
  "/home/legion/Desktop/I KNOW WHAT I SAW - C VERSION /platform/esp32/build/bootloader-prefix/tmp"
  "/home/legion/Desktop/I KNOW WHAT I SAW - C VERSION /platform/esp32/build/bootloader-prefix/src/bootloader-stamp"
  "/home/legion/Desktop/I KNOW WHAT I SAW - C VERSION /platform/esp32/build/bootloader-prefix/src"
  "/home/legion/Desktop/I KNOW WHAT I SAW - C VERSION /platform/esp32/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/legion/Desktop/I KNOW WHAT I SAW - C VERSION /platform/esp32/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/legion/Desktop/I KNOW WHAT I SAW - C VERSION /platform/esp32/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
