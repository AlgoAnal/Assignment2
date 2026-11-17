// seam_demo: interactive seam carving (Dear ImGui + GLFW + OpenGL).
// Usage: seam_demo [image]
//
// Left: controls (image, algorithm, target size, playback, statistics).
// Right: the original with every removed seam painted, next to the result.
// Images can be opened with the Open button / O key or dragged onto the window.

#include "carve_session.h"
#include "file_dialog.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>  // also includes the system OpenGL header (GL 1.1 calls only)

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>
#include <utility>

namespace {

namespace fs = std::filesystem;
using clock_type = std::chrono::steady_clock;

const char* kTitle = "Seam Carving Demo";

// Longest side the image is downscaled to before carving (0 = full size).
const int kWorkingSizes[] = {200, 300, 400, 600, 800, 0};
const char* kWorkingSizeNames[] = {"200 px", "300 px", "400 px", "600 px", "800 px",
                                   "Full size"};
constexpr int kDefaultWorkingSize = 2;  // 400 px

// Same colours as CarveSession paints (BGR there, RGB here).
const ImVec4 kRed(1.0f, 0.0f, 0.0f, 1.0f);
const ImVec4 kYellow(1.0f, 1.0f, 0.0f, 1.0f);
const ImVec4 kCyan(0.0f, 1.0f, 1.0f, 1.0f);

// An OpenGL texture holding a BGR cv::Mat, for ImGui::Image.
struct texture {
  GLuint id = 0;
  int w = 0;
  int h = 0;

  void upload(const cv::Mat& bgr) {
    cv::Mat rgba;
    cv::cvtColor(bgr, rgba, cv::COLOR_BGR2RGBA);
    if (id == 0) glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    // NEAREST keeps 1-pixel seams sharp when the image is zoomed.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);  // RGBA rows are always 4-byte aligned
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba.cols, rgba.rows, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, rgba.data);
    w = rgba.cols;
    h = rgba.rows;
  }
  void release() {
    if (id != 0) glDeleteTextures(1, &id);
    id = 0;
  }
  ImTextureID imgui_id() const { return (ImTextureID)(intptr_t)id; }
};

struct app_state {
  GLFWwindow* window = nullptr;
  CarveSession session;
  cv::Mat full;      // image as loaded, before downscaling
  std::string name;  // its file name (UTF-8)
  int working_size = kDefaultWorkingSize;

  bool playing = false;
  bool finishing = false;  // play as fast as possible until the target
  int seams_per_frame = 1;
  bool show_energy = false;
  bool sliders_active = false;  // a target slider is being dragged
  std::string message = "Open an image to start.";

  fs::path dropped;  // set by the GLFW drop callback

  texture left, right;
  int uploaded_version = -1;
  bool uploaded_energy = false;
};

// ---- Actions ----------------------------------------------------------------

void apply_working_size(app_state& app) {
  if (app.full.empty()) return;
  cv::Mat src = app.full;
  const int max_dim = kWorkingSizes[app.working_size];
  if (max_dim > 0 && std::max(src.cols, src.rows) > max_dim) {
    const double f = static_cast<double>(max_dim) / std::max(src.cols, src.rows);
    cv::resize(src, src, cv::Size(), f, f, cv::INTER_AREA);
  }
  app.session.load(src);  // keeps the selected algorithm
  app.playing = app.finishing = false;
}

void load_file(app_state& app, const fs::path& path) {
  cv::Mat img = read_image(path);
  if (img.empty()) {
    app.message = "Could not read " + path.filename().u8string() + " as an image.";
    return;
  }
  app.full = img;
  app.name = path.filename().u8string();
  apply_working_size(app);
  glfwSetWindowTitle(app.window, (std::string(kTitle) + " - " + app.name).c_str());
  app.message = "Loaded " + app.name + ". Press Step or Play.";
}

void open_dialog(app_state& app) {
  app.playing = false;
  std::string error;
  const fs::path path = browse_for_image(error);
  if (!path.empty()) {
    load_file(app, path);
  } else if (!error.empty()) {
    app.message = error;
  }
}

