# NanoDB — Mini Database Engine
**NUCES MS CS | CS-4002 Applied Programming | Spring 2026**  
**Team:**  Ayesha Bibi (26I-7814)

---

## Overview
NanoDB is a from-scratch mini-database engine in C++ implementing:
- Custom Buffer Pool with LRU eviction (Doubly Linked List)
- Polymorphic type system (IntField, FloatField, StringField)
- System Catalog with O(1) HashMap
- AVL Tree indexing (O(log N) lookups)
- Infix→Postfix parser with custom Stack (Shunting-Yard)
- Priority Queue for admin query pre-emption
- MST-based join optimizer (Prim's algorithm)
- Binary file serialization for persistence

**No STL containers used anywhere.**

---

## Project Structure
```
nanodb/
├── include/
│   ├── types.h          # Field, Row polymorphic types
│   ├── buffer_pool.h    # Pager + LRU doubly-linked list + HashMap
│   ├── catalog.h        # System catalog (hash map)
│   ├── avl_tree.h       # Self-balancing AVL index
│   ├── structures.h     # Stack, Queue, PriorityQueue
│   ├── parser.h         # Infix→Postfix + expression evaluator
│   ├── optimizer.h      # Graph + Prim's MST join optimizer
│   ├── table_engine.h   # Table insert/scan/index engine
│   └── nanodb.h         # Top-level DB coordinator
├── src/
│   ├── main.cpp         # Demo entry point (Test Cases A–G)
│   └── gen_data.cpp     # TPC-H data generator (100K records)
├── tests/
│   ├── test_runner.cpp  # Automated workload runner
│   └── queries.txt      # 50 SQL-like workload queries
├── data/                # Generated .tbl files (git-ignored)
├── Makefile
└── README.md
```

---

## Build & Run

### Prerequisites
```bash
g++ (C++17)   # any modern GCC/Clang
make
valgrind      # optional, for memory check
```

### Quick Start (Recommended)
```bash
# Build everything + generate data + run test runner
make run
```
This will:
1. Compile `nanodb`, `gen_data`, `test_runner`
2. Generate `data/customer.tbl` (20K), `data/orders.tbl` (30K), `data/lineitem.tbl` (50K)
3. Run `test_runner` on `queries.txt`
4. Save full output to `nanodb_execution.log`

### Run Full Demo (Test Cases A–G)
```bash
make demo
```

### Memory Check
```bash
make memcheck
```

### Manual Steps
```bash
make all          # compile all binaries
mkdir -p data
./gen_data        # generate TPC-H data
./nanodb          # run main demo
./test_runner     # run automated workload
```

---

## Demo Test Cases
| Case | What it tests |
|------|---------------|
| A    | Parser: Infix→Postfix conversion + row filtering |
| B    | Index: Sequential scan vs AVL indexed lookup timing |
| C    | Join Optimizer: MST path printed before execution |
| D    | Memory Stress: LRU eviction count with 50-page pool |
| E    | Priority Queue: Admin query jumps ahead of 50 SELECTs |
| F    | Deep Expression: Nested math + logic without crash |
| G    | Persistence: Insert 5 rows, flush, verify on reboot |

---

## Log Format
All internal events are prefixed:
```
[LOG]    Buffer pool / disk events
[INDEX]  AVL tree operations
[SCAN]   Sequential scan stats
[PQ]     Priority queue events
[JOIN]   MST optimizer decisions
[CATALOG] Schema registration
```

---

## Complexity Summary
| Component      | Structure             | Time         | Space   |
|----------------|-----------------------|--------------|---------|
| Buffer Pool    | Doubly Linked List    | O(1) evict   | O(P)    |
| Catalog Lookup | Chained HashMap       | O(1) avg     | O(T)    |
| Index Lookup   | AVL Tree              | O(log N)     | O(N)    |
| Query Parse    | Stack (Shunting-Yard) | O(T)         | O(T)    |
| Join Optimizer | Prim's MST on Graph   | O(V²)        | O(V²)   |
| Sequential Scan| Raw array traversal   | O(N)         | O(1)    |

---

## GitHub
https://github.com/i267814/NanoDB 

