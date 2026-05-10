#pragma once
#include <cstdio>
#include <cstdlib>
#include "types.h"

// ─── AVL Node ────────────────────────────────────────────────────────────────
struct AVLNode {
    int      key;       // row id / primary key (int)
    int      pageId;    // which page holds the row
    int      slotId;    // slot within page
    int      height;
    AVLNode* left;
    AVLNode* right;

    AVLNode(int k, int pg, int sl)
        : key(k), pageId(pg), slotId(sl), height(1), left(nullptr), right(nullptr) {}
};

// ─── AVL Tree ────────────────────────────────────────────────────────────────
class AVLTree {
public:
    AVLNode* root;
    int      nodeCount;

    AVLTree() : root(nullptr), nodeCount(0) {}
    ~AVLTree() { destroyTree(root); }

    void insert(int key, int pageId, int slotId) {
        root = insertNode(root, key, pageId, slotId);
        nodeCount++;
        printf("[INDEX] AVL insert key=%d (page=%d,slot=%d) height=%d\n",
               key, pageId, slotId, getHeight(root));
    }

    // Returns pageId,-1 if not found
    int search(int key, int& pageId, int& slotId) const {
        AVLNode* n = searchNode(root, key);
        if (!n) return -1;
        pageId = n->pageId;
        slotId = n->slotId;
        return 0;
    }

    void remove(int key) {
        root = deleteNode(root, key);
        nodeCount--;
    }

    int height() const { return getHeight(root); }

private:
    int getHeight(AVLNode* n) const { return n ? n->height : 0; }
    int getBalance(AVLNode* n) const {
        return n ? getHeight(n->left) - getHeight(n->right) : 0;
    }
    void updateHeight(AVLNode* n) {
        if (!n) return;
        int lh = getHeight(n->left), rh = getHeight(n->right);
        n->height = 1 + (lh > rh ? lh : rh);
    }

    AVLNode* rotateRight(AVLNode* y) {
        AVLNode* x  = y->left;
        AVLNode* T2 = x->right;
        x->right = y; y->left = T2;
        updateHeight(y); updateHeight(x);
        return x;
    }
    AVLNode* rotateLeft(AVLNode* x) {
        AVLNode* y  = x->right;
        AVLNode* T2 = y->left;
        y->left = x; x->right = T2;
        updateHeight(x); updateHeight(y);
        return y;
    }

    AVLNode* balance(AVLNode* n) {
        updateHeight(n);
        int b = getBalance(n);
        if (b > 1) {
            if (getBalance(n->left) < 0) n->left = rotateLeft(n->left);
            return rotateRight(n);
        }
        if (b < -1) {
            if (getBalance(n->right) > 0) n->right = rotateRight(n->right);
            return rotateLeft(n);
        }
        return n;
    }

    AVLNode* insertNode(AVLNode* n, int key, int pg, int sl) {
        if (!n) return new AVLNode(key, pg, sl);
        if (key < n->key)      n->left  = insertNode(n->left,  key, pg, sl);
        else if (key > n->key) n->right = insertNode(n->right, key, pg, sl);
        else { n->pageId = pg; n->slotId = sl; return n; } // update
        return balance(n);
    }

    AVLNode* minNode(AVLNode* n) {
        while (n->left) n = n->left;
        return n;
    }

    AVLNode* deleteNode(AVLNode* n, int key) {
        if (!n) return nullptr;
        if      (key < n->key) n->left  = deleteNode(n->left,  key);
        else if (key > n->key) n->right = deleteNode(n->right, key);
        else {
            if (!n->left || !n->right) {
                AVLNode* tmp = n->left ? n->left : n->right;
                delete n;
                return tmp;
            }
            AVLNode* succ = minNode(n->right);
            n->key = succ->key; n->pageId = succ->pageId; n->slotId = succ->slotId;
            n->right = deleteNode(n->right, succ->key);
        }
        return balance(n);
    }

    AVLNode* searchNode(AVLNode* n, int key) const {
        if (!n) return nullptr;
        if (key == n->key) return n;
        if (key <  n->key) return searchNode(n->left,  key);
        return searchNode(n->right, key);
    }

    void destroyTree(AVLNode* n) {
        if (!n) return;
        destroyTree(n->left);
        destroyTree(n->right);
        delete n;
    }
};
