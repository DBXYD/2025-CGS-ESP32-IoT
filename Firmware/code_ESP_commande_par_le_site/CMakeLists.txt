# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
set(PARTITION_TABLE_CUSTOM_FILENAME "${CMAKE_SOURCE_DIR}/partitions.csv")
set(PARTITION_TABLE_EMBED_TXT yes)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(code_ESP_commande_par_le_site)
