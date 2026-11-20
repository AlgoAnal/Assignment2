# Provides OpenCV for this project.
#
# 1. If an installed OpenCV is found (system package, OpenCV_DIR, ...), use it.
# 2. Otherwise download the OpenCV source once, build only the modules this
#    project needs as static libraries, and install them inside the build
#    folder (<build>/_deps/opencv-<version>/install). Later configures reuse it.
#
# Options:
#   -DSEAM_FETCH_OPENCV=OFF        fail instead of downloading
#   -DSEAM_OPENCV_VERSION=4.10.0   version to download
include_guard(GLOBAL)

option(SEAM_FETCH_OPENCV "Download and build OpenCV if it is not installed" ON)
set(SEAM_OPENCV_VERSION "4.10.0" CACHE STRING "OpenCV version to download if it is not installed")

# No highgui: the demo's window and UI come from GLFW + Dear ImGui.
set(_ocv_modules core imgproc imgcodecs)

set(_ocv_root "${CMAKE_BINARY_DIR}/_deps/opencv-${SEAM_OPENCV_VERSION}")
set(_ocv_src "${_ocv_root}/src/opencv-${SEAM_OPENCV_VERSION}")
set(_ocv_build "${_ocv_root}/build")
set(_ocv_install "${_ocv_root}/install")
set(_ocv_stamp "${_ocv_install}/.seam_carving_installed")

# Already downloaded into this build folder: skip straight to using it.
# (Probing first would let OpenCV's Windows config pick the wrong runtime
# folder, because OpenCV_STATIC is not set yet at that point.)
if(NOT EXISTS "${_ocv_stamp}")
  find_package(OpenCV QUIET COMPONENTS ${_ocv_modules})
  if(OpenCV_FOUND)
    message(STATUS "Using OpenCV ${OpenCV_VERSION} from ${OpenCV_DIR}")
    return()
  endif()

  if(NOT SEAM_FETCH_OPENCV)
    message(FATAL_ERROR "OpenCV not found. Install it, point OpenCV_DIR at it, "
                        "or configure with -DSEAM_FETCH_OPENCV=ON to download it.")
  endif()
  message(STATUS "OpenCV not found: building OpenCV ${SEAM_OPENCV_VERSION} from source "
                 "(one-time, takes several minutes)")

  # ---- Download + extract the source ---------------------------------------
  if(NOT EXISTS "${_ocv_src}/CMakeLists.txt")
    set(_url "https://github.com/opencv/opencv/archive/refs/tags/${SEAM_OPENCV_VERSION}.tar.gz")
    set(_archive "${_ocv_root}/opencv-${SEAM_OPENCV_VERSION}.tar.gz")
    message(STATUS "Downloading ${_url}")
    file(DOWNLOAD "${_url}" "${_archive}" STATUS _status TLS_VERIFY ON)
    list(GET _status 0 _code)
    if(NOT _code EQUAL 0)
      list(GET _status 1 _msg)
      file(REMOVE "${_archive}")
      message(FATAL_ERROR "Downloading OpenCV failed: ${_msg}")
    endif()
    file(ARCHIVE_EXTRACT INPUT "${_archive}" DESTINATION "${_ocv_root}/src")
    file(REMOVE "${_archive}")
  endif()

  # ---- Configure: same generator/compiler as this project, minimal modules --
  set(_gen_args -G "${CMAKE_GENERATOR}")
  if(CMAKE_GENERATOR_PLATFORM)
    list(APPEND _gen_args -A "${CMAKE_GENERATOR_PLATFORM}")
  endif()
  if(CMAKE_GENERATOR_TOOLSET)
    list(APPEND _gen_args -T "${CMAKE_GENERATOR_TOOLSET}")
  endif()
  if(CMAKE_MAKE_PROGRAM)
    list(APPEND _gen_args "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}")
  endif()
  if(NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    list(APPEND _gen_args "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
  endif()

  string(REPLACE ";" "," _ocv_build_list "${_ocv_modules}")
  message(STATUS "Configuring OpenCV (modules: ${_ocv_build_list})")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${_ocv_src}" -B "${_ocv_build}" ${_gen_args}
            -DCMAKE_BUILD_TYPE=Release
            "-DCMAKE_INSTALL_PREFIX=${_ocv_install}"
            "-DBUILD_LIST=${_ocv_build_list}"
            -DBUILD_SHARED_LIBS=OFF        # static: no DLLs to copy next to the .exe
            -DBUILD_WITH_STATIC_CRT=OFF    # MSVC: match this project's /MD runtime
            -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF
            -DBUILD_opencv_apps=OFF -DBUILD_DOCS=OFF -DBUILD_JAVA=OFF
            -DBUILD_opencv_python2=OFF -DBUILD_opencv_python3=OFF
            -DOPENCV_GENERATE_PKGCONFIG=OFF
            # Optional extras that need more downloads or are not used here
            -DWITH_IPP=OFF -DWITH_ADE=OFF -DWITH_PROTOBUF=OFF -DWITH_EIGEN=OFF
            -DWITH_OPENCL=OFF -DWITH_FFMPEG=OFF -DWITH_GSTREAMER=OFF -DWITH_TBB=OFF
            -DWITH_OPENEXR=OFF -DWITH_OPENJPEG=OFF -DWITH_JASPER=OFF -DWITH_TIFF=OFF
            -DWITH_WEBP=OFF -DWITH_LAPACK=OFF -DWITH_ITT=OFF -DWITH_QT=OFF -DWITH_VTK=OFF
    RESULT_VARIABLE _rc)
  if(NOT _rc EQUAL 0)
    message(FATAL_ERROR "Configuring OpenCV failed (see output above)")
  endif()

  # ---- Build + install (Release only) --------------------------------------
  cmake_host_system_information(RESULT _jobs QUERY NUMBER_OF_LOGICAL_CORES)
  message(STATUS "Building OpenCV with ${_jobs} jobs")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${_ocv_build}" --config Release
            --target install --parallel ${_jobs}
    RESULT_VARIABLE _rc)
  if(NOT _rc EQUAL 0)
    message(FATAL_ERROR "Building OpenCV failed (see output above)")
  endif()

  file(WRITE "${_ocv_stamp}" "${SEAM_OPENCV_VERSION}\n")
