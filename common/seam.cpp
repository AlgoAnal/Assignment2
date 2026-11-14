#include "seam.h"

#include <algorithm>
#include <chrono>

namespace 
{
  using clock_type = std::chrono::steady_clock;

  const cv::Vec3b kSeamColor(0, 0, 255);  // BGR red

  // Templated so the same code shrinks the image (Vec3b) and the "origin" map
  // (Vec2i) that records each pixel's (x, y) in the original image.
  template <typename T>
  cv::Mat remove_vertical_seam(const cv::Mat& src, const seam_t& seam) 
  {
    cv::Mat dst(src.rows, src.cols - 1, src.type());
    for (int r = 0; r < src.rows; r++) 
    {
      const T* in = src.ptr<T>(r);
      T* out = dst.ptr<T>(r);
      const int c = seam[r];
      std::copy(in, in + c, out);
      std::copy(in + c + 1, in + src.cols, out + c);
    }
    return dst;
  }

  // Removes `num_seams` vertical seams, painting each one on `marked`.
  cv::Mat carve_vertical(cv::Mat img, cv::Mat& origin, cv::Mat& marked, int num_seams, seam_finder find, run_stats& stats) 
  {
    for (int i = 0; i < num_seams; i++) 
    {
      const cv::Mat energy = compute_energy(img);

      float cost = 0.0f;
      const auto t0 = clock_type::now();
      const seam_t seam = find(energy, cost);
      const auto t1 = clock_type::now();

      stats.seam_search_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();
      stats.total_seam_energy += cost;
      stats.seams_removed++;

      for (int r = 0; r < img.rows; r++) 
      {
        const cv::Vec2i o = origin.at<cv::Vec2i>(r, seam[r]);
        marked.at<cv::Vec3b>(o[1], o[0]) = kSeamColor;
      }

      img = remove_vertical_seam<cv::Vec3b>(img, seam);
      origin = remove_vertical_seam<cv::Vec2i>(origin, seam);
    }
    return img;
  }
}

cv::Mat compute_energy(const cv::Mat& img) 
{
  cv::Mat gray, gx, gy;
  cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
  cv::Sobel(gray, gx, CV_32F, 1, 0, 3, 1.0, 0.0, cv::BORDER_REPLICATE);
  cv::Sobel(gray, gy, CV_32F, 0, 1, 3, 1.0, 0.0, cv::BORDER_REPLICATE);
  cv::Mat energy = cv::abs(gx) + cv::abs(gy);
  return energy;
}

// Transpose mat
cv::Mat transposed(const cv::Mat& m) 
{
  cv::Mat t;
  cv::transpose(m, t);
  return t;
}

cv::Mat seam_carve(const cv::Mat& src, int target_w, int target_h,seam_finder find, cv::Mat& marked, run_stats& stats) 
{
  cv::Mat img = src.clone();
  marked = src.clone();

  cv::Mat origin(src.size(), CV_32SC2);
  for (int y = 0; y < src.rows; y++) 
  {
    for (int x = 0; x < src.cols; x++) 
    {
      origin.at<cv::Vec2i>(y, x) = cv::Vec2i(x, y);
    }
  }

  img = carve_vertical(img, origin, marked, src.cols - target_w, find, stats);

  if (src.rows > target_h) 
  {
    img = transposed(img);
    origin = transposed(origin);
    img = carve_vertical(img, origin, marked, src.rows - target_h, find, stats);
    img = transposed(img);
  }
  return img;
}