void step_once(app_state& app) {
  app.playing = app.finishing = false;
  app.message = app.session.step() ? "" : "Already at the target size.";
}

void toggle_play(app_state& app) {
  if (app.session.done()) {
    app.message = "Already at the target size.";
    return;
  }
  app.playing = !app.playing || app.finishing;
  app.finishing = false;
  app.message.clear();
}

void finish(app_state& app) {
  if (app.session.done()) {
    app.message = "Already at the target size.";
    return;
  }
  app.playing = app.finishing = true;
  app.message.clear();
}

void reset(app_state& app) {
  app.session.reset();
  app.playing = app.finishing = false;
  app.message = "Reset to the original.";
}

// Letters, digits and '-' kept; everything else becomes '_'.
std::string file_safe(const std::string& s) {
  std::string out;
  for (unsigned char c : s) {
    const bool keep = std::isalnum(c) || c == '-';
    if (keep) out += static_cast<char>(c);
    else if (!out.empty() && out.back() != '_') out += '_';
  }
  while (!out.empty() && out.back() == '_') out.pop_back();
  return out;
}

void save(app_state& app) {
  const std::string stem = "demo_" + file_safe(fs::path(app.name).stem().u8string()) + "_" +
                           file_safe(kAlgos[app.session.algorithm()].name);
  const bool ok = cv::imwrite(stem + ".png", app.session.result()) &&
                  cv::imwrite(stem + "_seams.png", app.session.marked());
  std::error_code ec;
  const fs::path folder = fs::current_path(ec);
  app.message = ok ? "Saved " + stem + ".png and " + stem + "_seams.png in " + folder.u8string()
                   : "Could not save the images in the current folder.";
}

// Called once per frame, before drawing.
void run_playback(app_state& app) {
  if (!app.playing) return;
  bool more = true;
  if (app.finishing) {
    // Up to ~30 ms of seams per frame so the window stays responsive.
    const auto t0 = clock_type::now();
    do {
      more = app.session.step();
    } while (more && clock_type::now() - t0 < std::chrono::milliseconds(30));
  } else {
    for (int i = 0; i < app.seams_per_frame && more; i++) more = app.session.step();
  }
  if (!more || app.session.done()) {
    app.playing = app.finishing = false;
    app.message = "Reached the target size.";
  }
}

void handle_shortcuts(app_state& app) {
  if (ImGui::GetIO().WantTextInput) return;  // typing into a slider
  if (ImGui::IsKeyPressed(ImGuiKey_O, false)) open_dialog(app);
  if (app.session.empty()) return;
  if (ImGui::IsKeyPressed(ImGuiKey_Space)) step_once(app);  // hold to repeat
  if (ImGui::IsKeyPressed(ImGuiKey_P, false)) toggle_play(app);
  if (ImGui::IsKeyPressed(ImGuiKey_F, false)) finish(app);
  if (ImGui::IsKeyPressed(ImGuiKey_R, false)) reset(app);
  if (ImGui::IsKeyPressed(ImGuiKey_E, false)) app.show_energy = !app.show_energy;
  if (ImGui::IsKeyPressed(ImGuiKey_S, false)) save(app);
}

// ---- UI ---------------------------------------------------------------------

void hint(const char* text) {
  ImGui::PushTextWrapPos(0.0f);
  ImGui::TextDisabled("%s", text);
  ImGui::PopTextWrapPos();
}

void stat_row(const char* label, const std::string& value) {
  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  ImGui::TextUnformatted(label);
  ImGui::TableNextColumn();
  ImGui::TextUnformatted(value.c_str());
}

std::string fmt(const char* format, double value) {
  char buf[64];
  std::snprintf(buf, sizeof(buf), format, value);
  return buf;
}

