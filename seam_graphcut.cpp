// GAME PLAN: Turn the energy image into a flow network whose minimum S-T cut
// is exactly one connected vertical seam, then solve it with max flow.
// Graph from Rubinstein, Shamir & Avidan (2008), "Improved Seam Carving for
// Video Retargeting".
//
// Edges (S = source, T = sink):
//   S -> (r, 0)            inf          left side of every row is on S's side
//   (r, c) -> (r, c+1)     energy(r,c)  cutting it removes pixel (r, c)
//   (r, cols-1) -> T       energy       so the last column can be removed too
//   (r, c+1) -> (r, c)     inf          at most one cut per row
//   (r, c) -> (r+-1, c-1)  inf          seam moves at most 1 column per row

#include "./common/seam.h"
#include "MinCutMaxFlow.h"

namespace
{
  using mincut::ResidualGraph;
  using mincut::addEdge;
  using mincut::kInf;

  using max_flow_fn = double (*)(ResidualGraph&, int, int);

  // pixel (r, c) -> node id. S and T come after all pixels.
  int node(int r, int c, int cols) { return r * cols + c; }

  ResidualGraph build_seam_graph(const cv::Mat& energy, int& src, int& sink)
  {
    const int rows = energy.rows;
    const int cols = energy.cols;
    ResidualGraph graph(rows * cols + 2);
    src = rows * cols;
    sink = src + 1;

    for (int r = 0; r < rows; r++)
    {
      const float* e = energy.ptr<float>(r);
      addEdge(graph, src, node(r, 0, cols), kInf);

      for (int c = 0; c < cols; c++)
      {
        const int v = node(r, c, cols);
        const bool last = (c + 1 == cols);
        const int right = last ? sink : node(r, c + 1, cols);

        // 1. Cutting this edge removes pixel (r, c) -> costs its energy.
        addEdge(graph, v, right, e[c]);

        // 2. Infinite edge back: a row can only be cut once.
        if (!last) addEdge(graph, right, v, kInf);

        // 3. Infinite diagonals: seam moves at most 1 column per row.
        if (c > 0)
        {
          if (r > 0)        addEdge(graph, v, node(r - 1, c - 1, cols), kInf);
          if (r + 1 < rows) addEdge(graph, v, node(r + 1, c - 1, cols), kInf);
        }
      }
    }
    return graph;
  }

  seam_t find_seam_with(const cv::Mat& energy, float& total_cost, max_flow_fn maxFlow)
  {
    CV_Assert(energy.type() == CV_32F && energy.rows > 0 && energy.cols > 0);

    int src = 0;
    int sink = 0;
    ResidualGraph graph = build_seam_graph(energy, src, sink);
    maxFlow(graph, src, sink);

    std::vector<bool> sourceSide;
    mincut::findReachable(graph, src, sourceSide);

    // Each row's source side is a prefix 0..c; the seam pixel is its end.
    seam_t seam(energy.rows);
    total_cost = 0.0f;
    for (int r = 0; r < energy.rows; r++)
    {
      int c = 0;
      while (c + 1 < energy.cols && sourceSide[node(r, c + 1, energy.cols)]) c++;
      seam[r] = c;
      total_cost += energy.at<float>(r, c);
    }
    return seam;
  }
}

// Dinic, default
seam_t find_seam_graphcut(const cv::Mat& energy, float& total_cost)
{
  return find_seam_with(energy, total_cost, mincut::dinic);
}

// Edmonds-Karp graphcut
seam_t find_seam_graphcut_ek(const cv::Mat& energy, float& total_cost)
{
  return find_seam_with(energy, total_cost, mincut::edmondsKarp);
}
