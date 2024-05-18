# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/hoanganh/esp/esp-idf-v5.2.1/components/bootloader/subproject"
  "/home/hoanganh/Downloads/Node_RS485/ESP32C3/build/bootloader"
  "/home/hoanganh/Downloads/Node_RS485/ESP32C3/build/bootloader-prefix"
  "/home/hoanganh/Downloads/Node_RS485/ESP32C3/build/bootloader-prefix/tmp"
  "/home/hoanganh/Downloads/Node_RS485/ESP32C3/build/bootloader-prefix/src/bootloader-stamp"
  "/home/hoanganh/Downloads/Node_RS485/ESP32C3/build/bootloader-prefix/src"
  "/home/hoanganh/Downloads/Node_RS485/ESP32C3/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/hoanganh/Downloads/Node_RS485/ESP32C3/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/hoanganh/Downloads/Node_RS485/ESP32C3/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
