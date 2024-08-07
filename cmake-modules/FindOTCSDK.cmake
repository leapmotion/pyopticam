find_path(OTCSDK_ROOT_DIR
          NAMES include/camera.h
          HINTS "C:/Program Files (x86)/OptiTrack/CameraSDK"
          NO_CMAKE_PATH)

find_path(OTCSDK_INCLUDE_DIR
          NAMES cameralibrary.h
          HINTS "C:/Program Files (x86)/OptiTrack/CameraSDK/include"
          NO_CMAKE_PATH)

#message(STATUS "OTCSDK ROOT DIR PATH ${NP_CAMERASDK}")
message(STATUS "OTCSDK ROOT DIR ${OTCSDK_ROOT_DIR}")

#set(OTCSDK_INCLUDE_DIR ${OTCSDK_ROOT_DIR}/include)

message(STATUS "OTCSDK INCLUDE DIR ${OTCSDK_INCLUDE_DIR}")

find_library(OTCSDK_IMPORT_LIB CameraLibrary2019x64S HINTS "${OTCSDK_ROOT_DIR}/lib")
find_file(OTCSDK_SHARED_LIB NAMES "CameraLibrary2019x64S.dll" HINTS "${OTCSDK_ROOT_DIR}/lib")

message(STATUS "OTCSDK IMPORT LIBRARY DIR ${OTCSDK_IMPORT_LIB}")
message(STATUS "OTCSDK SHARED LIBRARY DIR ${OTCSDK_SHARED_LIB}")

include_directories("C:/Program Files (x86)/OptiTrack/CameraSDK/include")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OTCSDK DEFAULT_MSG OTCSDK_ROOT_DIR OTCSDK_INCLUDE_DIR OTCSDK_IMPORT_LIB OTCSDK_SHARED_LIB)

include(CreateImportTargetHelpers)
generate_import_target(OTCSDK SHARED TARGET CameraLibrary::CameraLibrary)
