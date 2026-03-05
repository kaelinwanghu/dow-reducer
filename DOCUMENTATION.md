## 3/3/2026 (Push 1)
- Completed very broken first brute-force draft of DOW reducer which almost surely doesn't work. Will need to be corrected and refined
  - using BFS now instead of DFS for correct min reduction length counting
  - Using C input, then converting to C++ string. Have to decide and test most performant I/O options later
  - Using the original Python code, but corrected in a sense of the word to have all possibilities instead of just the max length ones
  - This has not been compiled or run at all, so that'll have to be done soon
## 3/4/2026 (Push 2)
- BFS was not a good idea. DFS + memoization works and doesn't crash the computer
- Essentially just a very unoptimized version of the original Python code, but already significantly faster
- Future ideas: arena memory, slap const + noexcept, etc. (experiment with globals/static), better reserve, more testing for speed
  - Actually make better C++ code
### Initial Testing:
- Python:
count: 64
avg per DOW (s): 3.052477
total (s): 195.374995

--- cache ---
CacheInfo(hits=33102353, misses=4099173, maxsize=None, currsize=4099173)

- C++:
--- timing ---
num_dows: 64
dow_len: 16
total solve time (sum per DOW): 10.544045 s
wall time (including IO/normalize/etc): 10.544414 s
avg per DOW: 0.164751 s
min per DOW: 0.138483 s
max per DOW: 0.363750 s
memo entries: 4099172

### Improvements:
After a slight string rescoping:
--- timing ---
num_dows: 64
dow_len: 16
total solve time (sum per DOW): 10.020671 s
wall time (including IO/normalize/etc): 10.042367 s
avg per DOW: 0.156573 s
min per DOW: 0.130637 s
max per DOW: 0.350278 s
memo entries: 4099172

After using the ankerl map/set as well as a slightly better cut hash function:
--- timing ---
num_dows: 64
dow_len: 16
total solve time (sum per DOW): 5.191784 s
wall time (including IO/normalize/etc): 5.217021 s
avg per DOW: 0.081122 s
min per DOW: 0.069016 s
max per DOW: 0.165328 s
memo entries: 4099172

## Improvements (Push 3 & 4)
- standard hashmap and set go since they suck so much
- better allocation policies (reducing hashmaps unless absolutely necessary (ie memo and patterns))

--- timing ---
num_dows: 64
dow_len: 16
total solve time (sum per DOW): 4.432056 s
wall time (including IO/normalize/etc): 4.455390 s
avg per DOW: 0.069251 s
min per DOW: 0.056447 s
max per DOW: 0.157445 s
memo entries: 4099172

- Stop the allocation in find_all_patterns by using depth-based vectors and better cut packing
- --- timing ---
num_dows: 64
dow_len: 16
total solve time (sum per DOW): 4.185458 s
wall time (including IO/normalize/etc): 4.207067 s
avg per DOW: 0.065398 s
min per DOW: 0.053322 s
max per DOW: 0.154559 s
memo entries: 4099172

-- General optimizations and details
--- timing ---
num_dows: 64
dow_len: 16
total solve time (sum per DOW): 4.019742 s
wall time (including IO/normalize/etc): 4.042954 s
avg per DOW: 0.062808 s
min per DOW: 0.048149 s
max per DOW: 0.143456 s
memo entries: 4099172