endif()

# Clear anything a failed probe may have left behind, then load the static build.
unset(OpenCV_ARCH)
unset(OpenCV_RUNTIME)
unset(OpenCV_DIR CACHE)
set(OpenCV_STATIC ON)

# On Windows the install is a "pack": a top-level OpenCVConfig.cmake that
# picks <arch>/<vcNN>/staticlib from the compiler version. OpenCV 4.10 does
# not know newer compilers (e.g. Visual Studio 2026, MSVC 19.5x) and then
# finds nothing, so load the inner config directly when there is one.
set(_ocv_config_dir "${_ocv_install}")
file(GLOB_RECURSE _ocv_inner_configs "${_ocv_install}/OpenCVConfig.cmake")
list(FILTER _ocv_inner_configs INCLUDE REGEX "/staticlib/OpenCVConfig\\.cmake$")
if(_ocv_inner_configs)
  list(GET _ocv_inner_configs 0 _ocv_inner)
  get_filename_component(_ocv_config_dir "${_ocv_inner}" DIRECTORY)
endif()
find_package(OpenCV REQUIRED CONFIG COMPONENTS ${_ocv_modules}
             PATHS "${_ocv_config_dir}" NO_DEFAULT_PATH)
message(STATUS "Using downloaded OpenCV ${OpenCV_VERSION} from ${OpenCV_DIR}")

if(MSVC)
  # Only a Release build of OpenCV is made, and MSVC cannot mix Debug and
  # Release runtimes in one program.
  message(STATUS "Downloaded OpenCV is Release-only: build this project with "
                 "--config Release (or RelWithDebInfo), not Debug.")
endif()
