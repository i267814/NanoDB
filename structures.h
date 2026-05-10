#pragma once
#include <cstring>
#include <cstdlib>
#include <cstdio>

// ─── Generic Stack (raw array, no STL) ───────────────────────────────────────
template<typename T>
class Stack {
    T*  data;
    int cap;
    int top;
public:
    Stack(int capacity = 128) : cap(capacity), top(-1) {
        data = new T[cap];
    }
    ~Stack() { delete[] data; }

    void push(const T& val) {
        if (top + 1 >= cap) {
            cap *= 2;
            T* nd = new T[cap];
            for (int i = 0; i <= top; i++) nd[i] = data[i];
            delete[] data; data = nd;
        }
        data[++top] = val;
    }
    T pop() {
        if (top < 0) { fprintf(stderr, "[STACK] Underflow!\n"); return T{}; }
        return data[top--];
    }
    T peek() const {
        if (top < 0) return T{};
        return data[top];
    }
    bool empty() const { return top < 0; }
    int  size()  const { return top + 1; }
    void clear()       { top = -1; }
};

// ─── Generic Queue (circular array, no STL) ──────────────────────────────────
template<typename T>
class Queue {
    T*  data;
    int cap;
    int front, back, count;
public:
    Queue(int capacity = 128) : cap(capacity), front(0), back(0), count(0) {
        data = new T[cap];
    }
    ~Queue() { delete[] data; }

    void enqueue(const T& val) {
        if (count == cap) {
            int nc = cap * 2;
            T* nd = new T[nc];
            for (int i = 0; i < count; i++) nd[i] = data[(front + i) % cap];
            delete[] data; data = nd;
            front = 0; back = count; cap = nc;
        }
        data[back] = val;
        back = (back + 1) % cap;
        count++;
    }
    T dequeue() {
        if (count == 0) { fprintf(stderr, "[QUEUE] Underflow!\n"); return T{}; }
        T val = data[front];
        front = (front + 1) % cap;
        count--;
        return val;
    }
    T peekFront() const { return data[front]; }
    bool empty() const  { return count == 0; }
    int  size()  const  { return count; }
};

// ─── Priority Queue (min-heap, no STL) ───────────────────────────────────────
struct PQItem {
    int         priority;  // lower = higher priority (0 = admin)
    char        query[256];
    bool operator<(const PQItem& o) const { return priority < o.priority; }
};

class PriorityQueue {
    PQItem* heap;
    int     cap;
    int     sz;

    void swap(PQItem& a, PQItem& b) { PQItem t = a; a = b; b = t; }
    void bubbleUp(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;
            if (heap[p].priority > heap[i].priority) { swap(heap[p], heap[i]); i = p; }
            else break;
        }
    }
    void bubbleDown(int i) {
        while (true) {
            int l = 2*i+1, r = 2*i+2, mn = i;
            if (l < sz && heap[l].priority < heap[mn].priority) mn = l;
            if (r < sz && heap[r].priority < heap[mn].priority) mn = r;
            if (mn == i) break;
            swap(heap[i], heap[mn]); i = mn;
        }
    }
public:
    PriorityQueue(int cap = 256) : cap(cap), sz(0) { heap = new PQItem[cap]; }
    ~PriorityQueue() { delete[] heap; }

    void push(int priority, const char* query) {
        if (sz == cap) { fprintf(stderr,"[PQ] Full!\n"); return; }
        heap[sz].priority = priority;
        strncpy(heap[sz].query, query, 255);
        bubbleUp(sz++);
        printf("[PQ] Enqueued (priority=%d): %.60s\n", priority, query);
    }
    PQItem pop() {
        PQItem top = heap[0];
        heap[0] = heap[--sz];
        bubbleDown(0);
        return top;
    }
    PQItem peek() const { return heap[0]; }
    bool   empty() const { return sz == 0; }
    int    size()  const { return sz; }
};
