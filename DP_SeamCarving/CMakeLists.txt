cmake_minimum_required(VERSION 3.10)
project(DP_SeamCarving LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenCV REQUIRED CONFIG)

add_executable(dp_seamcarving main.cpp seam_dp.cpp)
target_compile_features(dp_seamcarving PRIVATE cxx_std_17)
target_include_directories(dp_seamcarving PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(dp_seamcarving PRIVATE ${OpenCV_LIBS})

set_target_properties(dp_seamcarving PROPERTIES OUTPUT_NAME dp_seam)