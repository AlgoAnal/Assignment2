// Small stand-alone test for MinCutMaxFlow (not part of seam_carving).
// Runs Edmonds-Karp and Dinic on the same toy graph; both must agree, and the
// min cut must equal the max flow.

#include "MinCutMaxFlow.h"

#include <iostream>
#include <vector>

using namespace mincut;

namespace {

void run(const char* label, double (*maxFlowFn)(ResidualGraph&, int, int),
         ResidualGraph graph, int src, int sink) {  // graph copied on purpose
  const double maxFlow = maxFlowFn(graph, src, sink);

  std::vector<bool> visited;
  findReachable(graph, src, visited);
  const double minCut = findMinCut(graph, visited);

  printGraph(graph, std::string(label) + ": flow / capacity");
  std::cout << "Maximum Flow: " << maxFlow << '\n';
  std::cout << "Minimum Cut:  " << minCut << '\n';
  std::cout << "Source side:  {";
  for (int v = 0; v < static_cast<int>(visited.size()); v++) {
    if (visited[v]) std::cout << ' ' << v;
  }
  std::cout << " }\n";
}

}  // namespace

int main() {
  // Same 4-node example as the first (adjacency matrix) draft.
  ResidualGraph graph(4);
  addEdge(graph, 0, 1, 10);
  addEdge(graph, 0, 2, 5);
  addEdge(graph, 1, 3, 4);
  addEdge(graph, 2, 3, 8);

  const int src = 0;
  const int sink = 3;

  run("Edmonds-Karp", edmondsKarp, graph, src, sink);
  run("Dinic", dinic, graph, src, sink);
}
