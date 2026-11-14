// shared header used for all the other algos
// each function has its own .cpp file
#pragma once

#include <opencv2/opencv.hpp>

#include <limits>
#include <vector>

using seam_t = std::vector<int>;

// energy: CV_32F, rows x cols (from compute_energy).
// total_cost: set to the sum of energy along the returned seam.
using seam_finder = seam_t (*)(const cv::Mat& energy, float& total_cost);

struct algo_entry {
  const char* name;
  seam_finder find;
};

struct run_stats {
  double seam_search_ms = 0.0;     // time spent ONLY inside the seam finder
  double total_seam_energy = 0.0;  // sum of energies of all removed seams
  int seams_removed = 0;
};

constexpr float kInfinity = std::numeric_limits<float>::max();

// ---- Shared functions (def in seams.cpp) -----------------------------------

// |Gx| + |Gy| of the grayscale image using Sobel. All algorithms must use this
// so total-energy numbers are comparable.
cv::Mat compute_energy(const cv::Mat& img);

cv::Mat transposed(const cv::Mat& m);

// Shrinks `src` to target_w x target_h using `find`. Vertical seams first,
// then horizontal seams (on the transposed image).
// `marked` receives a copy of the original with every removed seam in red.
cv::Mat seam_carve(const cv::Mat& src, int target_w, int target_h,
                   seam_finder find, cv::Mat& marked, run_stats& stats);

// ---- Algos --------------------------------------

// seam_greedy.cpp
seam_t find_seam_greedy(const cv::Mat& energy, float& total_cost);
seam_t find_seam_greedy_all_starts(const cv::Mat& energy, float& total_cost);

// seam_dp.cpp
seam_t find_seam_dp(const cv::Mat& energy, float& total_cost);

// seam_graphcut.cpp
seam_t find_seam_graphcut(const cv::Mat& energy, float& total_cost);
