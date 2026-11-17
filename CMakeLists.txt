cmake_minimum_required(VERSION 3.18)  # 3.18: file(ARCHIVE_EXTRACT) in FetchOpenCV
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
# Installed OpenCV if available, otherwise downloads and builds it once.
include(cmake/FetchOpenCV.cmake)

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

# Interactive demo (Dear ImGui + GLFW, both downloaded by CMake):
# step-by-step seams, target-size sliders, algorithm dropdown, drag-and-drop.
include(cmake/FetchImGui.cmake)
add_executable(seam_demo
  apps/demo/main.cpp
  apps/demo/carve_session.cpp
  apps/demo/file_dialog.cpp
)
target_link_libraries(seam_demo PRIVATE seam_core imgui)
target_compile_options(seam_demo PRIVATE ${WARN_FLAGS})
if(WIN32)
  target_link_libraries(seam_demo PRIVATE comdlg32)  # GetOpenFileNameW
  # Windows GUI app: no console window, but keep the normal main().
  set_target_properties(seam_demo PROPERTIES WIN32_EXECUTABLE TRUE)
  if(MSVC)
    target_link_options(seam_demo PRIVATE /ENTRY:mainCRTStartup)
  endif()
endif()

# Stand-alone max-flow test (toy 4-node graph, Edmonds-Karp vs Dinic).
add_executable(mincut_demo apps/mincut_demo.cpp)
target_link_libraries(mincut_demo PRIVATE maxflow)
