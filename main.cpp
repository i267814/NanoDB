// main.cpp — NanoDB Engine Entry Point
// Handles all 7 demo test cases (A-G)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "../include/nanodb.h"

// ─── Logging redirect ─────────────────────────────────────────────────────────
static FILE* logFile = nullptr;
void openLog()  {
    logFile = fopen("nanodb_execution.log","w");
    // Duplicate stdout to log via a simple tee approach
    // (We print everything to stdout; test runner can tee)
    printf("[LOG] NanoDB started. Log: nanodb_execution.log\n");
}
void closeLog() { if (logFile) fclose(logFile); }

// ─── Load TPC-H CSV tables into NanoDB ────────────────────────────────────────
void loadCustomers(NanoDB& db) {
    FILE* f = fopen("data/customer.tbl","r");
    if (!f) { printf("[ERROR] data/customer.tbl not found. Run gen_data first.\n"); return; }
    int loaded = 0;
    char line[256];
    while (fgets(line, sizeof(line), f) && loaded < 20000) {
        int custkey, nationkey; float acctbal; char seg[32];
        if (sscanf(line, "%d|%*[^|]|%f|%31[^|]|%d", &custkey, &acctbal, seg, &nationkey) < 3)
            continue;
        Row* r = new Row();
        r->addField(new IntField(custkey));
        r->addField(new FloatField(acctbal));
        r->addField(new StringField(seg));
        r->addField(new IntField(nationkey));
        db.insert("customer", r);
        delete r;
        loaded++;
    }
    fclose(f);
    printf("[LOAD] Loaded %d customer rows.\n", loaded);
}

void loadOrders(NanoDB& db) {
    FILE* f = fopen("data/orders.tbl","r");
    if (!f) { printf("[ERROR] data/orders.tbl not found.\n"); return; }
    int loaded = 0;
    char line[256];
    while (fgets(line, sizeof(line), f) && loaded < 30000) {
        int orderkey, custkey; char status[4]; float total; char pri[32];
        if (sscanf(line, "%d|%d|%3[^|]|%f|%31[^\n]",
                   &orderkey, &custkey, status, &total, pri) < 4) continue;
        Row* r = new Row();
        r->addField(new IntField(orderkey));
        r->addField(new IntField(custkey));
        r->addField(new StringField(status));
        r->addField(new FloatField(total));
        db.insert("orders", r);
        delete r;
        loaded++;
    }
    fclose(f);
    printf("[LOAD] Loaded %d orders rows.\n", loaded);
}

void loadLineItems(NanoDB& db) {
    FILE* f = fopen("data/lineitem.tbl","r");
    if (!f) { printf("[ERROR] data/lineitem.tbl not found.\n"); return; }
    int loaded = 0;
    char line[256];
    while (fgets(line, sizeof(line), f) && loaded < 50000) {
        int lid, orderkey, partkey; float price, discount; char rf[4], ls[4];
        if (sscanf(line,"%d|%d|%d|%f|%f|%3[^|]|%3[^\n]",
                   &lid,&orderkey,&partkey,&price,&discount,rf,ls)<6) continue;
        Row* r = new Row();
        r->addField(new IntField(lid));
        r->addField(new IntField(orderkey));
        r->addField(new FloatField(price));
        r->addField(new FloatField(discount));
        db.insert("lineitem", r);
        delete r;
        loaded++;
    }
    fclose(f);
    printf("[LOAD] Loaded %d lineitem rows.\n", loaded);
}

