cmake_minimum_required(VERSION 3.10)
project(seam_carving CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenCV REQUIRED)

# Always built.
set(SEAM_SOURCES main.cpp ./common/seam.cpp seam_greedy.cpp)
set(SEAM_DEFS "")

# Optional algorithms: picked up automatically once the file exists.
# (Re-run cmake after adding a file so this check runs again.)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/seam_dp.cpp")
  list(APPEND SEAM_SOURCES seam_dp.cpp)
  list(APPEND SEAM_DEFS SEAM_HAVE_DP)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/seam_graphcut.cpp")
  list(APPEND SEAM_SOURCES seam_graphcut.cpp)
  list(APPEND SEAM_DEFS SEAM_HAVE_GRAPHCUT)
endif()
message(STATUS "seam_carving sources: ${SEAM_SOURCES}")

add_executable(seam_carving ${SEAM_SOURCES})
target_include_directories(seam_carving PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(seam_carving PRIVATE ${OpenCV_LIBS})
if(SEAM_DEFS)
  target_compile_definitions(seam_carving PRIVATE ${SEAM_DEFS})
endif()
if(NOT MSVC)
  target_compile_options(seam_carving PRIVATE -Wall -Wextra)
endif()
