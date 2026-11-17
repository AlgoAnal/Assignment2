# Assignment2
Repository for Assignment 2 of the Algorithm Analysis (CSD3131/CSD3130) module, submitted by Group 17.

Image compression by seam carving, implemented three ways: dynamic programming,
greedy, and graph cut (min-cut / max-flow).

## Layout

```
src/
  common/       seam.h / seam.cpp     shared energy map (Sobel), seam removal loop, timing
  algorithms/   seam_dp.cpp/.h        dynamic programming (optimal seam)
                seam_greedy.cpp       greedy, row-by-row (+ "all starts" variant)
                seam_graphcut.cpp     graph cut: builds the pixel graph, runs max-flow
  maxflow/      MinCutMaxFlow.cpp/.h  Edmonds-Karp and Dinic on an adjacency list
apps/
  compare_main.cpp   -> seam_carving   runs every algorithm on one image and compares them
  dp_main.cpp        -> dp_seam        DP only, writes result + seam visualisation
  mincut_demo.cpp    -> mincut_demo    toy max-flow graph, no OpenCV needed
images/              test input images
```

Every algorithm implements the same signature from `src/common/seam.h`:

```cpp
seam_t find_seam_xxx(const cv::Mat& energy, float& total_cost);
```

so `seam_carving` can time and compare them on identical energy maps.

## Build

Requires CMake 3.10+, a C++17 compiler and OpenCV.

```bash
cmake -S . -B build
cmake --build build --config Release
```

## Run

With Visual Studio generators the executables land in `build/Release/` instead of `build/`.
OpenCV does not create folders, so make `output/` first (it is git-ignored).

```bash
mkdir -p output

# Compare greedy / DP / graph cut, shrinking to 500x500.
# Writes <prefix>_<algo>.png and <prefix>_<algo>_seams.png
./build/seam_carving images/tower.png output/tower 500 500

# DP only; also writes <output>_visualized.<ext> (vertical seams red, horizontal yellow)
./build/dp_seam images/tower.jpg output/tower_dp.jpg 1200 800

# Max-flow sanity check
./build/mincut_demo
```
