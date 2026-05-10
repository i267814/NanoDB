#pragma once
#include "catalog.h"
#include "buffer_pool.h"
#include "table_engine.h"
#include "optimizer.h"
#include "structures.h"
#include "parser.h"
#include <cstdio>
#include <cstring>

static const int MAX_ENGINES = 16;

class NanoDB {
public:
    SystemCatalog  catalog;
    TableEngine*   engines[MAX_ENGINES];
    int            engineCount;
    BufferPool*    pools[MAX_ENGINES];
    PriorityQueue  pq;

    NanoDB() : engineCount(0) {
        memset(engines, 0, sizeof(engines));
        memset(pools,   0, sizeof(pools));
    }

    ~NanoDB() {
        for (int i = 0; i < engineCount; i++) {
            engines[i]->flush();
            delete engines[i];
            delete pools[i];
        }
    }

    // ── Create table ───────────────────────────────────────────────────────────
    bool createTable(const char* name, ColDef* cols, int ncols) {
        char path[128];
        snprintf(path, sizeof(path), "%s.ndb", name);
        if (!catalog.createTable(name, cols, ncols, path)) return false;
        TableDef* td  = catalog.getTable(name);
        BufferPool* bp = new BufferPool(path, DEFAULT_POOL);
        TableEngine* te = new TableEngine(td, bp);
        pools[engineCount]   = bp;
        engines[engineCount] = te;
        engineCount++;
        return true;
    }

    // ── Get engine by table name ───────────────────────────────────────────────
    TableEngine* getEngine(const char* name) {
        for (int i = 0; i < engineCount; i++)
            if (strncmp(engines[i]->def->name, name, NAME_LEN) == 0)
                return engines[i];
        return nullptr;
    }

    // ── INSERT ─────────────────────────────────────────────────────────────────
    bool insert(const char* table, Row* row) {
        TableEngine* te = getEngine(table);
        if (!te) { printf("[ERROR] Table '%s' not found.\n", table); return false; }
        return te->insertRow(row);
    }

    // ── SELECT with WHERE ──────────────────────────────────────────────────────
    int select(const char* table, const char* whereExpr,
               Row** results, int maxRes = MAX_ROWS_RESULT) {
        TableEngine* te = getEngine(table);
        if (!te) { printf("[ERROR] Table '%s' not found.\n", table); return 0; }
        return te->seqScan(whereExpr, results, maxRes);
    }

    // ── INDEXED SELECT ─────────────────────────────────────────────────────────
    Row* selectByKey(const char* table, int pk) {
        TableEngine* te = getEngine(table);
        if (!te) return nullptr;
        return te->indexedLookup(pk);
    }

    // ── 3-TABLE JOIN with MST optimizer ───────────────────────────────────────
    void joinMST(const char* t1, const char* t2, const char* t3) {
        TableDef* d1 = catalog.getTable(t1);
        TableDef* d2 = catalog.getTable(t2);
        TableDef* d3 = catalog.getTable(t3);
        if (!d1||!d2||!d3) { printf("[ERROR] One or more tables not found.\n"); return; }

        const char* tables[3] = {t1, t2, t3};
        int counts[3] = {d1->rowCount, d2->rowCount, d3->rowCount};

        Graph g = buildJoinGraph(tables, counts, 3);
        g.printGraph();

        int order[3], costs[3];
        g.primMST(order, costs);
        printf("[JOIN] Executing join in MST order (lowest cost path selected).\n");
    }

    // ── PRIORITY QUEUE execution ───────────────────────────────────────────────
    void enqueueQuery(const char* query, int priority) {
        pq.push(priority, query);
    }

    void runQueryQueue() {
        printf("\n[PQ] Processing query queue (%d items)...\n", pq.size());
        while (!pq.empty()) {
            PQItem item = pq.pop();
            printf("[PQ] Executing (priority=%d): %s\n", item.priority, item.query);
            // In a full system, dispatch to parser here
        }
    }

    // ── FLUSH all ──────────────────────────────────────────────────────────────
    void flushAll() {
        for (int i = 0; i < engineCount; i++) engines[i]->flush();
        printf("[LOG] All tables flushed to disk.\n");
    }

    int totalEvictions() const {
        int total = 0;
        for (int i = 0; i < engineCount; i++)
            total += engines[i]->pool->getEvictionCount();
        return total;
    }
};
