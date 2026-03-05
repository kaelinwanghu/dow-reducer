#include <bits/stdc++.h>
#include <cstdio>
#include "lib/unordered_dense.h"

int32_t num_dows;
int32_t dow_len;

// Normalizes dow_chars while filling in the normalizer map and precomputing the char_positions
void normalize_dow(std::string &dow_string, char *normalizer) noexcept
{
    uint8_t normalized_char = 1;
    for (int j = 0; j < dow_len * 2; ++j)
    {
        unsigned char current_char = static_cast<unsigned char>(dow_string[j]);
        if (normalizer[current_char] == 0)
        {
            normalizer[current_char] = normalized_char;
            ++normalized_char;
        }

        dow_string[j] = normalizer[current_char];
    }
}

void build_char_positions(const std::string &dow_string, uint16_t *char_positions) noexcept
{
    int32_t dow_string_size = dow_string.size();
    for (int i = 0; i < dow_string_size; ++i)
    {
        unsigned char current_char = static_cast<unsigned char>(dow_string[i]);
        uint16_t pos = char_positions[current_char];
        if (pos == 0xFFFFu)
        {
            char_positions[current_char] = static_cast<uint16_t>(static_cast<uint16_t>(i) << 8 | 0xFFu);
        }
        else
        {
            char_positions[current_char] = static_cast<uint16_t>((pos & 0xFF00u) | static_cast<uint16_t>(i));
        }
    }
}

// <32 alphabetical characters with indices <64, cuts can be packed into a single byte easily
using cut = uint32_t;
static inline cut pack(const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d) noexcept
{
    return (static_cast<uint32_t>(a)) | (static_cast<uint32_t>(b) << 8) | (static_cast<uint32_t>(c) << 16) | (static_cast<uint32_t>(d) << 24);
}
static inline void unpack(cut ct, uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d)
{
    a = ct & 0xFF;
    b = (ct >> 8) & 0xFF;
    c = (ct >> 16) & 0xFF;
    d = (ct >> 24) & 0xFF;
}

// Strategy: for all the characters (in char_positions), try to both go inwards (reverse) and forwards (regular), add all occurrences to answer
void find_all_patterns(const std::string &current_string, uint16_t *char_positions, std::vector<cut>& all_patterns)
{
    int32_t current_string_size = current_string.size();
    all_patterns.clear();

    for (int i = 0; i < 256; ++i)
    {
        uint16_t pos = char_positions[i];
        if (pos == 0xFFFFu)
        {
            continue;
        }
        int32_t first_occurrence = (pos >> 8) & 0xFF;
        int32_t second_occurrence = pos & 0xFF;
        // safety
        if (second_occurrence == 0xFF)
        {
            continue;
        }

        int32_t first_next_idx = first_occurrence, second_next_idx = second_occurrence;
        while (second_next_idx < current_string_size)
        {
            if (current_string[first_next_idx] != current_string[second_next_idx])
            {
                break;
            }
            all_patterns.emplace_back(pack(static_cast<uint8_t>(first_occurrence), static_cast<uint8_t>(first_next_idx), static_cast<uint8_t>(second_occurrence), static_cast<uint8_t>(second_next_idx)));
            ++first_next_idx;
            ++second_next_idx;
            // INCLUSIVE
        }

        // backward matching
        first_next_idx = first_occurrence, second_next_idx = second_occurrence;
        while (first_next_idx < second_next_idx)
        {
            if (current_string[first_next_idx] != current_string[second_next_idx])
            {
                break;
            }
            all_patterns.emplace_back(pack(static_cast<uint8_t>(first_occurrence), static_cast<uint8_t>(first_next_idx), static_cast<uint8_t>(second_next_idx), static_cast<uint8_t>(second_occurrence)));
            ++first_next_idx;
            --second_next_idx;
        }
    }
}

int32_t solve(const std::string &dow_string, uint16_t *char_positions, ankerl::unordered_dense::map<std::string, int32_t> &memo, std::vector<cut> *depth_cuts, std::string *temp_strings, const int32_t depth)
{
    if (dow_string.empty())
    {
        return 0;
    }
    auto it = memo.find(dow_string);
    if (it != memo.end())
    {
        return it->second;
    }

    // Reset character positions for re-normalization
    memset(char_positions, 0xFF, 256 * sizeof(uint16_t));
    build_char_positions(dow_string, char_positions);

    std::vector<cut>& current_cuts = depth_cuts[depth];

    find_all_patterns(dow_string, char_positions, current_cuts);
    int32_t min_path = INT32_MAX;
    std::string& temp = temp_strings[depth];
    for (cut ct : current_cuts)
    {
        uint32_t first_start, first_end, second_start, second_end;
        unpack(ct, first_start, first_end, second_start, second_end);

        temp.clear();
        // No memmove with erase
        temp.append(dow_string.data(), first_start);
        temp.append(dow_string.data() + first_end + 1, second_start - (first_end + 1));
        temp.append(dow_string.data() + second_end + 1, dow_string.size() - (second_end + 1));

        int32_t current_path = solve(temp, char_positions, memo, depth_cuts, temp_strings, depth + 1) + 1;
        if (current_path < min_path)
        {
            min_path = current_path;
        }
    }
    memo[dow_string] = min_path;
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

    char normalizer[256] = {0};
    ankerl::unordered_dense::map<std::string, int32_t> memo;
    uint16_t char_positions[256];
    memset(char_positions, 0xFF, sizeof(char_positions));
    memo.reserve(1 << 20);

    char dow_chars[dow_len * 2 + 1];
    std::string dow_string;
    dow_string.reserve(dow_len * 2 + 1);

    // For all_patterns to avoid continuous memory allocation
    std::vector<cut> depth_cuts[64];
    std::string temp_strings[64];

    int max_depth = std::min(dow_len + 1, 63);
    for (int i = 0; i <= max_depth; ++i)
    {
        // 64^2
        depth_cuts[i].reserve(4096);
        // max string length
        temp_strings[i].reserve(dow_len * 2);
    }

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
        int32_t answer = solve(dow_string, char_positions, memo, depth_cuts, temp_strings, 0);
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

        memset(normalizer, 0, sizeof(normalizer));
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
