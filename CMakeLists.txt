# pins/CmakeLists.txt - Cmake build for pins library, 14.3.2026/pekka.
cmake_minimum_required(VERSION 3.12)

# Set project name (= project root folder name).
set(E_PROJECT "pins")

# Set build root environment variable E_ROOT
include("${CMAKE_CURRENT_SOURCE_DIR}/../eosal/osbuild/cmakedefs/eosal-root-path.txt")

if (DEFINED ESP_PLATFORM)
  # Include ESP-IDF build specific eosal settings for the top level cmake. 
  include("$ENV{E_ROOT}/eosal/osbuild/cmakedefs/eosal-top-level-espidf-setup.txt")

  # Add necessary ESP32 component directories to the build system. This is needed
  # for ESP-IDF build, so it can find the components and their CMakeLists.txt files.
  set(EXTRA_COMPONENT_DIRS ${E_MINIMAL_COMPONENT_DIRS})
  list(APPEND EXTRA_COMPONENT_DIRS "$ENV{E_ROOT}/iocom/iocom_esp32")
  list(APPEND EXTRA_COMPONENT_DIRS "$ENV{E_ROOT}/pins/dependencies/esp32")
  list(APPEND EXTRA_COMPONENT_DIRS "$ENV{E_ROOT}/${E_PROJECT}/${E_PROJECT}_esp32")

  # Enable dummy app_main for ESP-IDF build, so we can build eosal as executable ESP32 component. 
  add_definitions(-DOSAL_DUMMY_ESP32_APP_MAIN)

  # Include ESP-IDFbuild stuff. This will call the project() command and start the build process.
  include($ENV{IDF_PATH}/tools/cmake/project.cmake)
 
  # Name the project.
  project(${E_PROJECT})

else()
  project(${E_PROJECT})

  # Include common setting for most of builds.
  include("$ENV{E_ROOT}/eosal/osbuild/cmakedefs/eosal-defs.txt")

  # Set path to where to keep libraries.
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $ENV{E_LIBOUT})
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $ENV{E_LIBOUT})

  # Set path to source files.
  set(BACKBONE "$ENV{E_ROOT}/${E_PROJECT}/code")
  set(EXTENSIONS "$ENV{E_ROOT}/${E_PROJECT}/extensions")

  # Add pins to include path.
  include_directories("$ENV{E_ROOT}/iocom")
  include_directories("$ENV{E_ROOT}/${E_PROJECT}")

  # Add main headers.
  set(MAIN_HEADERS "$ENV{E_ROOT}/${E_PROJECT}/${E_PROJECT}.h" "$ENV{E_ROOT}/${E_PROJECT}/${E_PROJECT}x.h")

  # Add source files common to all operating systems.
  file(GLOB BB_CM_SOURCES "${BACKBONE}/common/*.c")
  file(GLOB BB_CM_HEADERS "${BACKBONE}/common/*.h")
  file(GLOB EX_CM_SOURCES "${EXTENSIONS}/*/common/*.c")
  file(GLOB EX_CM_HEADERS "${EXTENSIONS}/*/common/*.h")

    # We include simulation code on Windows and linux, and pigpio for Raspberry.
  if($ENV{E_OS} STREQUAL "windows")
     set(ENV{E_IMPL_DIR} "simulation")
  endif()
  if($ENV{E_OS} STREQUAL "linux")
    if($ENV{E_OSVER} STREQUAL "pi")
      set(ENV{E_IMPL_DIR} "pigpio")
      set(ENV{E_PINS_EXTRA_DIR} "pigpio")
    else()
      set(ENV{E_PINS_EXTRA_DIR} "simulation")
    endif()
  endif()

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
     file(GLOB BB_OS_HEADERS "${BACKBONE}/$ENV{E_IMPL_DIR}/*.h" "${BACKBONE}/$ENV{E_PINS_EXTRA_DIR}/*.h")
     file(GLOB EX_OS_SOURCES "${EXTENSIONS}/*/$ENV{E_IMPLX_DIR}/*.c" "${EXTENSIONS}/*/$ENV{E_IMPLX_DIR}/*.cpp" "${EXTENSIONS}/*/$ENV{E_PINS_EXTRA_DIR}/*.c" "${EXTENSIONS}/*/$ENV{E_PINS_EXTRA_DIR}/*.cpp")
     file(GLOB EX_OS_HEADERS "${EXTENSIONS}/*/$ENV{E_IMPLX_DIR}/*.h" "${EXTENSIONS}/*/$ENV{E_PINS_EXTRA_DIR}/*.h")
  else()
     file(GLOB BB_OS_SOURCES "${BACKBONE}/$ENV{E_IMPL_DIR}/*.c" "${BACKBONE}/$ENV{E_IMPL_DIR}/*.cpp")
     file(GLOB BB_OS_HEADERS "${BACKBONE}/$ENV{E_IMPL_DIR}/*.h")
     file(GLOB EX_OS_SOURCES "${EXTENSIONS}/*/$ENV{E_IMPLX_DIR}/*.c" "${EXTENSIONS}/*/$ENV{E_IMPLX_DIR}/*.cpp")
     file(GLOB EX_OS_HEADERS "${EXTENSIONS}/*/$ENV{E_IMPLX_DIR}/*.h")
  endif()

  # Generate the static library
  add_library(${E_PROJECT}${E_POSTFIX} STATIC ${MAIN_HEADERS} ${BB_CM_HEADERS} ${BB_OS_HEADERS} ${EX_CM_HEADERS} ${EX_OS_HEADERS} ${BB_CM_SOURCES} ${BB_OS_SOURCES} ${EX_CM_SOURCES} ${EX_OS_SOURCES})

endif()
