# pins/CmakeLists.txt - Cmake build for pins library.
cmake_minimum_required(VERSION 3.5)

# Set project name (= project root folder name).
set(E_PROJECT "pins")
project(${E_PROJECT})

# include build information common to all projects.
include(../eosal/osbuild/cmakedefs/eosal-defs.txt)

# Set path to where to keep libraries.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $ENV{E_LIBOUT})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $ENV{E_LIBOUT})

# Set path to source files.
set(BACKBONE "$ENV{E_ROOT}/${E_PROJECT}/code")
set(EXTENSIONS "$ENV{E_ROOT}/${E_PROJECT}/extensions")

# Add iocom and pins library include paths.
include_directories("$ENV{E_ROOT}/iocom")
include_directories("$ENV{E_ROOT}/${E_PROJECT}")

# Add main headers.
set(MAIN_HEADERS "$ENV{E_ROOT}/${E_PROJECT}/${E_PROJECT}.h" "$ENV{E_ROOT}/${E_PROJECT}/${E_PROJECT}x.h")

# Add source files common to all operating systems.
file(GLOB BB_CM_SOURCES "${BACKBONE}/common/*.c")
file(GLOB BB_CM_HEADERS "${BACKBONE}/common/*.h")
file(GLOB EX_CM_SOURCES "${EXTENSIONS}/*/common/*.c")
file(GLOB EX_CM_HEADERS "${EXTENSIONS}/*/common/*.h")

# We use simulation code on Windows and linux
if($ENV{E_OS} MATCHES "windows")
   set(ENV{E_IMPL_DIR} "simulation")
   set(ENV{E_IMPL_EXTS_DIR} "simulation")
endif()
if($ENV{E_OS} MATCHES "linux")
    if($ENV{E_OSVER} MATCHES "pi")
      set(ENV{E_IMPL_DIR} "pigpio")
  else()
      set(ENV{E_IMPL_DIR} "simulation")
      set(ENV{E_IMPL_EXTS_DIR} "simulation")
#      set(ENV{E_IMPL_DIR} "pigpio")
  endif()
endif()

if (NOT DEFINED ENV{E_IMPL_DIR})
   set(ENV{E_IMPL_DIR} $ENV{E_OS})
endif()
if (NOT DEFINED ENV{E_IMPL_EXTS_DIR})
   set(ENV{E_IMPL_EXTS_DIR} $ENV{E_OS})
endif()

# Add operating system specific files.
file(GLOB BB_OS_SOURCES "${BACKBONE}/$ENV{E_IMPL_DIR}/*.c" "${BACKBONE}/$ENV{E_IMPL_DIR}/*.cpp")
file(GLOB BB_OS_HEADERS "${BACKBONE}/$ENV{E_IMPL_DIR}/*.h")
file(GLOB EX_OS_SOURCES "${EXTENSIONS}/*/$ENV{E_IMPL_EXTS_DIR}/*.c" "${EXTENSIONS}/*/$ENV{E_IMPL_EXTS_DIR}/*.cpp")
file(GLOB EX_OS_HEADERS "${EXTENSIONS}/*/$ENV{E_IMPL_EXTS_DIR}/*.h")

# Generate the static library
add_library(${E_PROJECT}${E_POSTFIX} STATIC ${MAIN_HEADERS} ${BB_CM_HEADERS} ${BB_OS_HEADERS} ${EX_CM_HEADERS} ${EX_OS_HEADERS} ${BB_CM_SOURCES} ${BB_OS_SOURCES} ${EX_CM_SOURCES} ${EX_OS_SOURCES})
