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