void draw_controls(app_state& app) {
  const float half = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2;

  // Image
  ImGui::SeparatorText("Image");
  if (ImGui::Button("Open image...  (O)", ImVec2(-FLT_MIN, 0))) open_dialog(app);
  hint("or drag an image file onto this window");
  if (!app.name.empty()) {
    ImGui::TextWrapped("%s  (%d x %d)", app.name.c_str(), app.full.cols, app.full.rows);
  }
  ImGui::TextUnformatted("Working size (longest side)");
  ImGui::SetNextItemWidth(-FLT_MIN);
  if (ImGui::Combo("##working", &app.working_size, kWorkingSizeNames,
                   IM_ARRAYSIZE(kWorkingSizeNames))) {
    apply_working_size(app);
    app.message = "Working size changed: restarted.";
  }
  hint("Large images are slow, especially with graph cut.");

  if (app.session.empty()) return;
  CarveSession& s = app.session;
  const cv::Mat& src = s.source();

  // Algorithm
  ImGui::SeparatorText("Algorithm");
  const int algo = s.algorithm();
  ImGui::SetNextItemWidth(-FLT_MIN);
  if (ImGui::BeginCombo("##algo", kAlgos[algo].name)) {
    for (int i = 0; i < kNumAlgos; i++) {
      if (ImGui::Selectable(kAlgos[i].name, i == algo) && i != algo) {
        s.set_algorithm(i);
        app.playing = app.finishing = false;
        app.message = std::string("Algorithm: ") + kAlgos[i].name + " (restarted).";
      }
      if (i == algo) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  hint(kAlgos[s.algorithm()].description);

  // Target size
  ImGui::SeparatorText("Target size");
  int w = s.target_width();
  int h = s.target_height();
  ImGui::Text("Width   %d of %d px (%.0f%%)", w, src.cols, 100.0 * w / src.cols);
  ImGui::SetNextItemWidth(-FLT_MIN);
  bool changed = ImGui::SliderInt("##width", &w, 1, src.cols, "%d px");
  bool active = ImGui::IsItemActive();
  ImGui::Text("Height  %d of %d px (%.0f%%)", h, src.rows, 100.0 * h / src.rows);
  ImGui::SetNextItemWidth(-FLT_MIN);
  changed |= ImGui::SliderInt("##height", &h, 1, src.rows, "%d px");
  active |= ImGui::IsItemActive();
  app.sliders_active = active;
  if (changed && s.set_target(w, h)) {
    app.playing = app.finishing = false;
    app.message = "Target needs pixels back: restarted from the original.";
  }
  hint("Ctrl+click a slider to type an exact value.");

  // Carving
  ImGui::SeparatorText("Carve");
  if (ImGui::Button("Step  (Space)", ImVec2(half, 0))) step_once(app);
  ImGui::SameLine();
  const bool paused_label = app.playing && !app.finishing;
  if (ImGui::Button(paused_label ? "Pause  (P)" : "Play  (P)", ImVec2(half, 0))) toggle_play(app);
  if (ImGui::Button("Finish  (F)", ImVec2(half, 0))) finish(app);
  ImGui::SameLine();
  if (ImGui::Button("Reset  (R)", ImVec2(half, 0))) reset(app);
  if (ImGui::Button("Save result  (S)", ImVec2(-FLT_MIN, 0))) save(app);
  ImGui::SetItemTooltip("Writes the result and the seam image as PNGs to the current folder.");

  ImGui::TextUnformatted("Seams per frame when playing");
  ImGui::SetNextItemWidth(-FLT_MIN);
  ImGui::SliderInt("##speed", &app.seams_per_frame, 1, 20);
  ImGui::Checkbox("Show energy map  (E)", &app.show_energy);

  const int removed = s.vertical_removed() + s.horizontal_removed();
  const int total = removed + s.seams_left();
  char progress[64];
  std::snprintf(progress, sizeof(progress), "%d / %d seams", removed, total);
  ImGui::ProgressBar(total > 0 ? static_cast<float>(removed) / total : 1.0f,
                     ImVec2(-FLT_MIN, 0), progress);

  // Statistics
  ImGui::SeparatorText("Statistics");
  if (ImGui::BeginTable("stats", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
    stat_row("Current size", std::to_string(s.result().cols) + " x " +
                                 std::to_string(s.result().rows));
    stat_row("Removed", std::to_string(s.vertical_removed()) + " vertical, " +
                            std::to_string(s.horizontal_removed()) + " horizontal");
    stat_row("Search time", fmt("%.1f ms total", s.search_ms()));
    ImGui::SetItemTooltip("Time inside the seam finder only, as in seam_carving.");
    stat_row("Last seam", fmt("%.2f ms", s.last_seam_ms()));
    stat_row("Removed energy", fmt("%.0f", s.removed_energy()));
    stat_row("Next seam energy", s.has_next() ? fmt("%.0f", s.next_cost()) : "-");
    ImGui::EndTable();
  }

  // Legend
  ImGui::SeparatorText("Legend");
  const ImGuiColorEditFlags swatch = ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop;
  const float sz = ImGui::GetTextLineHeight();
  ImGui::ColorButton("##red", kRed, swatch, ImVec2(sz, sz));
  ImGui::SameLine();
  ImGui::TextUnformatted("Removed vertical seam");
  ImGui::ColorButton("##yellow", kYellow, swatch, ImVec2(sz, sz));
  ImGui::SameLine();
  ImGui::TextUnformatted("Removed horizontal seam");
  ImGui::ColorButton("##cyan", kCyan, swatch, ImVec2(sz, sz));
  ImGui::SameLine();
  ImGui::TextUnformatted("Next seam to remove");
}

void draw_views(app_state& app) {
  const ImVec2 avail = ImGui::GetContentRegionAvail();
  CarveSession& s = app.session;

  if (s.empty()) {
    const char* text = "Drag an image file onto this window, or";
    const float tw = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPos(ImVec2((avail.x - tw) / 2, avail.y / 2 - ImGui::GetFrameHeight()));
    ImGui::TextUnformatted(text);
    const float bw = ImGui::CalcTextSize("Open image...").x + ImGui::GetStyle().FramePadding.x * 4;
    ImGui::SetCursorPosX((avail.x - bw) / 2);
    if (ImGui::Button("Open image...", ImVec2(bw, 0))) open_dialog(app);
    return;
  }

  if (s.version() != app.uploaded_version || app.show_energy != app.uploaded_energy) {
    app.left.upload(s.original_view());
    app.right.upload(s.current_view(app.show_energy));
    app.uploaded_version = s.version();
    app.uploaded_energy = app.show_energy;
  }

  // Both panels use the same zoom, so the result visibly shrinks.
  const float gap = ImGui::GetStyle().ItemSpacing.x * 3;
  const float label_h = ImGui::GetTextLineHeightWithSpacing();
  const float panel_w = (avail.x - gap) / 2;
  const float zoom = std::max(
      0.01f, std::min(panel_w / s.source().cols, (avail.y - label_h) / s.source().rows));

  ImGui::BeginGroup();
  ImGui::Text("Original + removed seams  (%d x %d)", s.source().cols, s.source().rows);
  ImGui::Image(app.left.imgui_id(), ImVec2(app.left.w * zoom, app.left.h * zoom));
  ImGui::EndGroup();

  ImGui::SameLine(0, gap);
  ImGui::BeginGroup();
  ImGui::Text("%s  (%d x %d)", app.show_energy ? "Energy map of result" : "Result",
              s.result().cols, s.result().rows);
  ImGui::Image(app.right.imgui_id(), ImVec2(app.right.w * zoom, app.right.h * zoom));
  ImGui::EndGroup();
}

void draw_ui(app_state& app, float ui_scale) {
  // One window filling the whole GLFW window: controls left, images right.
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);
  ImGui::Begin("##root", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

  const float status_h = ImGui::GetFrameHeightWithSpacing();
  ImGui::BeginChild("controls", ImVec2(340 * ui_scale, -status_h), ImGuiChildFlags_Borders);
  draw_controls(app);
  ImGui::EndChild();

  ImGui::SameLine();
  ImGui::BeginChild("views", ImVec2(0, -status_h));
  draw_views(app);
  ImGui::EndChild();

  // Status line
  if (app.playing) {
    ImGui::TextColored(kCyan, app.finishing ? "Finishing..." : "Playing...");
    ImGui::SameLine();
  }
  ImGui::TextUnformatted(app.message.c_str());
  ImGui::End();
}

void load_font(ImGuiIO& io, float scale) {
  const char* candidates[] = {
      "C:/Windows/Fonts/segoeui.ttf",
      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
      "/System/Library/Fonts/Supplemental/Arial.ttf",
  };
  for (const char* path : candidates) {
    std::error_code ec;
    if (fs::exists(path, ec) && io.Fonts->AddFontFromFileTTF(path, 17.0f * scale)) return;
  }
  ImFontConfig cfg;
  cfg.SizePixels = 13.0f * scale;
  io.Fonts->AddFontDefault(&cfg);
}

}  // namespace

int main(int argc, char** argv) {
  glfwSetErrorCallback([](int code, const char* desc) {
    std::fprintf(stderr, "GLFW error %d: %s\n", code, desc);
  });
  if (!glfwInit()) return 1;

  // OpenGL 3.0 + GLSL 130, as in the Dear ImGui GLFW/OpenGL3 example.
#ifdef __APPLE__
  const char* glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
  const char* glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

  // Start at 85% of the screen, centred. The window is freely resizable.
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  int mx = 0, my = 0, mw = 1280, mh = 800;
  if (monitor != nullptr) glfwGetMonitorWorkarea(monitor, &mx, &my, &mw, &mh);
  const int ww = mw * 85 / 100;
  const int wh = mh * 85 / 100;
  GLFWwindow* window = glfwCreateWindow(ww, wh, kTitle, nullptr, nullptr);
  if (window == nullptr) {
    glfwTerminate();
    return 1;
  }
  glfwSetWindowPos(window, mx + (mw - ww) / 2, my + (mh - wh) / 2);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);  // vsync

  app_state app;
  app.window = window;
  glfwSetWindowUserPointer(window, &app);
  glfwSetDropCallback(window, [](GLFWwindow* w, int count, const char** paths) {
    if (count > 0) {
      static_cast<app_state*>(glfwGetWindowUserPointer(w))->dropped = fs::u8path(paths[0]);
    }
  });

  // Dear ImGui, scaled for high-DPI screens.
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;  // don't write imgui.ini
  float ui_scale = 1.0f;
  glfwGetWindowContentScale(window, &ui_scale, nullptr);
  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowRounding = 0.0f;
  style.ChildRounding = 4.0f;
  style.FrameRounding = 4.0f;
  style.GrabRounding = 4.0f;
  style.GrabMinSize = 14.0f;
  style.ScaleAllSizes(ui_scale);
  load_font(io, ui_scale);
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  if (argc >= 2) load_file(app, fs::path(argv[1]));  // also: image dropped on the .exe

  while (!glfwWindowShouldClose(window)) {
    // Sleep until input arrives unless an animation is running.
    if (app.playing) {
      glfwPollEvents();
    } else {
      glfwWaitEventsTimeout(0.1);
    }

    if (!app.dropped.empty()) {
      const fs::path path = std::move(app.dropped);
      app.dropped.clear();
      load_file(app, path);
    }
    run_playback(app);
    // Preview the next seam when idle (not while dragging: graph cut is slow).
    if (!app.session.empty() && !app.playing && !app.sliders_active) {
      app.session.prepare_next();
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    handle_shortcuts(app);
    draw_ui(app, ui_scale);
    ImGui::Render();

    int fb_w = 0, fb_h = 0;
    glfwGetFramebufferSize(window, &fb_w, &fb_h);
    glViewport(0, 0, fb_w, fb_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  app.left.release();
  app.right.release();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
