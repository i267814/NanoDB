#pragma once
#include <cstdio>
#include <cstring>
#include <ctime>
#include "types.h"
#include "buffer_pool.h"
#include "catalog.h"
#include "avl_tree.h"
#include "parser.h"

static const int MAX_ROWS_RESULT = 50000;

// ─── Table Engine ─────────────────────────────────────────────────────────────
class TableEngine {
public:
    TableDef*   def;
    BufferPool* pool;
    AVLTree     index;       // index on first integer column (primary key)
    int         nextPageId;

    TableEngine(TableDef* td, BufferPool* bp)
        : def(td), pool(bp), nextPageId(0) {}

    // ── INSERT ────────────────────────────────────────────────────────────────
    bool insertRow(Row* row) {
        int pageId = def->rowCount / MAX_ROWS_PAGE;
        int slotId = def->rowCount % MAX_ROWS_PAGE;

        Page* p = pool->fetchPage(pageId);

        // Serialize row into page data
        // Page layout: [rowCount:int][row0][row1]...
        int* rcPtr = reinterpret_cast<int*>(p->data);
        int  rc    = *rcPtr;

        int offset = sizeof(int) + rc * estimateRowSize();
        if (offset + estimateRowSize() > PAGE_SIZE) {
            pageId = ++nextPageId + (def->rowCount / MAX_ROWS_PAGE);
            p = pool->fetchPage(pageId);
            rcPtr = reinterpret_cast<int*>(p->data);
            rc = *rcPtr;
            offset = sizeof(int) + rc * estimateRowSize();
            slotId = rc;
        }

        row->serialize(p->data, offset);
        (*rcPtr)++;
        pool->markDirty(pageId);

        // Update index on first int column
        if (row->numFields > 0 && row->fields[0]->typeId() == 0) {
            int pk = static_cast<IntField*>(row->fields[0])->value;
            index.insert(pk, pageId, slotId);
        }

        def->rowCount++;
        return true;
    }

    // ── SEQUENTIAL SCAN with optional WHERE ───────────────────────────────────
    int seqScan(const char* whereExpr, Row** results, int maxResults) {
        clock_t t0 = clock();
        int found = 0;

        Token infix[MAX_TOKENS], postfix[MAX_TOKENS];
        int   pn = 0;
        bool  hasWhere = (whereExpr && strlen(whereExpr) > 0);

        if (hasWhere) {
            int tn = tokenize(whereExpr, infix, MAX_TOKENS);
            pn = infixToPostfix(infix, tn, postfix);
            printf("[LOG] Infix \"%s\" converted to Postfix \"", whereExpr);
            for (int i=0;i<pn;i++) printf("%s ", postfix[i].val);
            printf("\"\n");
        }

        // Build colNames array for evaluator
        const char* colNames[MAX_COLS];
        for (int i = 0; i < def->numCols; i++) colNames[i] = def->cols[i].name;

        int totalPages = (def->rowCount / MAX_ROWS_PAGE) + 1;
        int rowsSeen   = 0;

        for (int pg = 0; pg <= totalPages && found < maxResults; pg++) {
            Page* p  = pool->fetchPage(pg);
            int   rc = *reinterpret_cast<int*>(p->data);
            int   off = sizeof(int);

            for (int s = 0; s < rc && rowsSeen < def->rowCount; s++, rowsSeen++) {
                Row* r = new Row();
                r->deserialize(p->data, off);

                bool include = true;
                if (hasWhere)
                    include = evaluatePostfixStr(postfix, pn, r,
                                                  colNames, def->numCols);
                if (include) results[found++] = r;
                else         delete r;
            }
        }

        double ms = 1000.0 * (clock() - t0) / CLOCKS_PER_SEC;
        printf("[SCAN] Sequential scan on '%s': %d rows scanned, %d matched, %.3f ms\n",
               def->name, def->rowCount, found, ms);
        return found;
    }

    // ── INDEXED LOOKUP (AVL) ───────────────────────────────────────────────────
    Row* indexedLookup(int pk) {
        clock_t t0 = clock();
        int pageId, slotId;
        if (index.search(pk, pageId, slotId) < 0) {
            printf("[INDEX] Key %d not found in AVL index.\n", pk);
            return nullptr;
        }
        Page* p   = pool->fetchPage(pageId);
        int   off = sizeof(int) + slotId * estimateRowSize();
        Row*  r   = new Row();
        r->deserialize(p->data, off);

        double ms = 1000.0 * (clock() - t0) / CLOCKS_PER_SEC;
        printf("[INDEX] AVL indexed lookup key=%d: found at page=%d,slot=%d in %.4f ms\n",
               pk, pageId, slotId, ms);
        return r;
    }

    // ── FLUSH to disk ──────────────────────────────────────────────────────────
    void flush() { pool->flushAll(); }

    int estimateRowSize() const {
        int sz = sizeof(int); // numFields
        for (int i = 0; i < def->numCols; i++) {
            sz += sizeof(int); // typeId
            if      (def->cols[i].typeId == 0) sz += sizeof(int);
            else if (def->cols[i].typeId == 1) sz += sizeof(float);
            else                               sz += MAX_STR;
        }
        return sz;
    }
};
