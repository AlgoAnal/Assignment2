// GAME PLAN: Row-by-row: from the current pixel, move to the cheapest of the three
// neighbours in the next row. No look-ahead, no backtracking.

#include "common/seam.h"
#include <algorithm>
#include <utility>

namespace 
{
  // Follows the greedy rule from `start_col` in the top row.
  seam_t trace_greedy(const cv::Mat& energy, int start_col, float& total_cost) 
  {
    // Edge cases for empty energy or invalid start pos
    CV_Assert(energy.type() == CV_32F && energy.rows > 0 && energy.cols > 0);
    CV_Assert(start_col >= 0 && start_col < energy.cols);

    const int rows = energy.rows;
    const int cols = energy.cols;
    seam_t seam(rows);

    int col = start_col;
    seam[0] = col;
    total_cost = energy.at<float>(0, col);

    for (int r = 1; r < rows; r++) 
    {
      const float* row = energy.ptr<float>(r);
      const int lo = std::max(col - 1, 0);
      const int hi = std::min(col + 1, cols - 1);

      int best = col;  // start from "straight down" so ties go straight
      for (int c = lo; c <= hi; c++) 
      {
        if (row[c] < row[best]) 
          best = c; 
      }

      col = best;
      seam[r] = col;
      total_cost += row[col];
    }
    return seam;
  }
} 

// Starts at the cheapest pixel in the top row. 
seam_t find_seam_greedy(const cv::Mat& energy, float& total_cost) 
{
  const float* top = energy.ptr<float>(0);
  int start = 0;

  for (int c = 1; c < energy.cols; c++) 
  {
    if (top[c] < top[start]) start = c;
  }

  return trace_greedy(energy, start, total_cost);
}

// Improvement: run the greedy trace from every top-row pixel and keep the cheapest. O(rows * cols) per seam, still no full DP table.
seam_t find_seam_greedy_all_starts(const cv::Mat& energy, float& total_cost)
{
  float best_cost{};
  seam_t best_seam = trace_greedy(energy, 0, best_cost);  // always non-empty

  for (int start = 1; start < energy.cols; start++)
  {
    float cost = 0.0f;
    seam_t seam = trace_greedy(energy, start, cost);
    if (cost < best_cost)
    {
      best_cost = cost;
      best_seam = std::move(seam);
    }
  }

  total_cost = best_cost;
  return best_seam;
}
