# pins/pins_esp32/CmakeLists.txt, 13.3.2026/pekka
#
# ESP-IDF build only: The second CMakeLists.txt in "pins_esp32" is responsible for controlling 
# the build process of the component and its integration into the overall project. 
# The component CMakeLists.txt file lists source files and registers the component to the build 
# system using idf_component_register.
#
# In ESP_IDF the directory name is the component name: The ESP-IDF build system identifies components 
# based on the name of the directory containing the CMakeLists.txt file. Here, a directory named 
# pins_esp32 is treated as the component pins_esp32.
#
# Since icoom code is placed in "code" and "extensions/dynamicio" folders for other build system, new directory 
# pins_esp32 holds the CmakeList.txt which compiles files in code and extensions directories.
#
cmake_minimum_required(VERSION 3.12)

# Project name, is development root folder name for the project.
set(E_PROJECT "pins")

# Set E_ROOT environment variable to point to the project root folder. 
# This is used in the CMakeLists.txt files to locate source files and include paths.
include("${COMPONENT_PATH}/../../eosal/osbuild/cmakedefs/eosal-root-path.txt")

# Include ESP32 specific setting for build.
include("$ENV{E_ROOT}/eosal/osbuild/cmakedefs/eosal-defs-espidf.txt")

# Set path to source files.
set(BACKBONE "$ENV{E_ROOT}/${E_PROJECT}/code")
set(EXTENSIONS "$ENV{E_ROOT}/${E_PROJECT}/extensions")

# Add source files common to all operating systems.
file(GLOB BB_CM_SOURCES "${BACKBONE}/common/*.c")
file(GLOB EX_CM_SOURCES "${EXTENSIONS}/*/common/*.c")

# Add ESP32 specific files. 
set(ENV{E_IMPL_DIR} $ENV{E_OS})
set(ENV{E_IMPLX_DIR} $ENV{E_OS})
file(GLOB BB_OS_SOURCES "${BACKBONE}/$ENV{E_IMPL_DIR}/*.c" "${BACKBONE}/$ENV{E_IMPL_DIR}/*.cpp")
file(GLOB EX_OS_SOURCES "${EXTENSIONS}/*/$ENV{E_IMPLX_DIR}/*.c" "${EXTENSIONS}/*/$ENV{E_IMPLX_DIR}/*.cpp")

# Merge the .c files as one list.
set(srcs ${BB_CM_SOURCES} ${BB_OS_SOURCES} ${EX_CM_SOURCES} ${EX_OS_SOURCES})

# The pins library depends on: 
set(requires "driver")
list(APPEND requires "esp_timer")
list(APPEND requires "esp_adc")
list(APPEND requires "esp_driver_gpio")
list(APPEND requires "esp_driver_ledc")
list(APPEND requires "eosal_esp32")
list(APPEND requires "iocom_esp32")

# Set include paths
set(includes $ENV{E_ROOT}/eosal)
list(APPEND includes $ENV{E_ROOT}/iocom)
list(APPEND includes $ENV{E_ROOT}/${E_PROJECT})

# Register eosal as ESP-IDF component.
idf_component_register(SRCS "${srcs}" 
    INCLUDE_DIRS "${includes}" 
    REQUIRES "${requires}" )

    # This results correct link order. 
idf_component_optional_requires(PRIVATE iocom_esp32)