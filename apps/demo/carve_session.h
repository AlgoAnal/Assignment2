// Step-by-step seam carving for the interactive demo (no UI code here).
//
// Uses the same seam finders as seam_carving (src/common/seam.h), but removes
// one seam per step() so the UI can show every seam. Vertical seams are
// removed first, then horizontal ones (same order as seam_carve()).
#pragma once

#include "common/seam.h"

struct algo_choice {
  const char* name;
  const char* description;
  seam_finder find;
};
extern const algo_choice kAlgos[];
extern const int kNumAlgos;

class CarveSession {
 public:
  // Starts over with a new image. Target defaults to 75% width, full height.
  void load(const cv::Mat& src);
  bool empty() const { return src_.empty(); }

  void reset();                  // back to the original image
  void set_algorithm(int index); // restarts
  // Shrinking further continues from the current image. Anything that would
  // need pixels back (or vertical seams after horizontal ones) restarts.
  // Returns true if it had to restart.
  bool set_target(int width, int height);

  // Finds (but does not remove) the next seam so the views can preview it.
  // step() calls it too; the UI calls it when idle.
  void prepare_next();
  bool step();  // removes the next seam; false if already at the target
  bool done() const;

  // Original with every removed seam painted (red = vertical,
  // yellow = horizontal) and the next seam in cyan.
  cv::Mat original_view() const;
  // Current carved image (or its energy map) with the next seam in cyan.
  cv::Mat current_view(bool show_energy) const;

  // Changes whenever the views would look different.
  int version() const { return version_; }

  const cv::Mat& source() const { return src_; }
  const cv::Mat& result() const { return img_; }
  const cv::Mat& marked() const { return marked_; }
  int algorithm() const { return algo_; }
  int target_width() const { return target_w_; }
  int target_height() const { return target_h_; }
  int vertical_removed() const { return v_removed_; }
  int horizontal_removed() const { return h_removed_; }
  int seams_left() const;
  double search_ms() const { return search_ms_; }  // time inside the finder only
  double last_seam_ms() const { return last_ms_; }
  double removed_energy() const { return total_energy_; }
  bool has_next() const { return next_.valid; }
  float next_cost() const { return next_.cost; }

 private:
  // The seam that will be removed next, found ahead of time so it can be shown.
  struct pending_seam {
    bool valid = false;
    bool vertical = true;  // false: seam is on the transposed image
    seam_t seam;
    float cost = 0.0f;
    double ms = 0.0;
  };

  template <typename F>
  void for_each_next_pixel(F f) const;

  cv::Mat src_;     // input image
  cv::Mat img_;     // current carved image
  cv::Mat origin_;  // CV_32SC2: (x, y) in src_ of every pixel of img_
  cv::Mat marked_;  // src_ with every removed seam painted
  int algo_ = 2;    // dp
  int target_w_ = 1;
  int target_h_ = 1;
  int v_removed_ = 0;
  int h_removed_ = 0;
  double search_ms_ = 0.0;
  double last_ms_ = 0.0;
  double total_energy_ = 0.0;
  pending_seam next_;
  int version_ = 0;
};
