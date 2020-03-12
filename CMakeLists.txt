# pins/CmakeLists.txt - Cmake build for pins library.
cmake_minimum_required(VERSION 3.5)

# Set project name (= project root folder name).
set(E_PROJECT "pins")
project(${E_PROJECT})

# include build information common to all projects.
include(../eosal/build/cmakedefs/eosal-defs.txt)

# Set path to where to keep libraries.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $ENV{E_LIBOUT})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $ENV{E_LIBOUT})

# Set path to source files.
set(BACKBONE "$ENV{E_ROOT}/${E_PROJECT}/code")

# Add iocom and pins library include paths.
include_directories("$ENV{E_ROOT}/iocom")
include_directories("$ENV{E_ROOT}/${E_PROJECT}")

# Add main headers.
set(MAIN_HEADERS "$ENV{E_ROOT}/${E_PROJECT}/${E_PROJECT}.h" "$ENV{E_ROOT}/${E_PROJECT}/${E_PROJECT}x.h")

# Add source files common to all operating systems.
file(GLOB BB_CM_SOURCES "${BACKBONE}/common/*.c")
file(GLOB BB_CM_HEADERS "${BACKBONE}/common/*.h")

# We use simulation code on Windows and linux
if($ENV{E_OS} MATCHES "windows")
   set(ENV{E_PINS_SIMULATION} "simulation")
endif()
if($ENV{E_OS} MATCHES "linux")
   set(ENV{E_PINS_SIMULATION} "simulation")
endif()

# Select OS specific directory or simulation
if (DEFINED ENV{E_PINS_SIMULATION})
   set(ENV{E_IMPL_DIR} $ENV{E_PINS_SIMULATION})
else()
   set(ENV{E_IMPL_DIR} $ENV{E_OS})
endif()


file(GLOB BB_OS_SOURCES "${BACKBONE}/$ENV{E_IMPL_DIR}/*.c" "${BACKBONE}/$ENV{E_IMPL_DIR}/*.cpp")
file(GLOB BB_OS_HEADERS "${BACKBONE}/$ENV{E_IMPL_DIR}/*.h")

# Generate the static library
add_library(${E_PROJECT}${E_POSTFIX} STATIC ${MAIN_HEADERS} ${BB_CM_HEADERS} ${BB_OS_HEADERS} ${BB_CM_SOURCES} ${BB_OS_SOURCES})
