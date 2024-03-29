project(pyopticam)
cmake_minimum_required(VERSION 3.17...3.22)

if (NOT SKBUILD)
  message(WARNING "This CMake file should be executed via scikit-build. "
                      "Please run\n$ pip install .")
endif()

add_compile_definitions(_NATURAL_POINT_CPP11_)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules")
message(STATUS "Module Path ${CMAKE_MODULE_PATH}")
find_package(OTCSDK)

# Constrain FindPython to find the Python version used by scikit-build
if (SKBUILD)
  message(STATUS "Python_VERSION ${PYTHON_VERSION_STRING}")
  message(STATUS "Python_EXECUTABLE ${PYTHON_EXECUTABLE}")
  message(STATUS "Python_INCLUDE_DIR ${PYTHON_INCLUDE_DIR}")
  message(STATUS "Python_LIBRARIES ${PYTHON_LIBRARY}")
  set(Python_VERSION "${PYTHON_VERSION_STRING}")
  set(Python_EXECUTABLE "${PYTHON_EXECUTABLE}")
  set(Python_INCLUDE_DIR "${PYTHON_INCLUDE_DIR}")
  set(Python_LIBRARIES "${PYTHON_LIBRARY}")
elseif (MSVC)
  # MSVC needs a little extra help finding the Python library
  find_package(PythonInterp)
  find_package(Python)
endif()
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

# Detect nanobind and import it
execute_process(
  COMMAND
  "${PYTHON_EXECUTABLE}" -c "import nanobind; print(nanobind.cmake_dir())"
  OUTPUT_VARIABLE _tmp_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ECHO STDOUT)
  list(APPEND CMAKE_PREFIX_PATH "${_tmp_dir}")

find_package(nanobind CONFIG REQUIRED)

message(STATUS ${NB_DIR})

nanobind_add_module(
  pyopticam_ext
  NB_STATIC # Build static libnanobind (the extension module itself remains a shared library)
  src/pyopticam_ext.cpp
)
target_link_libraries(pyopticam_ext PRIVATE "C:/Program Files (x86)/OptiTrack/Camera SDK/lib/CameraLibrary2015x64S.lib")

install(TARGETS pyopticam_ext LIBRARY DESTINATION .)