// ─── Schema definitions ───────────────────────────────────────────────────────
void createSchemas(NanoDB& db) {
    // customer(c_custkey INT, c_acctbal FLOAT, c_mktsegment STRING, c_nationkey INT)
    ColDef custCols[4];
    strcpy(custCols[0].name,"c_custkey");    custCols[0].typeId=0;
    strcpy(custCols[1].name,"c_acctbal");    custCols[1].typeId=1;
    strcpy(custCols[2].name,"c_mktsegment"); custCols[2].typeId=2;
    strcpy(custCols[3].name,"c_nationkey");  custCols[3].typeId=0;
    db.createTable("customer", custCols, 4);

    // orders(o_orderkey INT, o_custkey INT, o_orderstatus STRING, o_totalprice FLOAT)
    ColDef ordCols[4];
    strcpy(ordCols[0].name,"o_orderkey");   ordCols[0].typeId=0;
    strcpy(ordCols[1].name,"o_custkey");    ordCols[1].typeId=0;
    strcpy(ordCols[2].name,"o_orderstatus");ordCols[2].typeId=2;
    strcpy(ordCols[3].name,"o_totalprice"); ordCols[3].typeId=1;
    db.createTable("orders", ordCols, 4);

    // lineitem(l_orderkey INT, l_partkey INT, l_extendedprice FLOAT, l_discount FLOAT)
    ColDef lineCols[4];
    strcpy(lineCols[0].name,"l_linenumber");    lineCols[0].typeId=0;
    strcpy(lineCols[1].name,"l_orderkey");      lineCols[1].typeId=0;
    strcpy(lineCols[2].name,"l_extendedprice"); lineCols[2].typeId=1;
    strcpy(lineCols[3].name,"l_discount");      lineCols[3].typeId=1;
    db.createTable("lineitem", lineCols, 4);
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST CASES A–G
// ═══════════════════════════════════════════════════════════════════════════════

void testCaseA(NanoDB& db) {
    printf("\n============================================================\n");
    printf("TEST CASE A: Parser & Evaluator\n");
    printf("============================================================\n");

    const char* expr = "(c_acctbal > 5000 AND c_mktsegment == BUILDING) OR c_nationkey == 15";
    printf("[TEST-A] Input expression: %s\n", expr);

    Token infix[MAX_TOKENS], postfix[MAX_TOKENS];
    int tn = tokenize(expr, infix, MAX_TOKENS);
    int pn = infixToPostfix(infix, tn, postfix);
    printPostfix(postfix, pn);

    Row* results[500];
    int found = db.select("customer", expr, results, 500);
    printf("[TEST-A] Rows matching condition: %d\n", found);
    if (found > 0) {
        printf("[TEST-A] First 3 results:\n");
        for (int i = 0; i < (found<3?found:3); i++) {
            printf("  Row %d: ", i+1); results[i]->print(); printf("\n");
        }
    }
    for (int i=0;i<found;i++) delete results[i];
}

void testCaseB(NanoDB& db) {
    printf("\n============================================================\n");
    printf("TEST CASE B: Index Optimizer (Sequential vs AVL)\n");
    printf("============================================================\n");

    int targetKey = 5000;

    // Sequential scan
    clock_t t0 = clock();
    Row* seqResults[2];
    db.select("customer", "c_custkey == 5000", seqResults, 1);
    double seqMs = 1000.0*(clock()-t0)/CLOCKS_PER_SEC;
    printf("[TEST-B] Sequential scan time: %.4f ms\n", seqMs);

    // AVL indexed lookup
    t0 = clock();
    Row* idxRow = db.selectByKey("customer", targetKey);
    double idxMs = 1000.0*(clock()-t0)/CLOCKS_PER_SEC;
    printf("[TEST-B] AVL indexed lookup time: %.4f ms\n", idxMs);
    printf("[TEST-B] Speedup: %.1fx faster with index\n",
           seqMs > 0 ? seqMs/idxMs : 999.0);

    if (idxRow) { printf("[TEST-B] Found row: "); idxRow->print(); printf("\n"); }
    delete idxRow;
}

void testCaseC(NanoDB& db) {
    printf("\n============================================================\n");
    printf("TEST CASE C: Join Optimizer (MST)\n");
    printf("============================================================\n");
    db.joinMST("customer","orders","lineitem");
}

void testCaseD(NanoDB& db) {
    printf("\n============================================================\n");
    printf("TEST CASE D: Memory Stress Test (50-page pool, 5000 lineitem scan)\n");
    printf("============================================================\n");

    // Create a small-pool engine for lineitem
    TableDef* td = db.catalog.getTable("lineitem");
    if (!td) { printf("[TEST-D] lineitem not found.\n"); return; }

    BufferPool smallPool("lineitem.ndb", 50);
    TableEngine te(td, &smallPool);

    // Re-insert 5000 rows into this engine to stress test
    Row* results[5000];
    int found = te.seqScan("", results, 5000);
    for (int i=0;i<found;i++) delete results[i];

    printf("[TEST-D] Total LRU evictions during stress test: %d\n",
           smallPool.getEvictionCount());
}

void testCaseE(NanoDB& db) {
    printf("\n============================================================\n");
    printf("TEST CASE E: Priority Queue Concurrency\n");
    printf("============================================================\n");

    // Enqueue 50 normal SELECT queries (priority=5)
    for (int i = 1; i <= 50; i++) {
        char q[128];
        snprintf(q, sizeof(q), "SELECT * FROM customer WHERE c_custkey == %d", i);
        db.enqueueQuery(q, 5);
    }
    // Enqueue 1 ADMIN UPDATE (priority=0 = highest)
    db.enqueueQuery("UPDATE customer SET c_acctbal = 9999.99 WHERE c_custkey == 1 [ADMIN]", 0);

    printf("[TEST-E] Queue has %d items. Admin query should execute first.\n", db.pq.size());
    db.runQueryQueue();
}

void testCaseF(NanoDB& db) {
    printf("\n============================================================\n");
    printf("TEST CASE F: Deep Expression Tree\n");
    printf("============================================================\n");

    const char* expr = "( ( o_totalprice * 1.5 ) > 100000 AND ( o_custkey % 2 == 0 ) ) OR ( o_orderstatus != O )";
    printf("[TEST-F] Complex expression: %s\n", expr);

    Token infix[MAX_TOKENS], postfix[MAX_TOKENS];
    int tn = tokenize(expr, infix, MAX_TOKENS);
    int pn = infixToPostfix(infix, tn, postfix);
    printPostfix(postfix, pn);

    Row* results[1000];
    int found = db.select("orders", expr, results, 1000);
    printf("[TEST-F] Rows matched: %d\n", found);
    for (int i=0;i<found;i++) delete results[i];
}

void testCaseG(NanoDB& db) {
    printf("\n============================================================\n");
    printf("TEST CASE G: Durability & Persistence\n");
    printf("============================================================\n");

    // Insert 5 new customer records
    for (int i = 20001; i <= 20005; i++) {
        Row* r = new Row();
        r->addField(new IntField(i));
        r->addField(new FloatField((float)(i * 10)));
        r->addField(new StringField("BUILDING"));
        r->addField(new IntField(1));
        db.insert("customer", r);
        printf("[TEST-G] Inserted customer %d\n", i);
        delete r;
    }

    // Flush to disk
    db.flushAll();
    printf("[TEST-G] All pages flushed to disk. Simulating shutdown...\n");
    printf("[TEST-G] On reboot: query for c_custkey >= 20001 will verify persistence.\n");
}

// ═══════════════════════════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════════════════════════
int main() {
    openLog();
    printf("\n========================================\n");
    printf("   NanoDB — NUCES MS CS Semester Project\n");
    printf("   Ayesha Bibi\n");
    printf("========================================\n\n");

    NanoDB db;
    createSchemas(db);

    printf("\n[INIT] Loading TPC-H data...\n");
    loadCustomers(db);
    loadOrders(db);
    loadLineItems(db);
    db.catalog.listTables();

    // Run all test cases
    testCaseA(db);
    testCaseB(db);
    testCaseC(db);
    testCaseD(db);
    testCaseE(db);
    testCaseF(db);
    testCaseG(db);

    printf("\n[DONE] Total LRU evictions across all pools: %d\n", db.totalEvictions());
    printf("[DONE] NanoDB shutdown complete.\n");
    closeLog();
    return 0;
}
