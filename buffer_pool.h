#pragma once
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "types.h"

static const int PAGE_SIZE      = 4096;   // bytes per page
static const int MAX_ROWS_PAGE  = 20;     // rows per page
static const int DEFAULT_POOL   = 50;     // default pool size

// ─── Page ────────────────────────────────────────────────────────────────────
struct Page {
    int  pageId;
    bool dirty;
    int  rowCount;
    char data[PAGE_SIZE];

    Page() : pageId(-1), dirty(false), rowCount(0) { memset(data, 0, PAGE_SIZE); }

    void serialize()   { /* data already in buffer */ }
    void deserialize() { /* data already loaded    */ }
};

// ─── DLL Node for LRU ────────────────────────────────────────────────────────
struct LRUNode {
    int      pageId;
    Page*    page;
    LRUNode* prev;
    LRUNode* next;
    LRUNode(int id, Page* p) : pageId(id), page(p), prev(nullptr), next(nullptr) {}
};

// ─── Custom Doubly Linked List ────────────────────────────────────────────────
struct DLL {
    LRUNode* head;   // MRU end
    LRUNode* tail;   // LRU end
    int      size;

    DLL() : head(nullptr), tail(nullptr), size(0) {}
    ~DLL() {
        LRUNode* cur = head;
        while (cur) { LRUNode* nx = cur->next; delete cur; cur = nx; }
    }

    LRUNode* pushFront(int pageId, Page* p) {
        LRUNode* n = new LRUNode(pageId, p);
        n->next = head;
        if (head) head->prev = n;
        head = n;
        if (!tail) tail = n;
        size++;
        return n;
    }

    void moveToFront(LRUNode* n) {
        if (n == head) return;
        // detach
        if (n->prev) n->prev->next = n->next;
        if (n->next) n->next->prev = n->prev;
        if (n == tail) tail = n->prev;
        // attach at front
        n->prev = nullptr;
        n->next = head;
        if (head) head->prev = n;
        head = n;
    }

    LRUNode* removeTail() {
        if (!tail) return nullptr;
        LRUNode* t = tail;
        if (tail->prev) { tail->prev->next = nullptr; tail = tail->prev; }
        else            { head = tail = nullptr; }
        t->prev = t->next = nullptr;
        size--;
        return t;
    }

    void remove(LRUNode* n) {
        if (n->prev) n->prev->next = n->next; else head = n->next;
        if (n->next) n->next->prev = n->prev; else tail = n->prev;
        n->prev = n->next = nullptr;
        size--;
    }
};

// ─── Simple Hash Map for pageId → LRUNode* ────────────────────────────────────
static const int HM_SIZE = 1024;
struct HashEntry { int key; LRUNode* val; HashEntry* next; };

struct HashMap {
    HashEntry* buckets[HM_SIZE];
    HashMap() { memset(buckets, 0, sizeof(buckets)); }
    ~HashMap() {
        for (int i = 0; i < HM_SIZE; i++) {
            HashEntry* e = buckets[i];
            while (e) { HashEntry* nx = e->next; delete e; e = nx; }
        }
    }
    int hash(int k) const { return (k * 2654435761u) % HM_SIZE; }
    void put(int k, LRUNode* v) {
        int h = hash(k);
        HashEntry* e = buckets[h];
        while (e) { if (e->key == k) { e->val = v; return; } e = e->next; }
        HashEntry* ne = new HashEntry{k, v, buckets[h]};
        buckets[h] = ne;
    }
    LRUNode* get(int k) const {
        HashEntry* e = buckets[hash(k)];
        while (e) { if (e->key == k) return e->val; e = e->next; }
        return nullptr;
    }
    void remove(int k) {
        int h = hash(k);
        HashEntry** pp = &buckets[h];
        while (*pp) {
            if ((*pp)->key == k) {
                HashEntry* dead = *pp; *pp = dead->next; delete dead; return;
            }
            pp = &(*pp)->next;
        }
    }
};

// ─── Buffer Pool Manager ──────────────────────────────────────────────────────
class BufferPool {
public:
    int      poolSize;
    int      evictionCount;
    DLL      lruList;
    HashMap  pageMap;
    Page**   pool;        // raw array of page pointers
    char     dbFile[128];

    BufferPool(const char* filename, int size = DEFAULT_POOL)
        : poolSize(size), evictionCount(0) {
        strncpy(dbFile, filename, 127);
        pool = new Page*[poolSize];
        for (int i = 0; i < poolSize; i++) pool[i] = nullptr;
    }

    ~BufferPool() {
        flushAll();
        for (int i = 0; i < poolSize; i++) delete pool[i];
        delete[] pool;
    }

    Page* fetchPage(int pageId) {
        LRUNode* node = pageMap.get(pageId);
        if (node) {
            lruList.moveToFront(node);
            return node->page;
        }
        // Not in cache — load from disk
        Page* p = new Page();
        p->pageId = pageId;
        loadFromDisk(p);

        if (lruList.size >= poolSize) evict();

        LRUNode* n = lruList.pushFront(pageId, p);
        pageMap.put(pageId, n);
        return p;
    }

    void markDirty(int pageId) {
        LRUNode* n = pageMap.get(pageId);
        if (n) n->page->dirty = true;
    }

    void flushAll() {
        LRUNode* cur = lruList.head;
        while (cur) {
            if (cur->page->dirty) writeToDisk(cur->page);
            cur = cur->next;
        }
    }

    int getEvictionCount() const { return evictionCount; }

private:
    void evict() {
        LRUNode* victim = lruList.removeTail();
        if (!victim) return;
        if (victim->page->dirty) {
            writeToDisk(victim->page);
            printf("[LOG] Page %d evicted via LRU, written to disk\n", victim->pageId);
        } else {
            printf("[LOG] Page %d evicted via LRU (clean)\n", victim->pageId);
        }
        pageMap.remove(victim->pageId);
        delete victim->page;
        delete victim;
        evictionCount++;
    }

    void writeToDisk(Page* p) {
        FILE* f = fopen(dbFile, "r+b");
        if (!f) f = fopen(dbFile, "w+b");
        if (!f) return;
        fseek(f, (long)p->pageId * PAGE_SIZE, SEEK_SET);
        fwrite(p->data, 1, PAGE_SIZE, f);
        fclose(f);
        p->dirty = false;
    }

    void loadFromDisk(Page* p) {
        FILE* f = fopen(dbFile, "rb");
        if (!f) { memset(p->data, 0, PAGE_SIZE); return; }
        fseek(f, (long)p->pageId * PAGE_SIZE, SEEK_SET);
        fread(p->data, 1, PAGE_SIZE, f);
        fclose(f);
    }
};
