# Assignment2
Repository for Assignment 2 of the Algorithm Analysis (CSD3131/CSD3130) module, submitted by Group 17.

Image compression by seam carving, implemented three ways: dynamic programming,
greedy, and graph cut (min-cut / max-flow).

## Video presentation

[![Video presentation](https://img.youtube.com/vi/y9E9H-4E-kw/hqdefault.jpg)](https://www.youtube.com/watch?v=y9E9H-4E-kw)

Watch on YouTube: https://www.youtube.com/watch?v=y9E9H-4E-kw

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
  demo/              -> seam_demo      interactive demo (Dear ImGui + GLFW)
    carve_session.*                    step-by-step carving state (no UI code)
    file_dialog.*                      Open dialog + image loading (platform code)
    main.cpp                           window and UI
  dp_main.cpp        -> dp_seam        DP only, writes result + seam visualisation
  mincut_demo.cpp    -> mincut_demo    toy max-flow graph, no OpenCV needed
images/              test input images
```

Every algorithm implements the same signature from `src/common/seam.h`:

```cpp
seam_t find_seam_xxx(const cv::Mat& energy, float& total_cost);
```

so `seam_carving` can time and compare them on identical energy maps.

## Quick start (Windows)

Double-click **`run.bat`** for a menu, or use it from a terminal:

| Command | What it does |
|---|---|
| `run.bat` | Menu: pick a program to run, build, or clean |
| `run.bat demo [image]` | Interactive demo (`seam_demo`) |
| `run.bat compare [image width height]` | All algorithms on one image (`seam_carving`) |
| `run.bat dp [image width height]` | DP only, with seam visualisation (`dp_seam`) |
| `run.bat mincut` | Max-flow / min-cut test (`mincut_demo`) |
| `run.bat build` | Build everything (Release) |
| `run.bat clean` | Delete compiled files, keep the downloaded OpenCV |
| `run.bat clean all` | Delete the whole `build/` folder (OpenCV is rebuilt next time) |

Every run command builds first if needed. Missing image/size arguments are
asked for, with defaults. Results are written to `output/`.

## Build

Requires CMake 3.18+ and a C++17 compiler (on Windows: Visual Studio 2022 with
"Desktop development with C++"). Nothing else needs to be installed:

- If OpenCV is installed (or `OpenCV_DIR` points at it), that copy is used.
- Otherwise the first `cmake` configure downloads the OpenCV source
  ([`cmake/FetchOpenCV.cmake`](cmake/FetchOpenCV.cmake)), builds only the modules
  this project needs (core, imgproc, imgcodecs) as static libraries and
  installs them into `build/_deps/`. This takes several minutes once; later
  configures reuse it. Pass `-DSEAM_FETCH_OPENCV=OFF` to turn this off.
- `seam_demo` also needs [Dear ImGui](https://github.com/ocornut/imgui) and
  [GLFW](https://www.glfw.org/); CMake downloads both
  ([`cmake/FetchImGui.cmake`](cmake/FetchImGui.cmake)). They build in seconds.

```bash
cmake -S . -B build
cmake --build build --config Release
```

Notes on the downloaded OpenCV:
- It is Release-only, so with Visual Studio build `Release` or `RelWithDebInfo`, not `Debug`.
- On Linux/WSL, GLFW needs the X11/OpenGL headers:
  `sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl-dev`.

## Run

With Visual Studio generators the executables land in `build/Release/` instead of `build/`.
OpenCV does not create folders, so make `output/` first (it is git-ignored).

```bash
mkdir -p output

# Compare greedy / DP / graph cut (Dinic and Edmonds-Karp), removing 5 seams.
# Graph cut is slow, so keep the number of seams small.
# Writes <prefix>_<algo>.png and <prefix>_<algo>_seams.png
./build/seam_carving images/tower.png output/tower 646 717

# DP only; also writes <output>_visualized.<ext> (vertical seams red, horizontal yellow)
./build/dp_seam images/tower1.jpg output/tower_dp.jpg 1200 800

# Max-flow sanity check
./build/mincut_demo
```

## Interactive demo

```bash
./build/seam_demo            # or: ./build/seam_demo images/tower.jpg
```

A resizable window with the controls on the left and, on the right, the
original with every removed seam painted next to the current result.
Vertical seams are removed first, then horizontal ones.

**Opening images:** the *Open image...* button (or `O`), dragging an image file
onto the window, dropping one onto `seam_demo.exe` in Explorer, or passing a path.
On Linux the Open dialog uses `zenity` (`sudo apt install zenity`).

| Control | What it does |
|---|---|
| Working size | Downscales the image so its longest side is at most 200-800 px (or full size) before carving. Graph cut is slow on large images. |
| Algorithm (dropdown) | Greedy, Greedy (all starts), Dynamic programming, Graph cut (Dinic), Graph cut (Edmonds-Karp). Changing it restarts. |
| Width / Height sliders | Target size. Shrinking further continues from the current image; growing restarts. Ctrl+click to type a value. |
| Step (`Space`) | Remove one seam (hold to repeat) |
| Play / Pause (`P`) | Animate, with *seams per frame* setting the speed |
| Finish (`F`) | Remove all remaining seams as fast as possible |
| Reset (`R`) | Back to the original |
| Save result (`S`) | Writes `demo_<image>_<algo>.png` and `..._seams.png` to the current folder |
| Show energy map (`E`) | Shows the Sobel energy `\|Gx\| + \|Gy\|` of the result instead |

Seam colours: red = removed vertical seam, yellow = removed horizontal seam,
cyan = the next seam that will be removed. The statistics panel shows the seam
search time (time inside the seam finder only, like `seam_carving`), the total
energy of the removed seams and the energy of the next seam.
