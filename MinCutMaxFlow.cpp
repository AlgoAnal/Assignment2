#include "MinCutMaxFlow.h"


void updateResidualGraph(std::vector<std::vector<int>>& residualGraph, const std::vector<int>& parent, int src, int sink, int pathFlow){
    for (int v = sink; v != src; v = parent[v])
    {
        int u = parent[v];

        // Update forward residual capacity
        residualGraph[u][v] -= pathFlow;

        // Update reverse residual capacity
        residualGraph[v][u] += pathFlow;
    
    }



}



// runs during Max-Flow
/*
-Searches for a path from S to T.
-Records the path using parent.
-Returns true immediately when T is discovered.
-Returns false if T cannot be reached.
*/
bool bfs(const std::vector<std::vector<int>>& residualGraph, int src, int sink, std::vector<int>& parent){
    int n = residualGraph.size();
    std::vector<bool> visited(n, false);
    std::queue<int> q; 

    q.push(src);
    visited[src] = true;

    //parent will tell us what the recorded path is

    //initialize the parent by replacing all the ints with -1. -1 means no predecessor
    parent.assign(n,-1);

    
    while(!q.empty()){
        int current = q.front();
        q.pop();

        for (int next = 0; next < n; next++){
            //try to travel to the next node
            if (!visited[next] && residualGraph[current][next] > 0)
            {
                visited[next] = true;
                parent[next] = current;

                if (next == sink) return true;

                q.push(next);
            }

        }
    }
    
    return false;

}


int findPathFlow(const std::vector<std::vector<int>>& residualGraph, const std::vector<int>& parent, int src, int sink){
    int pathFlow = std::numeric_limits<int>::max();

    for (int v = sink; v != src; v = parent[v]){
        int u = parent[v];
        pathFlow = std::min(pathFlow, residualGraph[u][v]);

    }

    return pathFlow;
}

// runs after Max-Flow has finished
/*
-Starts searching from S.
-Explores every vertex reachable through positive residual capacity.
-Doesn't need to record parents.
-Doesn't stop early at T.
-Produces the visited array used to identify the minimum cut.
*/
void findReachable(const std::vector<std::vector<int>>& residualGraph, int src, std::vector<bool>& visited){

    int n = residualGraph.size();
    visited.assign(n, false);
    std::queue<int> q{};

    q.push(src);
    visited[src] = true;

    
    while (!q.empty())
    {
        int current = q.front();
        q.pop();

        // Explore neighbours here
        for (int i = 0; i < n; i++){
            if (!visited[i] && residualGraph[current][i] > 0){
                    visited[i] = true;
                    q.push(i);
            }
        }
    }



}

int findMinCut(const std::vector<std::vector<int>>& adjacencyMatrix, const std::vector<bool>& visited){
    int minCutCapacity{};
    for (int i = 0; i < adjacencyMatrix.size(); i++){
        for(int j = 0; j < adjacencyMatrix[i].size(); j++){
            if (visited[i] && !visited[j] && adjacencyMatrix[i][j] > 0){
                minCutCapacity += adjacencyMatrix[i][j];
            }
        }   
    }
    return minCutCapacity;
}

//Debug
void printMatrix(const std::vector<std::vector<int>>& matrix,
                 const std::string& name)
{
    std::cout << "\n=== " << name << " ===\n";

    // Print column headings
    std::cout << std::setw(6) << " ";
    for (int j = 0; j < matrix.size(); j++)
    {
        std::cout << std::setw(6) << j;
    }
    std::cout << '\n';

    // Print each row
    for (int i = 0; i < matrix.size(); i++)
    {
        std::cout << std::setw(6) << i;

        for (int j = 0; j < matrix[i].size(); j++)
        {
            std::cout << std::setw(6) << matrix[i][j];
        }

        std::cout << '\n';
    }
}


