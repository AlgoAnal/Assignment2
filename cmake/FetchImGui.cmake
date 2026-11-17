# Downloads GLFW (window + OpenGL context + file drop) and Dear ImGui (UI)
# for seam_demo. Both are small and build in seconds.
include_guard(GLOBAL)
include(FetchContent)

set(SEAM_GLFW_VERSION "3.4" CACHE STRING "GLFW version to download")
set(SEAM_IMGUI_VERSION "1.91.5" CACHE STRING "Dear ImGui version to download")

# Only the library itself; on Linux build the X11 backend only (fewer packages).
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)

FetchContent_Declare(glfw
  URL "https://github.com/glfw/glfw/archive/refs/tags/${SEAM_GLFW_VERSION}.tar.gz"
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(imgui
  URL "https://github.com/ocornut/imgui/archive/refs/tags/v${SEAM_IMGUI_VERSION}.tar.gz"
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(glfw imgui)  # imgui has no CMakeLists: only downloaded

find_package(OpenGL REQUIRED)

# Dear ImGui core + the GLFW / OpenGL 3 backends.
add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)
