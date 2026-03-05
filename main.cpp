#include <bits/stdc++.h>
#include <cstdio>

int32_t num_dows;
int32_t dow_len;

// Normalizes dow_chars while filling in the normalizer map and precomputing the char_positions
void normalize_dow(std::string& dow_string, std::unordered_map<char, char>& normalizer)
{
    // TODO: when this actually works put this down to 0 instead of the '0' char. Or maybe just add a better print function?
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

void build_char_positions(const std::string& dow_string, std::unordered_map<char, std::pair<int32_t, int32_t>>& char_positions)
{
    std::unordered_set<char> encountered;
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

struct cut_hash
{
    auto operator()(const std::tuple<int32_t, int32_t, int32_t, int32_t>& t) const
    {
        return std::hash<int32_t>{}(std::get<0>(t)) ^ std::hash<int32_t>{}(std::get<1>(t)) ^ std::hash<int32_t>{}(std::get<2>(t)) ^ std::hash<int32_t>{}(std::get<3>(t));
    }
};

// Strategy: for all the characters (in char_positions), try to both go inwards (reverse) and forwards (regular), add all occurrences to answer
std::unordered_set<std::tuple<int32_t, int32_t, int32_t, int32_t>, cut_hash> find_all_patterns(const std::string& current_string, std::unordered_map<char, std::pair<int32_t, int32_t>>& char_positions)
{
    int32_t current_string_size = current_string.size();
    // printf("current string size: %d\n", current_string_size);
    // fflush(stdout);

    std::unordered_set<std::tuple<int32_t, int32_t, int32_t, int32_t>, cut_hash> patterns;
    patterns.reserve(current_string_size * 2);

    for (auto it = char_positions.begin(); it != char_positions.end(); ++it)
    {
        auto [first_occurrence, second_occurrence] = it->second;
        // printf("char: %c, first occurrence: %d, second occurrence: %d\n", it->first, first_occurrence, second_occurrence);
        // fflush(stdout);
        // forward matching
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
            // INCLUSIVE
        }
    }

    return patterns;
}

int32_t solve(const std::string& dow_string, std::unordered_map<char, std::pair<int32_t, int32_t>>& char_positions, std::unordered_map<std::string, int32_t>& memo)
{
    if (memo.count(dow_string) > 0)
    {
        return memo[dow_string];
    }

    if (dow_string.empty())
    {
        return 0;
    }

    // printf("current string: %s, current path length: %d\n", dow_string.c_str(), current_path_len);
    // fflush(stdout);
    // printf("below min reductions len\n");
    // fflush(stdout);
    char_positions.clear();
    build_char_positions(dow_string, char_positions);
    std::unordered_set<std::tuple<int32_t, int32_t, int32_t, int32_t>, cut_hash> all_patterns = find_all_patterns(dow_string, char_positions);
    int32_t min_path = INT32_MAX;
    for (auto it = all_patterns.begin(); it != all_patterns.end(); ++it)
    {
        auto [first_start, first_end, second_start, second_end] = *it;
        // printf("first_start: %d, first_end: %d, second_start: %d, second_end: %d\n", first_start, first_end, second_start, second_end);
        // fflush(stdout);
        std::string to_emplace(dow_string);
        // Second goes before first to prevent weird index shifts from happening
        to_emplace.erase(to_emplace.begin() + second_start, to_emplace.begin() + second_end + 1);
        to_emplace.erase(to_emplace.begin() + first_start, to_emplace.begin() + first_end + 1);
        int32_t path_solution = solve(to_emplace, char_positions, memo) + 1;
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
    FILE *data_file = fopen("data.txt", "ra");
    if (data_file == NULL)
    {
        perror("Error opening file");
        exit(1);
    }
    fscanf(data_file, "%d", &num_dows);
    fscanf(data_file, "%d\n", &dow_len);

    std::unordered_map<char, char> normalizer;
    std::unordered_map<std::string, int32_t> memo;
    memo.reserve(num_dows * num_dows);
    std::unordered_map<char, std::pair<int32_t, int32_t>> char_positions;
    normalizer.reserve(dow_len * 2);
    char_positions.reserve(dow_len);

    char dow_chars[dow_len * 2 + 1];
    std::string dow_string;
    dow_string.reserve(dow_len * 2 + 1);

    for (int i = 0; i < num_dows; ++i)
    {
        fread(dow_chars, sizeof(char), dow_len * 2 + 1, data_file);
        dow_chars[dow_len * 2] = '\0';
        dow_string = dow_chars;
        normalize_dow(dow_string, normalizer);
        int32_t answer = solve(dow_string, char_positions, memo);
        printf("%d\n", answer);

        normalizer.clear();
    }

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
