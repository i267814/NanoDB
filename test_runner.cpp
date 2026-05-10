// test_runner.cpp — Automated Test Runner
// Reads queries.txt and executes each command through NanoDB
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "../include/nanodb.h"

static NanoDB* gDB = nullptr;

// ─── Simple command dispatcher ────────────────────────────────────────────────
void dispatchQuery(const char* line) {
    if (!line || line[0]=='#' || line[0]=='\n' || line[0]=='\0') return;

    printf("\n[RUNNER] Executing: %s\n", line);

    // INSERT customer custkey acctbal segment nationkey
    if (strncmp(line,"INSERT customer",15)==0) {
        int ck,nk; float ab; char seg[32];
        if (sscanf(line,"INSERT customer %d %f %31s %d",&ck,&ab,seg,&nk)==4) {
            Row* r = new Row();
            r->addField(new IntField(ck));
            r->addField(new FloatField(ab));
            r->addField(new StringField(seg));
            r->addField(new IntField(nk));
            gDB->insert("customer", r);
            delete r;
        }
    }
    // INSERT orders orderkey custkey status totalprice
    else if (strncmp(line,"INSERT orders",13)==0) {
        int ok,ck; char st[4]; float tp;
        if (sscanf(line,"INSERT orders %d %d %3s %f",&ok,&ck,st,&tp)==4) {
            Row* r = new Row();
            r->addField(new IntField(ok));
            r->addField(new IntField(ck));
            r->addField(new StringField(st));
            r->addField(new FloatField(tp));
            gDB->insert("orders", r);
            delete r;
        }
    }
    // SELECT customer WHERE <expr>
    else if (strncmp(line,"SELECT customer WHERE",21)==0) {
        const char* expr = line + 22;
        Row* results[1000];
        int  found = gDB->select("customer", expr, results, 1000);
        printf("[RUNNER] %d rows returned.\n", found);
        for (int i=0;i<(found<5?found:5);i++) {
            printf("  "); results[i]->print(); printf("\n");
        }
        for (int i=0;i<found;i++) delete results[i];
    }
    // SELECT orders WHERE <expr>
    else if (strncmp(line,"SELECT orders WHERE",19)==0) {
        const char* expr = line + 20;
        Row* results[1000];
        int  found = gDB->select("orders", expr, results, 1000);
        printf("[RUNNER] %d rows returned.\n", found);
        for (int i=0;i<(found<5?found:5);i++) {
            printf("  "); results[i]->print(); printf("\n");
        }
        for (int i=0;i<found;i++) delete results[i];
    }
    // SELECT_KEY customer <key>
    else if (strncmp(line,"SELECT_KEY customer",19)==0) {
        int key; sscanf(line,"SELECT_KEY customer %d",&key);
        Row* r = gDB->selectByKey("customer", key);
        if (r) { printf("[RUNNER] Found: "); r->print(); printf("\n"); delete r; }
    }
    // JOIN customer orders lineitem
    else if (strncmp(line,"JOIN",4)==0) {
        gDB->joinMST("customer","orders","lineitem");
    }
    // ADMIN <query>
    else if (strncmp(line,"ADMIN",5)==0) {
        gDB->enqueueQuery(line+6, 0);
        gDB->runQueryQueue();
    }
    // FLUSH
    else if (strncmp(line,"FLUSH",5)==0) {
        gDB->flushAll();
    }
    // STATS
    else if (strncmp(line,"STATS",5)==0) {
        gDB->catalog.listTables();
        printf("[STATS] Total evictions: %d\n", gDB->totalEvictions());
    }
    else {
        printf("[RUNNER] Unknown command: %s\n", line);
    }
}

int main() {
    printf("========================================\n");
    printf("  NanoDB Automated Test Runner\n");
    printf("========================================\n");

    // Initialize DB and schemas
    NanoDB db;
    gDB = &db;

    ColDef custCols[4];
    strcpy(custCols[0].name,"c_custkey");    custCols[0].typeId=0;
    strcpy(custCols[1].name,"c_acctbal");    custCols[1].typeId=1;
    strcpy(custCols[2].name,"c_mktsegment"); custCols[2].typeId=2;
    strcpy(custCols[3].name,"c_nationkey");  custCols[3].typeId=0;
    db.createTable("customer", custCols, 4);

    ColDef ordCols[4];
    strcpy(ordCols[0].name,"o_orderkey");    ordCols[0].typeId=0;
    strcpy(ordCols[1].name,"o_custkey");     ordCols[1].typeId=0;
    strcpy(ordCols[2].name,"o_orderstatus"); ordCols[2].typeId=2;
    strcpy(ordCols[3].name,"o_totalprice");  ordCols[3].typeId=1;
    db.createTable("orders", ordCols, 4);

    ColDef lineCols[4];
    strcpy(lineCols[0].name,"l_linenumber");    lineCols[0].typeId=0;
    strcpy(lineCols[1].name,"l_orderkey");      lineCols[1].typeId=0;
    strcpy(lineCols[2].name,"l_extendedprice"); lineCols[2].typeId=1;
    strcpy(lineCols[3].name,"l_discount");      lineCols[3].typeId=1;
    db.createTable("lineitem", lineCols, 4);

    // Open and execute queries.txt
    FILE* f = fopen("queries.txt","r");
    if (!f) { printf("[ERROR] queries.txt not found!\n"); return 1; }

    char line[512];
    int  qnum = 0;
    while (fgets(line, sizeof(line), f)) {
        // Strip newline
        int len = strlen(line);
        if (len > 0 && line[len-1]=='\n') line[len-1]='\0';
        qnum++;
        dispatchQuery(line);
    }
    fclose(f);

    printf("\n[RUNNER] Processed %d queries from queries.txt\n", qnum);
    printf("[RUNNER] Final eviction count: %d\n", db.totalEvictions());
    db.flushAll();
    return 0;
}
