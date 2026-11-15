#pragma once
#include <utility>
#include <queue>
#include <algorithm>
#include <limits>
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>



void updateResidualGraph(std::vector<std::vector<int>>& residualGraph, const std::vector<int>& parent, int src, int sink, int pathFlow);

bool bfs(const std::vector<std::vector<int>>& residualGraph, int src, int sink, std::vector<int>& parent);

int findPathFlow(const std::vector<std::vector<int>>& residualGraph, const std::vector<int>& parent, int src, int sink);

void printMatrix(const std::vector<std::vector<int>>& matrix, const std::string& name);

void findReachable(const std::vector<std::vector<int>>& residualGraph, int src, std::vector<bool>& visited);

int findMinCut(const std::vector<std::vector<int>>& adjacencyMatrix, const std::vector<bool>& visited);