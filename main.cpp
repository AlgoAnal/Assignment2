
#include "./common/seam.h"

#include <exception>
#include <iomanip>
#include <iostream>
#include <string>


#include <exception>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

// To add an algorithm: declare it in seam_common.h, add a row here (guarded
// by its own macro if the file is optional) and add its .cpp to the build.
const algo_entry kAlgos[] = {
    {"greedy", find_seam_greedy},
    {"greedy_all_starts", find_seam_greedy_all_starts},
#ifdef SEAM_HAVE_DP
    {"dp", find_seam_dp},
#endif
#ifdef SEAM_HAVE_GRAPHCUT
    {"graphcut", find_seam_graphcut},
    // {"graphcut_ek", find_seam_graphcut_ek},  // Edmonds-Karp: slow, disabled
#endif
};

#ifdef SEAM_HAVE_DP
// Compares the first vertical seam of the original image: how much worse is
// each other algorithm than DP, and how many rows coincide with the DP seam?
void compare_first_seam(const cv::Mat& src) {
  const cv::Mat energy = compute_energy(src);
  float dp_cost = 0.0f;
  const seam_t dp = find_seam_dp(energy, dp_cost);

  std::cout << "\nFirst vertical seam (original image), vs dp (" << dp_cost
            << ")\n";
  for (const algo_entry& algo : kAlgos) {
    if (std::string(algo.name) == "dp") continue;
    try {
      float cost = 0.0f;
      const seam_t seam = algo.find(energy, cost);
      int same = 0;
      for (int r = 0; r < energy.rows; r++) {
        if (seam[r] == dp[r]) same++;
      }
      std::cout << "  " << std::left << std::setw(20) << algo.name
                << "energy " << std::setw(10) << cost << " ("
                << cost / dp_cost << "x dp, " << 100.0 * same / energy.rows
                << "% rows match)\n";
    } catch (const std::exception&) {
      // not implemented yet; already reported in the main table
    }
  }
}
#endif

}  // namespace

int main(int argc, char** argv) {
  if (argc != 5) {
    std::cerr << "usage: " << argv[0]
              << " input_image out_prefix target_width target_height\n";
    return 1;
  }

  const cv::Mat src = cv::imread(argv[1], cv::IMREAD_COLOR);
  if (src.empty()) {
    std::cerr << "could not read " << argv[1] << "\n";
    return 1;
  }
  const std::string prefix = argv[2];
  const int target_w = std::stoi(argv[3]);
  const int target_h = std::stoi(argv[4]);

  if (target_w < 1 || target_h < 1 || target_w > src.cols ||
      target_h > src.rows) {
    std::cerr << "target must be between 1x1 and " << src.cols << "x"
              << src.rows << " (this implementation only shrinks)\n";
    return 1;
  }

  std::cout << "input " << src.cols << "x" << src.rows << " -> " << target_w
            << "x" << target_h << "\n\n"
            << std::left << std::setw(20) << "algorithm" << std::setw(8)
            << "seams" << std::setw(18) << "search time (ms)"
            << "total seam energy\n";

  for (const algo_entry& algo : kAlgos) {
    try {
      cv::Mat marked;
      run_stats stats;
      const cv::Mat result =
          seam_carve(src, target_w, target_h, algo.find, marked, stats);

      std::cout << std::left << std::setw(20) << algo.name << std::setw(8)
                << stats.seams_removed << std::setw(18) << std::fixed
                << std::setprecision(2) << stats.seam_search_ms
                << stats.total_seam_energy << "\n";

      const std::string stem = prefix + "_" + algo.name;
      cv::imwrite(stem + ".png", result);
      cv::imwrite(stem + "_seams.png", marked);
    } catch (const std::exception& e) {
      std::cout << std::left << std::setw(20) << algo.name << "skipped: "
                << e.what() << "\n";
    }
  }

#ifdef SEAM_HAVE_DP
  compare_first_seam(src);
#else
  std::cout << "\n(dp not built: skipping the greedy-vs-dp seam comparison)\n";
#endif
  return 0;
}