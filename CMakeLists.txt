cmake_minimum_required(VERSION 3.27)

project(tmsolve)

set(MYLIB_VERSION_MAJOR 3)
set(MYLIB_VERSION_MINOR 1)
set(MYLIB_VERSION_PATCH 0)
set(MYLIB_VERSION_STRING ${MYLIB_VERSION_MAJOR}.${MYLIB_VERSION_MINOR}.${MYLIB_VERSION_PATCH})

set(CMAKE_CXX_STANDARD 20)

# Build from the headers I got here
add_compile_definitions(LOCAL_BUILD)

# Include header files
include_directories(include)

# Not running using skbuild, so we only need to build the shared library without bindings
if (NOT SKBUILD)
  # Install library headers
  file(GLOB HEADERS include/*.h)

  # Find source files
  file(GLOB SOURCES src/*.c src/*.cpp)

  # Create shared library
  add_library(${PROJECT_NAME} SHARED ${SOURCES})

  # Link to math library
  target_link_libraries(${PROJECT_NAME} m)

  if (MSYS)
  target_link_libraries(${PROJECT_NAME} winpthread)
  add_compile_options("-static")
  endif()

  set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${MYLIB_VERSION_STRING} SOVERSION ${MYLIB_VERSION_MAJOR})

  # Install rules
  if(LINUX)
      install(TARGETS ${PROJECT_NAME} DESTINATION /usr/local/lib)
      install(CODE "execute_process(COMMAND ldconfig)")
  elseif(MSYS)
      install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX})
  endif()

  install(FILES ${HEADERS} DESTINATION include/${PROJECT_NAME})

# Building the Python bindings, not a regular shared library
else()
  if (CMAKE_VERSION VERSION_LESS 3.18)
    set(DEV_MODULE Development)
  else()
    set(DEV_MODULE Development.Module)
  endif()

  # Enable nanobind code segments in the C++ binder
  add_compile_definitions(PYTHON_BINDINGS_BUILD)

  # Try to import all Python components potentially needed by nanobind
  find_package(Python 3.8
    REQUIRED COMPONENTS Interpreter Development.Module
    OPTIONAL_COMPONENTS Development.SABIModule)

  find_package(Python 3.8 COMPONENTS Interpreter ${DEV_MODULE} REQUIRED)

  if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
  endif()

  # Detect the installed nanobind package and import it into CMake
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ext/nanobind)

  nanobind_add_module(tmsolve src/c++_binder.cpp src/bitwise.c src/error_handler.c src/evaluator.c src/function.c src/hashmap.c src/hashset.c src/internals.c src/int_parser.c src/matrix.c src/parser.c src/parser_common.h src/scientific.c src/string_tools.c src/tms_complex.c src/version.c)

  nanobind_add_stub(
  tmsolve_stub
  MODULE tmsolve
  OUTPUT tmsolve.pyi
  PYTHON_PATH $<TARGET_FILE_DIR:tmsolve>
  DEPENDS tmsolve)

  install(TARGETS tmsolve LIBRARY DESTINATION tmsolve)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tmsolve.pyi DESTINATION tmsolve)

endif()
