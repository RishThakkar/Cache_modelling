# Cache Performance Modeling Library (C++)

This project is a **configurable cache performance modeling framework** written in C++.  
It is intended for **computer architecture learning, design-space exploration, and performance modeling**, rather than RTL-accurate simulation.

The goal is to allow users to:
- Configure cache parameters (size, line size, associativity, latencies)
- Drive the cache with arbitrary memory-access traces
- Collect metrics such as miss rate and AMAT
- Export results for plotting and architectural analysis

This mirrors how cache behavior is studied in architecture textbooks (e.g., Hennessy & Patterson) and in early-stage industry performance modeling.

---

## Key Concepts Modeled

This library models the **logical behavior** of a cache, including:

- Cache lines (blocks)
- Sets and associativity
- Tag comparison
- Valid bits
- LRU replacement policy
- Hit and miss latency
- Cold-start behavior

It does **not** model:
- Actual data values
- RTL signals
- Cycle-accurate pipelines
- Bandwidth or coherence effects (yet)

---

## Metrics Collected

The cache tracks:

- Total accesses
- Hits
- Misses
- Miss rate
- Average Memory Access Time (AMAT)

---

## Using the Library

Typical workflow:
- Instantiate a cache with desired parameters
- Drive it using any memory-access pattern
- Collect statistics
- Export results (e.g., CSV)
- Plot or analyze results externally (Python, Excel, etc.)

The library is intentionally flexible so users can:
- Swap in different trace generators
- Load traces from files
- Model different workloads (sequential, strided, random, reuse-heavy, etc.)

---

## Example Use Cases

This framework can be used to study:
- Impact of cache size on miss rate
- Impact of associativity on conflict misses
- Effects of cache line size on spatial locality
- Capacity vs compulsory misses
- AMAT sensitivity to architectural parameters

It is suitable for:
- Architecture coursework
- Self-study and experimentation
- Early-stage performance modeling
- Design intuition building before RTL

---

## Design Philosophy

This project prioritizes:
- Clarity over micro-optimizations
- Architectural correctness over cycle-accurate fidelity
- Ease of experimentation and modification

It is meant to be extended:
- Multi-level caches (L1/L2/L3)
- Separate instruction/data caches
- Write policies (write-back / write-through)
- Variable miss penalties
- Bandwidth and contention models
- Integration with SystemC or TLM

---

## Dependencies

- C++17 or later
- Standard C++ library
- (Optional) Python 3 with pandas and matplotlib for plotting

---

## How to Run

mkdir -p build
cmake -S . -B build
cmake --build build -j
./build/cache_sim

if you want to PLOT:
python3 plotter.py
