# pins/code/CmakeLists.txt, 4.9.2021/pekka
#
# ESP-IDF build only: The second CMakeLists.txt in src_dir (code) sis responsible for controlling 
# the build process of the component and its integration into the overall project. 
# The component CMakeLists.txt file lists source files and registers the component to the build 
# system using idf_component_register.
#
cmake_minimum_required(VERSION 3.16.0)

# Project name, development root folder, and operatig system as used in eosal.
set(E_PROJECT "pins")
set(ENV{E_ROOT} "/coderoot")
set(ENV{E_OS} "esp32")

# Set path to source files.
set(BACKBONE "$ENV{E_ROOT}/${E_PROJECT}/code")
set(EXTENSIONS "$ENV{E_ROOT}/${E_PROJECT}/extensions")

# Add main headers.
# set(MAIN_HEADERS "$ENV{E_ROOT}/${E_PROJECT}/${E_PROJECT}.h" "$ENV{E_ROOT}/${E_PROJECT}/${E_PROJECT}x.h")

# Add source files common to all operating systems.
file(GLOB BB_CM_SOURCES "${BACKBONE}/common/*.c")
file(GLOB EX_CM_SOURCES "${EXTENSIONS}/*/common/*.c")
# file(GLOB BB_CM_HEADERS "${BACKBONE}/common/*.h")
# file(GLOB EX_CM_HEADERS "${EXTENSIONS}/*/common/*.h")

if (NOT DEFINED ENV{E_IMPL_DIR})
  set(ENV{E_IMPL_DIR} $ENV{E_OS})
endif()
if (NOT DEFINED ENV{E_IMPLX_DIR})
  set(ENV{E_IMPLX_DIR} $ENV{E_OS})
endif()

# Add operating system specific files. Filed are always searched from "os" directory, like "linux". We can also seachh from additional
# directory if E_PINS_EXTRA_DIR is specified, like for pigpio specific files or simulation specific file
if (DEFINED ENV{E_PINS_EXTRA_DIR})
   file(GLOB BB_OS_SOURCES "${BACKBONE}/$ENV{E_IMPL_DIR}/*.c" "${BACKBONE}/$ENV{E_IMPL_DIR}/*.cpp" "${BACKBONE}/$ENV{E_PINS_EXTRA_DIR}/*.c" "${BACKBONE}/$ENV{E_PINS_EXTRA_DIR}/*.cpp")
   file(GLOB EX_OS_SOURCES "${EXTENSIONS}/*/$ENV{E_IMPLX_DIR}/*.c" "${EXTENSIONS}/*/$ENV{E_IMPLX_DIR}/*.cpp" "${EXTENSIONS}/*/$ENV{E_PINS_EXTRA_DIR}/*.c" "${EXTENSIONS}/*/$ENV{E_PINS_EXTRA_DIR}/*.cpp")
   # file(GLOB BB_OS_HEADERS "${BACKBONE}/$ENV{E_IMPL_DIR}/*.h" "${BACKBONE}/$ENV{E_PINS_EXTRA_DIR}/*.h")
   # file(GLOB EX_OS_HEADERS "${EXTENSIONS}/*/$ENV{E_IMPLX_DIR}/*.h" "${EXTENSIONS}/*/$ENV{E_PINS_EXTRA_DIR}/*.h")
else()
   file(GLOB BB_OS_SOURCES "${BACKBONE}/$ENV{E_IMPL_DIR}/*.c" "${BACKBONE}/$ENV{E_IMPL_DIR}/*.cpp")
   file(GLOB EX_OS_SOURCES "${EXTENSIONS}/*/$ENV{E_IMPLX_DIR}/*.c" "${EXTENSIONS}/*/$ENV{E_IMPLX_DIR}/*.cpp")
   # file(GLOB BB_OS_HEADERS "${BACKBONE}/$ENV{E_IMPL_DIR}/*.h")
   # file(GLOB EX_OS_HEADERS "${EXTENSIONS}/*/$ENV{E_IMPLX_DIR}/*.h")
endif()

# Merge the .c files as one list.
set(srcs ${BB_OS_SOURCES})
list(APPEND srcs ${EX_OS_SOURCES})
list(APPEND srcs ${BB_CM_SOURCES})
list(APPEND srcs ${EX_CM_SOURCES})

# The pins library depends on: 
set(requires, "idf::iocom")

# Register pins as ESP-IDEF component.
idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "." REQUIRES "${requires}" )
