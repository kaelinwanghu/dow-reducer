#include <bits/stdc++.h>
#include <cstdio>
#include "lib/unordered_dense.h"

int32_t num_dows;
int32_t dow_len;

// Normalizes dow_chars while filling in the normalizer map and precomputing the char_positions
void normalize_dow(std::string &dow_string, ankerl::unordered_dense::map<char, char> &normalizer)
{
    char normalized_char = 0;
    for (int j = 0; j < dow_len * 2; ++j)
    {
        char current_char = dow_string[j];
        if (normalizer.count(current_char) == 0)
        {
            normalizer[current_char] = normalized_char;
            ++normalized_char;
        }

        dow_string[j] = normalizer[current_char];
    }
}

void build_char_positions(const std::string &dow_string, ankerl::unordered_dense::map<char, std::pair<int32_t, int32_t>> &char_positions)
{
    ankerl::unordered_dense::set<char> encountered;
    int32_t dow_string_size = dow_string.size();
    for (int i = 0; i < dow_string_size; ++i)
    {
        char current_char = dow_string[i];
        if (encountered.count(current_char) == 0)
        {
            char_positions[current_char].first = i;
            encountered.insert(current_char);
        }
        else
        {
            char_positions[current_char].second = i;
        }
    }
}

// multiply shift. TODO get rid of this entirely
struct cut_hash
{
    size_t operator()(const std::tuple<int32_t, int32_t, int32_t, int32_t> &t) const
    {
        size_t h = 0;
        h ^= std::hash<int32_t>{}(std::get<0>(t)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int32_t>{}(std::get<1>(t)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int32_t>{}(std::get<2>(t)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int32_t>{}(std::get<3>(t)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

// Strategy: for all the characters (in char_positions), try to both go inwards (reverse) and forwards (regular), add all occurrences to answer
ankerl::unordered_dense::set<std::tuple<int32_t, int32_t, int32_t, int32_t>, cut_hash> find_all_patterns(const std::string &current_string, ankerl::unordered_dense::map<char, std::pair<int32_t, int32_t>> &char_positions)
{
    int32_t current_string_size = current_string.size();

    ankerl::unordered_dense::set<std::tuple<int32_t, int32_t, int32_t, int32_t>, cut_hash> patterns;
    patterns.reserve(current_string_size * 2);

    for (auto it = char_positions.begin(); it != char_positions.end(); ++it)
    {
        auto [first_occurrence, second_occurrence] = it->second;
        int32_t first_next_idx = first_occurrence, second_next_idx = second_occurrence;
        char first_next_char;
        char second_next_char;
        while (second_next_idx >= 0 && second_next_idx < current_string_size)
        {
            first_next_char = current_string[first_next_idx];
            second_next_char = current_string[second_next_idx];
            if (first_next_char != second_next_char)
            {
                break;
            }
            patterns.emplace(first_occurrence, first_next_idx, second_occurrence, second_next_idx);
            ++first_next_idx;
            ++second_next_idx;
            // INCLUSIVE
        }

        // backward matching
        first_next_idx = first_occurrence, second_next_idx = second_occurrence;
        while (first_next_idx < current_string_size && second_next_idx >= 0 && first_next_idx < second_next_idx)
        {
            first_next_char = current_string[first_next_idx];
            second_next_char = current_string[second_next_idx];
            if (first_next_char != second_next_char)
            {
                break;
            }
            patterns.emplace(first_occurrence, first_next_idx, second_next_idx, second_occurrence);
            ++first_next_idx;
            --second_next_idx;
        }
    }

    return patterns;
}

int32_t solve(const std::string &dow_string, ankerl::unordered_dense::map<char, std::pair<int32_t, int32_t>> &char_positions, ankerl::unordered_dense::map<std::string, int32_t> &memo)
{
    auto it = memo.find(dow_string);
    if (it != memo.end())
    {
        return it->second;
    }
    if (dow_string.empty())
    {
        return 0;
    }

    char_positions.clear();
    build_char_positions(dow_string, char_positions);
    ankerl::unordered_dense::set<std::tuple<int32_t, int32_t, int32_t, int32_t>, cut_hash> all_patterns = find_all_patterns(dow_string, char_positions);
    int32_t min_path = INT32_MAX;
    std::string to_add;
    to_add.reserve(dow_len * 2 + 1);
    for (auto it = all_patterns.begin(); it != all_patterns.end(); ++it)
    {
        auto [first_start, first_end, second_start, second_end] = *it;
        to_add = dow_string;
        to_add.erase(to_add.begin() + second_start, to_add.begin() + second_end + 1);
        to_add.erase(to_add.begin() + first_start, to_add.begin() + first_end + 1);
        int32_t path_solution = solve(to_add, char_positions, memo) + 1;
        if (path_solution < min_path)
        {
            min_path = path_solution;
            memo[dow_string] = path_solution;
        }
    }

    return min_path;
}

int main()
{
    // Note: input must be well-formed, or else this is gonna UB
    FILE *data_file = fopen("test/data.txt", "r");
    if (data_file == NULL)
    {
        perror("Error opening file");
        exit(1);
    }
    fscanf(data_file, "%d", &num_dows);
    fscanf(data_file, "%d\n", &dow_len);

    ankerl::unordered_dense::map<char, char> normalizer;
    ankerl::unordered_dense::map<std::string, int32_t> memo;
    ankerl::unordered_dense::map<char, std::pair<int32_t, int32_t>> char_positions;
    normalizer.reserve(dow_len * 2);
    memo.reserve(1 << 20);
    char_positions.reserve(dow_len);

    char dow_chars[dow_len * 2 + 1];
    std::string dow_string;
    dow_string.reserve(dow_len * 2 + 1);

    using clock = std::chrono::steady_clock;

    double total_sec = 0.0;
    double min_sec = std::numeric_limits<double>::infinity();
    double max_sec = 0.0;
    auto t_all0 = clock::now();

    for (int i = 0; i < num_dows; ++i)
    {
        fread(dow_chars, sizeof(char), dow_len * 2 + 1, data_file);
        dow_chars[dow_len * 2] = '\0';
        dow_string = dow_chars;
        normalize_dow(dow_string, normalizer);
        auto t0 = clock::now();
        int32_t answer = solve(dow_string, char_positions, memo);
        auto t1 = clock::now();
        printf("%d\n", answer);
        double sec = std::chrono::duration<double>(t1 - t0).count();
        total_sec += sec;
        if (sec < min_sec)
        {
            min_sec = sec;
        }
        if (sec > max_sec)
        {
            max_sec = sec;
        }

        normalizer.clear();
    }
    auto t_all1 = clock::now();
    double wall_sec = std::chrono::duration<double>(t_all1 - t_all0).count();
    double avg_sec = (total_sec / num_dows);

    printf("\n--- timing ---\n");
    printf("num_dows: %d\n", num_dows);
    printf("dow_len: %d\n", dow_len);
    printf("total solve time (sum per DOW): %.6f s\n", total_sec);
    printf("wall time (including IO/normalize/etc): %.6f s\n", wall_sec);
    printf("avg per DOW: %.6f s\n", avg_sec);
    printf("min per DOW: %.6f s\n", std::isfinite(min_sec) ? min_sec : 0.0);
    printf("max per DOW: %.6f s\n", max_sec);
    printf("memo entries: %zu\n", memo.size());

    fclose(data_file);
    return 0;
}

/**
 * Input: list of DOWs, in a file (probably slow lmao)
 * Output: list of DOWs (unnormalized, same as input) plus the optimal
 * Note: DOW length means the number of unique characters/divide by 2 to get the correct length
 * DOW length for each file is gonna constant (hell yeah!)
 * To run on cmd line: input file (arg 2), out file (arg 3), plus the C++.exe
 * Finally, we need to thoroughly test that my shit is correct
 * For testing: Tristan will give file, I will see if my stuff matches (or I can generate my own tests...)
 *
 *
 * 25-30 characters is gonna be pretty good
 * DP to cache the strings (maybe, size is really bad, see if we don't need DP. Size is an issue: might have to hard limit DP size)
 * But first make a basic 1 to 1 dow from Python with DP
 * Keep stuff in standardized form (first character in alphabet is always 0, after that it's 1, 2, 3, 4, 5)
 * If it does one trial every 5 seconds, that's fine (long ass time)
 * On the scale of 3000 to 5000 trials overall, meaning runtime is gonna cap out at 3-4 hours (that's fine)
 *
 * Possible conversion ideas: one string sitting in memory perpetually, substrings will have to be created anew each time for the reduction
 * Is string_view possible? Not in most cases, because the characters we'll store are going to be disjoint from original. that sucks
 * it'll be a raw string to int map, with probably capped memoization depending on time testing
 */
