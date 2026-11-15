#include "MinCutMaxFlow.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <queue>

namespace mincut {

void addEdge(ResidualGraph& graph, int u, int v, double cap) {
  // Each edge stores where its partner lives, so pushing flow can update the
  // reverse residual capacity in O(1) (the matrix version used graph[v][u]).
  graph[u].push_back({v, static_cast<int>(graph[v].size()), cap, cap});
  graph[v].push_back({u, static_cast<int>(graph[u].size()) - 1, 0.0, 0.0});
}

// edmonds karp


// runs during Max-Flow
/*
-Searches for a path from S to T.
-Records the path using parent (node) and parentEdge (which edge was used).
-Returns true immediately when T is discovered.
-Returns false if T cannot be reached.
*/
bool bfs(const ResidualGraph& graph, int src, int sink, std::vector<int>& parent,
         std::vector<int>& parentEdge) {
  const int n = static_cast<int>(graph.size());
  std::vector<bool> visited(n, false);
  std::queue<int> q;

  q.push(src);
  visited[src] = true;

  // -1 means no predecessor
  parent.assign(n, -1);
  parentEdge.assign(n, -1);

  while (!q.empty()) {
    const int current = q.front();
    q.pop();

    for (int e = 0; e < static_cast<int>(graph[current].size()); e++) {
      const Edge& edge = graph[current][e];
      const int next = edge.to;

      if (!visited[next] && edge.cap > kEps) {
        visited[next] = true;
        parent[next] = current;
        parentEdge[next] = e;

        if (next == sink) return true;

        q.push(next);
      }
    }
  }

  return false;
}

double findPathFlow(const ResidualGraph& graph, const std::vector<int>& parent,
                    const std::vector<int>& parentEdge, int src, int sink) {
  double pathFlow = kInf;

  for (int v = sink; v != src; v = parent[v]) {
    const int u = parent[v];
    pathFlow = std::min(pathFlow, graph[u][parentEdge[v]].cap);
  }

  return pathFlow;
}

void updateResidualGraph(ResidualGraph& graph, const std::vector<int>& parent,
                         const std::vector<int>& parentEdge, int src, int sink,
                         double pathFlow) {
  for (int v = sink; v != src; v = parent[v]) {
    const int u = parent[v];
    Edge& forward = graph[u][parentEdge[v]];

    // Update forward residual capacity
    forward.cap -= pathFlow;

    // Update reverse residual capacity
    graph[v][forward.rev].cap += pathFlow;
  }
}

double edmondsKarp(ResidualGraph& graph, int src, int sink) {
  std::vector<int> parent;
  std::vector<int> parentEdge;
  double maxFlow = 0.0;

  while (bfs(graph, src, sink, parent, parentEdge)) {
    // trace back from the sink to the source
    const double pathFlow = findPathFlow(graph, parent, parentEdge, src, sink);
    if (pathFlow == kInf) return kInf;  // path of only infinite edges
    updateResidualGraph(graph, parent, parentEdge, src, sink, pathFlow);
    maxFlow += pathFlow;
  }

  return maxFlow;
}

// dinic


/*
-Same BFS as Edmonds-Karp, but does NOT stop at T and records the distance
 (level) of every node instead of a single path.
-Only edges going from level L to level L+1 are used by blockingFlow.
*/
bool bfsLevel(const ResidualGraph& graph, int src, int sink,
              std::vector<int>& level) {
  level.assign(graph.size(), -1);
  std::queue<int> q;

  q.push(src);
  level[src] = 0;

  while (!q.empty()) {
    const int current = q.front();
    q.pop();

    for (const Edge& edge : graph[current]) {
      if (level[edge.to] < 0 && edge.cap > kEps) {
        level[edge.to] = level[current] + 1;
        q.push(edge.to);
      }
    }
  }

  return level[sink] >= 0;
}

/*
-Iterative DFS (no recursion: a pixel graph can have paths thousands of nodes
 long, which could overflow the call stack, especially on Windows' 1 MB stack).
-Walks forward along level-graph edges. On reaching T it pushes the bottleneck
 flow, then backs up only to the first edge that became full.
-On a dead end it backs up one node and skips that edge for the rest of the
 phase (next[u]++), so every edge is abandoned at most once per phase.
*/
double blockingFlow(ResidualGraph& graph, int src, int sink,
                    const std::vector<int>& level, std::vector<int>& next) {
  double total = 0.0;
  std::vector<int> pathNodes{src};  // nodes on the current DFS path
  std::vector<int> pathEdges;       // pathEdges[k]: edge index in pathNodes[k]

  while (true) {
    const int u = pathNodes.back();

    if (u == sink) {
      // Bottleneck along the path
      double pathFlow = kInf;
      for (std::size_t k = 0; k < pathEdges.size(); k++) {
        pathFlow = std::min(pathFlow, graph[pathNodes[k]][pathEdges[k]].cap);
      }
      if (pathFlow == kInf) return kInf;  // path of only infinite edges

      // Push it, and remember the first edge that is now full
      std::size_t firstFull = pathEdges.size();
      for (std::size_t k = 0; k < pathEdges.size(); k++) {
        Edge& forward = graph[pathNodes[k]][pathEdges[k]];
        forward.cap -= pathFlow;
        graph[forward.to][forward.rev].cap += pathFlow;
        if (firstFull == pathEdges.size() && forward.cap <= kEps) firstFull = k;
      }
      total += pathFlow;

      // Retreat to the tail of the first full edge; that edge will now be
      // skipped by the advance loop because its capacity is used up.
      pathNodes.resize(firstFull + 1);
      pathEdges.resize(firstFull);
      continue;
    }

    // Advance along the first usable level-graph edge
    bool advanced = false;
    for (; next[u] < static_cast<int>(graph[u].size()); next[u]++) {
      const Edge& edge = graph[u][next[u]];
      if (edge.cap > kEps && level[edge.to] == level[u] + 1) {
        pathEdges.push_back(next[u]);
        pathNodes.push_back(edge.to);
        advanced = true;
        break;
      }
    }
    if (advanced) continue;

    // Dead end
    if (u == src) break;  // nothing more can leave the source this phase
    pathNodes.pop_back();
    pathEdges.pop_back();
    next[pathNodes.back()]++;  // never try the edge into u again this phase
  }

  return total;
}

double dinic(ResidualGraph& graph, int src, int sink) {
  std::vector<int> level;
  std::vector<int> next;
  double maxFlow = 0.0;

  while (bfsLevel(graph, src, sink, level)) {
    next.assign(graph.size(), 0);
    maxFlow += blockingFlow(graph, src, sink, level, next);
    if (maxFlow == kInf) return kInf;
  }

  return maxFlow;
}

// ============================================================================
// Min cut
// ============================================================================

// runs after Max-Flow has finished
/*
-Starts searching from S.
-Explores every vertex reachable through positive residual capacity.
-Doesn't need to record parents.
-Doesn't stop early at T.
-Produces the visited array used to identify the minimum cut.
*/
void findReachable(const ResidualGraph& graph, int src,
                   std::vector<bool>& visited) {
  visited.assign(graph.size(), false);
  std::queue<int> q;

  q.push(src);
  visited[src] = true;

  while (!q.empty()) {
    const int current = q.front();
    q.pop();

    // Explore neighbours here
    for (const Edge& edge : graph[current]) {
      if (!visited[edge.to] && edge.cap > kEps) {
        visited[edge.to] = true;
        q.push(edge.to);
      }
    }
  }
}

double findMinCut(const ResidualGraph& graph, const std::vector<bool>& visited) {
  double minCutCapacity = 0.0;
  for (int u = 0; u < static_cast<int>(graph.size()); u++) {
    if (!visited[u]) continue;
    for (const Edge& edge : graph[u]) {
      // original > 0 skips the reverse edges
      if (!visited[edge.to] && edge.original > 0.0) {
        minCutCapacity += edge.original;
      }
    }
  }
  return minCutCapacity;
}

// Debug
void printGraph(const ResidualGraph& graph, const std::string& name) {
  std::cout << "\n=== " << name << " ===\n";
  for (int u = 0; u < static_cast<int>(graph.size()); u++) {
    for (const Edge& edge : graph[u]) {
      if (edge.original <= 0.0) continue;  // hide reverse edges
      std::cout << std::setw(4) << u << " -> " << std::setw(4) << edge.to
                << " : " << std::setw(6) << edge.original - edge.cap << " / "
                << edge.original << '\n';
    }
  }
}

}
