#pragma once

#include <limits>
#include <string>
#include <vector>

// Generic max-flow / min-cut on a directed graph stored as an ADJACENCY LIST.
//
// Why not an adjacency matrix (first draft, commit a003de4)?
//   A pixel graph has V = rows*cols nodes, so a V x V matrix needs V^2 cells
//   (500x500 image -> 6.25e10 ints). Each pixel only has ~4 edges, so an
//   adjacency list needs O(V + E) memory instead.
//
// Capacities are double because the image energy is a float.

namespace mincut {

constexpr double kInf = std::numeric_limits<double>::infinity();
constexpr double kEps = 1e-6;  // residual capacity <= kEps counts as "full"

struct Edge {
  int to;           // head of the edge
  int rev;          // index of the reverse edge inside graph[to]
  double cap;       // residual capacity (changes while running max-flow)
  double original;  // capacity given to addEdge (0 for reverse edges)
};

// graph[u] = every edge leaving u (forward edges and their reverse edges).
using ResidualGraph = std::vector<std::vector<Edge>>;

// Adds u -> v with capacity `cap`, plus the reverse edge v -> u with 0.
void addEdge(ResidualGraph& graph, int u, int v, double cap);

// ---- Edmonds-Karp: Ford-Fulkerson with BFS shortest augmenting paths -------
// O(V * E^2). Each BFS finds ONE path, then the whole search restarts.

// Searches for a path from src to sink. parent[v] = previous node,
// parentEdge[v] = index of the edge used inside graph[parent[v]].
bool bfs(const ResidualGraph& graph, int src, int sink, std::vector<int>& parent,
         std::vector<int>& parentEdge);

double findPathFlow(const ResidualGraph& graph, const std::vector<int>& parent,
                    const std::vector<int>& parentEdge, int src, int sink);

void updateResidualGraph(ResidualGraph& graph, const std::vector<int>& parent,
                         const std::vector<int>& parentEdge, int src, int sink,
                         double pathFlow);

// Runs the bfs -> findPathFlow -> updateResidualGraph loop. Returns max flow.
// `graph` is left as the final residual graph.
// Both edmondsKarp and dinic return kInf (and stop) if src reaches sink using
// only infinite edges; a correct seam graph never contains such a path.
double edmondsKarp(ResidualGraph& graph, int src, int sink);

// ---- Dinic -----------------------------------------------------------------
// O(V^2 * E). One BFS builds a level graph, then a DFS pushes flow along
// MANY shortest paths (a blocking flow) before the next BFS.

// Labels every node with its BFS distance from src (-1 = unreachable).
// Returns true if sink is reachable.
bool bfsLevel(const ResidualGraph& graph, int src, int sink,
              std::vector<int>& level);

// Pushes a blocking flow through the level graph. `next` is each node's
// "current edge" pointer, so dead edges are never re-scanned in this phase.
double blockingFlow(ResidualGraph& graph, int src, int sink,
                    const std::vector<int>& level, std::vector<int>& next);

double dinic(ResidualGraph& graph, int src, int sink);

// ---- Min cut (runs after either max-flow) ----------------------------------

// visited[v] = true if v is reachable from src in the residual graph
// (the source side of the minimum cut).
void findReachable(const ResidualGraph& graph, int src,
                   std::vector<bool>& visited);

// Sum of ORIGINAL capacities of edges going from visited to not visited.
// Equals the max flow (max-flow min-cut theorem); useful as a check.
double findMinCut(const ResidualGraph& graph, const std::vector<bool>& visited);

// Debug: prints every forward edge as "u -> v : flow / capacity".
void printGraph(const ResidualGraph& graph, const std::string& name);

}  // namespace mincut
