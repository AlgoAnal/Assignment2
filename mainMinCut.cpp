#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include "MinCutMaxFlow.h"
#include <utility>
#include <queue>
#include <algorithm>
#include <limits>

//this will represent the Adjacency Matrix in 4x4 
std::vector<std::vector<int>> adjacencyMatrix(4, std::vector<int>(4,0));
std::vector<std::vector<int>> residualGraph(4, std::vector<int>(4,0));


int main(){

    adjacencyMatrix[0][1] = 10;
    adjacencyMatrix[0][2] = 5;
    adjacencyMatrix[1][3] = 4;
    adjacencyMatrix[2][3] = 8;
       
    residualGraph = adjacencyMatrix;

    int src = 0;
    int sink = 3;
    std::vector<int> parent(4, -1);

    printMatrix(residualGraph, "Residual Graph Before Update");

    bool pathFound{};

    int maxFlow = 0;


    while (pathFound = bfs(residualGraph, src, sink, parent)) {

    //from here i want to trace back to the source from the sink
    int pathFlow = findPathFlow(residualGraph, parent, src, sink);

    std::cout << "Path Flow: " << pathFlow << "\n";
    
    updateResidualGraph(residualGraph, parent, src, sink, pathFlow);

    maxFlow += pathFlow;
    
    }


    
    printMatrix(residualGraph, "Residual Graph After Update");
    std::cout << "Maximum Flow: " << maxFlow << '\n';


    std::vector<bool> visited;

    findReachable(residualGraph, src, visited);

    int minCut = findMinCut(adjacencyMatrix, visited);

    std::cout << "Maximum Flow: " << maxFlow << '\n';
    std::cout << "Minimum Cut: " << minCut << '\n';



}



