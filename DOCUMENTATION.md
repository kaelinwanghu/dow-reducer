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

After getting rid of vectors entirely and using only 1D arrays:
--- timing ---
num_dows: 128
dow_len: 16
total solve time (sum per DOW): 6.230493 s
wall time (including IO/normalize/etc): 6.231171 s
avg per DOW: 0.048676 s
min per DOW: 0.039020 s
max per DOW: 0.065272 s
memo entries: 8113853

After more tuning with stamps and global context:
--- timing ---
num_dows: 128
dow_len: 16
total solve time (sum per DOW): 5.761247 s
wall time (including IO/normalize/etc): 5.761983 s
avg per DOW: 0.045010 s
min per DOW: 0.038821 s
max per DOW: 0.052827 s
memo entries: 8113853

After investigation, it turns out that the memoization was hitting, but the global memo was counterproductive since it got too big for caching
memo hits: 1709118255
memo misses: 59500563
memo inserts: 59500563
(~95% hit rate, but stable from the first indices forward):
memo hits: 43802909
memo misses: 1599478
memo inserts: 1599478


And this was with memoization limiting that ensured that for 16-length DOWs, only 24-length strings and below would be cached. Without this bound:
memo hits: 999423326
memo misses: 60197563
memo inserts: 60197563
(~94% hit rate, slightly worse)
Similarly the 94% hit rate maintained itself throughout the run (1024 16-length dows)
memo hits: 26069481
memo misses: 1616903
memo inserts: 1616903

So in conclusion the global memoization was actively making everything slower without helping anything.
But the original Python function is here to help and save us from the super-expanding memoization table, which led to this
--- timing ---
num_dows: 256
dow_len: 16
total solve time (sum per DOW): 6.894903 s
wall time (including IO/normalize/etc): 6.896016 s
avg per DOW: 0.026933 s
min per DOW: 0.001659 s
max per DOW: 0.073736 s
memo entries: 10107182

We still have to solve the problem of the ever-expanding and coldening memo table, but this was a nice optimization

Idea one: do nothing and test:

--- timing ---
num_dows: 5052
dow_len: 16
total solve time (sum per DOW): 206.253018 s
wall time (including IO/normalize/etc): 206.273020 s
avg per DOW: 0.040826 s
min per DOW: 0.000777 s
max per DOW: 0.325432 s
memo entries: 174902154

Idea two: use length-divided memoization

--- timing ---
num_dows: 5052
dow_len: 16
total solve time (sum per DOW): 446.794479 s
wall time (including IO/normalize/etc): 446.822204 s
avg per DOW: 0.088439 s
min per DOW: 0.000731 s
max per DOW: 17.682028 s
memo entries: 174902154

(garbage)

Idea three: What if we did length division but had better allocation

--- timing ---
num_dows: 5052
dow_len: 16
total solve time (sum per DOW): 293.281797 s
wall time (including IO/normalize/etc): 293.310077 s
avg per DOW: 0.058053 s
min per DOW: 0.000787 s
max per DOW: 0.361577 s
memo entries: 174902154


Idea four: small global pool (living in L3 cache hopefully) plus local per-DOW memoization

--- timing ---
num_dows: 5052
dow_len: 16
total solve time (sum per DOW): 68.976476 s
wall time (including IO/normalize/etc): 69.073878 s
avg per DOW: 0.013653 s
min per DOW: 0.000269 s
max per DOW: 0.038792 s
global memo entries: 3491646
scratch memo entries: 14187

(nice)

So that works (with good allocation)