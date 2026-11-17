#include "file_dialog.h"

#include <opencv2/imgcodecs.hpp>

#include <fstream>
#include <iterator>
#include <vector>

namespace fs = std::filesystem;

cv::Mat read_image(const fs::path& path) {
  // Read the bytes ourselves and decode them: std::ifstream takes a
  // filesystem::path, so non-ASCII folder names work on Windows too.
  std::ifstream in(path, std::ios::binary);
  if (!in) return cv::Mat();
  const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)),
                                         std::istreambuf_iterator<char>());
  if (bytes.empty()) return cv::Mat();
  return cv::imdecode(bytes, cv::IMREAD_COLOR);
}

#ifdef _WIN32
// ---- Windows: system Open dialog -------------------------------------------
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>

fs::path browse_for_image(std::string& /*error*/) {
  wchar_t file[MAX_PATH * 4] = L"";
  OPENFILENAMEW ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = GetActiveWindow();  // the demo window
  ofn.lpstrFilter =
      L"Images (*.png;*.jpg;*.jpeg;*.bmp;*.tif;*.tiff)\0*.png;*.jpg;*.jpeg;*.bmp;*.tif;*.tiff\0"
      L"All files (*.*)\0*.*\0";
  ofn.lpstrFile = file;
  ofn.nMaxFile = static_cast<DWORD>(std::size(file));
  ofn.lpstrTitle = L"Open an image to carve";
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

  // Start in ./images when run from the repo folder.
  std::error_code ec;
  const fs::path images = fs::current_path(ec) / "images";
  const std::wstring initial = images.wstring();
  if (!ec && fs::is_directory(images, ec)) ofn.lpstrInitialDir = initial.c_str();

  if (!GetOpenFileNameW(&ofn)) return fs::path();  // cancelled
  return fs::path(file);
}

#else
// ---- Linux / macOS: zenity -------------------------------------------------
#include <sys/wait.h>

#include <cstdio>

fs::path browse_for_image(std::string& error) {
  FILE* pipe = popen(
      "zenity --file-selection --title='Open an image to carve' "
      "--file-filter='Images | *.png *.jpg *.jpeg *.bmp *.tif *.tiff' "
      "--file-filter='All files | *' 2>/dev/null",
      "r");
  if (pipe == nullptr) {
    error = "Could not run zenity";
    return fs::path();
  }
  std::string out;
  char buf[512];
  while (std::fgets(buf, sizeof(buf), pipe) != nullptr) out += buf;
  const int status = pclose(pipe);

  while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
  // zenity exits with 1 on Cancel; 127 from the shell means "not installed".
  if (out.empty() && WIFEXITED(status) && WEXITSTATUS(status) == 127) {
    error = "No file dialog: install zenity, or drag an image onto the window";
  }
  return fs::u8path(out);
}
#endif
