#include "carve_session.h"

#include <algorithm>
#include <chrono>
#include <utility>

const algo_choice kAlgos[] = {
    {"Greedy",
     "Row by row, step to the cheapest of the 3 pixels below.\n"
     "O(rows) per seam, not optimal.",
     find_seam_greedy},
    {"Greedy (all starts)",
     "Greedy from every top-row pixel, keep the cheapest.\n"
     "O(rows x cols) per seam, still not optimal.",
     find_seam_greedy_all_starts},
    {"Dynamic programming",
     "M[i][j] = e[i][j] + min(M[i-1][j-1], M[i-1][j], M[i-1][j+1]).\n"
     "O(rows x cols) per seam, optimal.",
     find_seam_dp},
    {"Graph cut (Dinic)",
     "Minimum s-t cut of the pixel graph via max flow.\n"
     "Optimal, but much slower than DP.",
     find_seam_graphcut},
    {"Graph cut (Edmonds-Karp)",
     "Minimum s-t cut via BFS shortest augmenting paths.\n"
     "Optimal, but far slower than Dinic: use a small image.",
     find_seam_graphcut_ek},
};
const int kNumAlgos = static_cast<int>(sizeof(kAlgos) / sizeof(kAlgos[0]));

namespace {

using clock_type = std::chrono::steady_clock;

const cv::Vec3b kVertColor(0, 0, 255);    // red    (BGR)
const cv::Vec3b kHorzColor(0, 255, 255);  // yellow
const cv::Vec3b kNextColor(255, 255, 0);  // cyan

// Removes one vertical seam. Templated so it works on the image (Vec3b) and
// on the origin map (Vec2i).
template <typename T>
cv::Mat remove_seam(const cv::Mat& src, const seam_t& seam) {
  cv::Mat dst(src.rows, src.cols - 1, src.type());
  for (int r = 0; r < src.rows; r++) {
    const T* in = src.ptr<T>(r);
    T* out = dst.ptr<T>(r);
    const int c = seam[r];
    std::copy(in, in + c, out);
    std::copy(in + c + 1, in + src.cols, out + c);
  }
  return dst;
}

}  // namespace

void CarveSession::load(const cv::Mat& src) {
  src_ = src.clone();
  target_w_ = std::max(1, src_.cols * 3 / 4);
  target_h_ = src_.rows;
  reset();
}

void CarveSession::reset() {
  img_ = src_.clone();
  marked_ = src_.clone();
  origin_.create(src_.size(), CV_32SC2);
  for (int y = 0; y < src_.rows; y++) {
    for (int x = 0; x < src_.cols; x++) {
      origin_.at<cv::Vec2i>(y, x) = cv::Vec2i(x, y);
    }
  }
  v_removed_ = h_removed_ = 0;
  search_ms_ = last_ms_ = total_energy_ = 0.0;
  next_.valid = false;
  version_++;
}

void CarveSession::set_algorithm(int index) {
  algo_ = std::clamp(index, 0, kNumAlgos - 1);
  reset();
}

bool CarveSession::set_target(int width, int height) {
  width = std::clamp(width, 1, src_.cols);
  height = std::clamp(height, 1, src_.rows);
  if (width == target_w_ && height == target_h_) return false;

  const bool can_continue = width <= img_.cols && height <= img_.rows &&
                            (h_removed_ == 0 || width == img_.cols);
  target_w_ = width;
  target_h_ = height;
  next_.valid = false;  // direction of the next seam may have changed
  version_++;
  if (!can_continue) reset();
  return !can_continue;
}

bool CarveSession::done() const {
  return img_.cols <= target_w_ && img_.rows <= target_h_;
}

int CarveSession::seams_left() const {
  return std::max(0, img_.cols - target_w_) + std::max(0, img_.rows - target_h_);
}

void CarveSession::prepare_next() {
  if (next_.valid || empty() || done()) return;
  const bool vertical = img_.cols > target_w_;
  const cv::Mat energy = compute_energy(vertical ? img_ : transposed(img_));

  float cost = 0.0f;
  const auto t0 = clock_type::now();
  seam_t seam = kAlgos[algo_].find(energy, cost);
  const auto t1 = clock_type::now();

  next_.valid = true;
  next_.vertical = vertical;
  next_.seam = std::move(seam);
  next_.cost = cost;
  next_.ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
  version_++;
}

// Calls f(y, x) for every pixel of the pending seam, in image coordinates.
template <typename F>
void CarveSession::for_each_next_pixel(F f) const {
  if (!next_.valid) return;
  if (next_.vertical) {
    for (int y = 0; y < img_.rows; y++) f(y, next_.seam[y]);
  } else {
    for (int x = 0; x < img_.cols; x++) f(next_.seam[x], x);
  }
}

bool CarveSession::step() {
  prepare_next();
  if (!next_.valid) return false;

  const cv::Vec3b color = next_.vertical ? kVertColor : kHorzColor;
  for_each_next_pixel([&](int y, int x) {
    const cv::Vec2i o = origin_.at<cv::Vec2i>(y, x);
    marked_.at<cv::Vec3b>(o[1], o[0]) = color;
  });

  if (next_.vertical) {
    img_ = remove_seam<cv::Vec3b>(img_, next_.seam);
    origin_ = remove_seam<cv::Vec2i>(origin_, next_.seam);
    v_removed_++;
  } else {
    img_ = transposed(remove_seam<cv::Vec3b>(transposed(img_), next_.seam));
    origin_ = transposed(remove_seam<cv::Vec2i>(transposed(origin_), next_.seam));
    h_removed_++;
  }
  search_ms_ += next_.ms;
  last_ms_ = next_.ms;
  total_energy_ += next_.cost;
  next_.valid = false;
  version_++;
  return true;
}

cv::Mat CarveSession::original_view() const {
  cv::Mat view = marked_.clone();
  for_each_next_pixel([&](int y, int x) {
    const cv::Vec2i o = origin_.at<cv::Vec2i>(y, x);
    view.at<cv::Vec3b>(o[1], o[0]) = kNextColor;
  });
  return view;
}

cv::Mat CarveSession::current_view(bool show_energy) const {
  cv::Mat view;
  if (show_energy) {
    cv::Mat e8;
    cv::normalize(compute_energy(img_), e8, 0, 255, cv::NORM_MINMAX, CV_8U);
    cv::cvtColor(e8, view, cv::COLOR_GRAY2BGR);
  } else {
    view = img_.clone();
  }
  for_each_next_pixel([&](int y, int x) { view.at<cv::Vec3b>(y, x) = kNextColor; });
  return view;
}
