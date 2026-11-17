// Opening images for the demo. Platform code, not part of the algorithms.
// (Drag-and-drop is handled by GLFW in main.cpp.)
#pragma once

#include <opencv2/core.hpp>

#include <filesystem>
#include <string>

// Shows an "Open image" dialog (Windows: system dialog, Linux: zenity).
// Returns an empty path if cancelled; sets `error` if no dialog is available.
std::filesystem::path browse_for_image(std::string& error);

// Reads an image as 8-bit BGR. Unicode-safe on Windows, unlike cv::imread.
// Returns an empty Mat on failure.
cv::Mat read_image(const std::filesystem::path& path);
