#pragma once
#include <cstring>
#include <cstdio>

static const int MAX_COLS    = 16;
static const int MAX_TABLES  = 32;
static const int NAME_LEN    = 64;

// ─── Column Descriptor ───────────────────────────────────────────────────────
struct ColDef {
    char name[NAME_LEN];
    int  typeId;   // 0=int, 1=float, 2=string
};

// ─── Table Descriptor ────────────────────────────────────────────────────────
struct TableDef {
    char   name[NAME_LEN];
    char   filePath[128];
    ColDef cols[MAX_COLS];
    int    numCols;
    int    rowCount;
    bool   valid;

    TableDef() : numCols(0), rowCount(0), valid(false) {
        name[0] = '\0'; filePath[0] = '\0';
    }
};

// ─── Catalog Hash Map  (chaining, O(1) average) ──────────────────────────────
static const int CAT_BUCKETS = 64;

struct CatEntry {
    char       key[NAME_LEN];
    TableDef*  table;
    CatEntry*  next;
};

class SystemCatalog {
public:
    CatEntry* buckets[CAT_BUCKETS];

    SystemCatalog() { memset(buckets, 0, sizeof(buckets)); }
    ~SystemCatalog() {
        for (int i = 0; i < CAT_BUCKETS; i++) {
            CatEntry* e = buckets[i];
            while (e) {
                CatEntry* nx = e->next;
                delete e->table;
                delete e;
                e = nx;
            }
        }
    }

    unsigned int hash(const char* s) const {
        unsigned int h = 5381;
        while (*s) h = ((h << 5) + h) ^ (unsigned char)(*s++);
        return h % CAT_BUCKETS;
    }

    // Register a new table
    bool createTable(const char* tname, ColDef* cols, int ncols, const char* path) {
        if (getTable(tname)) { printf("[CATALOG] Table '%s' already exists.\n", tname); return false; }
        TableDef* td = new TableDef();
        strncpy(td->name,     tname, NAME_LEN-1);
        strncpy(td->filePath, path,  127);
        td->numCols = ncols;
        td->valid   = true;
        for (int i = 0; i < ncols; i++) td->cols[i] = cols[i];

        unsigned int h = hash(tname);
        CatEntry* e = new CatEntry();
        strncpy(e->key, tname, NAME_LEN-1);
        e->table = td;
        e->next  = buckets[h];
        buckets[h] = e;
        printf("[CATALOG] Table '%s' registered (path=%s, cols=%d)\n", tname, path, ncols);
        return true;
    }

    // O(1) average lookup
    TableDef* getTable(const char* tname) const {
        CatEntry* e = buckets[hash(tname)];
        while (e) {
            if (strncmp(e->key, tname, NAME_LEN) == 0) return e->table;
            e = e->next;
        }
        return nullptr;
    }

    int colIndex(const char* tname, const char* cname) const {
        TableDef* td = getTable(tname);
        if (!td) return -1;
        for (int i = 0; i < td->numCols; i++)
            if (strncmp(td->cols[i].name, cname, NAME_LEN) == 0) return i;
        return -1;
    }

    void listTables() const {
        printf("[CATALOG] Registered tables:\n");
        for (int i = 0; i < CAT_BUCKETS; i++) {
            CatEntry* e = buckets[i];
            while (e) {
                printf("  - %s (%d cols, %d rows)\n",
                       e->table->name, e->table->numCols, e->table->rowCount);
                e = e->next;
            }
        }
    }
};
