# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/janus/esp/esp-idf/components/bootloader/subproject"
  "/home/janus/Desktop/ENSEA/Stages/1A/Projet/2025-CGS-ESP32-IoT/Firmware/PCB_Affichage_GoogleCalendar_et_BarLED/build/bootloader"
  "/home/janus/Desktop/ENSEA/Stages/1A/Projet/2025-CGS-ESP32-IoT/Firmware/PCB_Affichage_GoogleCalendar_et_BarLED/build/bootloader-prefix"
  "/home/janus/Desktop/ENSEA/Stages/1A/Projet/2025-CGS-ESP32-IoT/Firmware/PCB_Affichage_GoogleCalendar_et_BarLED/build/bootloader-prefix/tmp"
  "/home/janus/Desktop/ENSEA/Stages/1A/Projet/2025-CGS-ESP32-IoT/Firmware/PCB_Affichage_GoogleCalendar_et_BarLED/build/bootloader-prefix/src/bootloader-stamp"
  "/home/janus/Desktop/ENSEA/Stages/1A/Projet/2025-CGS-ESP32-IoT/Firmware/PCB_Affichage_GoogleCalendar_et_BarLED/build/bootloader-prefix/src"
  "/home/janus/Desktop/ENSEA/Stages/1A/Projet/2025-CGS-ESP32-IoT/Firmware/PCB_Affichage_GoogleCalendar_et_BarLED/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/janus/Desktop/ENSEA/Stages/1A/Projet/2025-CGS-ESP32-IoT/Firmware/PCB_Affichage_GoogleCalendar_et_BarLED/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/janus/Desktop/ENSEA/Stages/1A/Projet/2025-CGS-ESP32-IoT/Firmware/PCB_Affichage_GoogleCalendar_et_BarLED/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
