cmake_minimum_required(VERSION 3.10)
project(seam_carving CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  set(WARN_FLAGS /W3)
else()
  set(WARN_FLAGS -Wall -Wextra)
endif()

# ---- Max-flow / min-cut library (no OpenCV) --------------------------------
add_library(maxflow STATIC src/maxflow/MinCutMaxFlow.cpp)
target_include_directories(maxflow PUBLIC src)
target_compile_options(maxflow PRIVATE ${WARN_FLAGS})

# ---- Seam carving core: shared code + every seam-finding algorithm ---------
find_package(OpenCV REQUIRED)

add_library(seam_core STATIC
  src/common/seam.cpp
  src/algorithms/seam_greedy.cpp
  src/algorithms/seam_dp.cpp
  src/algorithms/seam_graphcut.cpp
)
target_include_directories(seam_core PUBLIC src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(seam_core PUBLIC maxflow ${OpenCV_LIBS})
target_compile_options(seam_core PRIVATE ${WARN_FLAGS})

# ---- Executables -----------------------------------------------------------

# Runs greedy / DP / graph cut on one image and compares them.
add_executable(seam_carving apps/compare_main.cpp)
target_link_libraries(seam_carving PRIVATE seam_core)

# DP only, writes the result plus a red/yellow seam visualisation.
add_executable(dp_seam apps/dp_main.cpp)
target_link_libraries(dp_seam PRIVATE seam_core)

# Stand-alone max-flow test (toy 4-node graph, Edmonds-Karp vs Dinic).
add_executable(mincut_demo apps/mincut_demo.cpp)
target_link_libraries(mincut_demo PRIVATE maxflow)
