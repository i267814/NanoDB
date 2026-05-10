#pragma once
#include <cstdio>
#include <cstring>
#include <climits>

static const int MAX_GRAPH_NODES = 16;

// ─── Graph for join cost modeling ────────────────────────────────────────────
struct Graph {
    int  numNodes;
    char nodeNames[MAX_GRAPH_NODES][64];
    int  rowCounts[MAX_GRAPH_NODES];
    int  adjMatrix[MAX_GRAPH_NODES][MAX_GRAPH_NODES];  // join cost (edge weight)

    Graph() : numNodes(0) {
        memset(adjMatrix, 0, sizeof(adjMatrix));
        memset(rowCounts, 0, sizeof(rowCounts));
    }

    int addNode(const char* name, int rows) {
        int idx = numNodes++;
        strncpy(nodeNames[idx], name, 63);
        rowCounts[idx] = rows;
        return idx;
    }

    void addEdge(int u, int v, int cost) {
        adjMatrix[u][v] = cost;
        adjMatrix[v][u] = cost;
    }

    // Prim's MST — returns order of join
    // mstOrder[] filled with node indices in MST traversal order
    int primMST(int mstOrder[], int mstCosts[]) {
        bool  inMST[MAX_GRAPH_NODES] = {};
        int   key[MAX_GRAPH_NODES];
        int   parent[MAX_GRAPH_NODES];
        for (int i = 0; i < numNodes; i++) { key[i] = INT_MAX; parent[i] = -1; }
        key[0] = 0;
        int orderIdx = 0;

        for (int count = 0; count < numNodes; count++) {
            // Pick min key vertex not yet in MST
            int u = -1;
            for (int v = 0; v < numNodes; v++)
                if (!inMST[v] && (u == -1 || key[v] < key[u])) u = v;

            inMST[u] = true;
            mstOrder[orderIdx]  = u;
            mstCosts[orderIdx]  = key[u];
            orderIdx++;

            for (int v = 0; v < numNodes; v++) {
                if (!inMST[v] && adjMatrix[u][v] && adjMatrix[u][v] < key[v]) {
                    key[v] = adjMatrix[u][v];
                    parent[v] = u;
                }
            }
        }

        printf("[LOG] Multi-table join routed via MST: ");
        for (int i = 0; i < orderIdx; i++) {
            printf("%s", nodeNames[mstOrder[i]]);
            if (i < orderIdx-1) printf(" -> ");
        }
        printf("\n");
        printf("[LOG] MST edge costs: ");
        for (int i = 1; i < orderIdx; i++) printf("%d ", mstCosts[i]);
        printf("\n");

        return orderIdx;
    }

    void printGraph() const {
        printf("[GRAPH] Join cost matrix:\n");
        printf("%-12s", "");
        for (int i = 0; i < numNodes; i++) printf("%-12s", nodeNames[i]);
        printf("\n");
        for (int i = 0; i < numNodes; i++) {
            printf("%-12s", nodeNames[i]);
            for (int j = 0; j < numNodes; j++) printf("%-12d", adjMatrix[i][j]);
            printf("\n");
        }
    }
};

// ─── Build join graph from table sizes (cost = product of row counts) ─────────
inline Graph buildJoinGraph(const char** tables, int* rowCounts, int n) {
    Graph g;
    for (int i = 0; i < n; i++) g.addNode(tables[i], rowCounts[i]);
    for (int i = 0; i < n; i++)
        for (int j = i+1; j < n; j++)
            g.addEdge(i, j, rowCounts[i] * rowCounts[j] / 1000 + 1);
    return g;
}
