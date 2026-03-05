#include <bits/stdc++.h>
#include <cstdio>
#include "lib/unordered_dense.h"

int32_t num_dows;
int32_t dow_len;
static constexpr int MAX_LEN = 32;
static constexpr int MAX_DEPTH = MAX_LEN + 1;
static constexpr int MAX_CUTS = 2048; // 64^2

struct key
{
    uint8_t len;
    uint8_t s[MAX_LEN * 2]; // 0 to len is valid

    bool operator==(const key& other) const noexcept
    {
        uint8_t current_len = len;
        return current_len == other.len && memcmp(this->s, other.s, current_len) == 0;
    }
};

struct key_hash
{
    size_t operator()(const key& k) const noexcept
    {
        return ankerl::unordered_dense::detail::wyhash::hash(k.s, k.len);
    }
};

// Normalizes dow_chars while filling in the normalizer map and precomputing the char_positions
void normalize_dow(const char *dow_chars, key& out, uint8_t *normalizer) noexcept
{
    out.len = dow_len * 2;
    uint8_t normalizing_char = 0;
    for (int i = 0; i < dow_len * 2; ++i)
    {
        uint8_t current_char = static_cast<uint8_t>(dow_chars[i]);
        uint8_t normalized = normalizer[current_char];
        if (normalized == 0xFF)
        {
            normalized = normalizing_char;
            ++normalizing_char;
            normalizer[current_char] = normalized;
        }

        out.s[i] = normalized;
    }
}

// uses dow_string to build character positions
void build_char_positions(const key &k, uint16_t *char_positions) noexcept
{
    int32_t dow_size = k.len;
    for (int i = 0; i < dow_size; ++i)
    {
        uint8_t current_char = static_cast<uint8_t>(k.s[i]);
        uint16_t pos = char_positions[current_char];
        // first position is first byte, second position is second byte
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

// Strategy: for all the characters (in char_positions), try to both go inwards (reverse) and forwards (regular), add all occurrences to all_patterns
void find_all_patterns(const key &k, uint16_t *char_positions, cut * out_cuts, uint16_t& out_count)
{
    int32_t current_string_size = k.len;
    out_count = 0;

    for (int i = 0; i < 32; ++i)
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
            if (k.s[first_next_idx] != k.s[second_next_idx])
            {
                break;
            }
            out_cuts[out_count] = pack(first_occurrence, first_next_idx, second_occurrence, second_next_idx);
            ++out_count;
            ++first_next_idx;
            ++second_next_idx;
            // INCLUSIVE
        }

        // backward matching
        first_next_idx = first_occurrence, second_next_idx = second_occurrence;
        while (first_next_idx < second_next_idx)
        {
            if (k.s[first_next_idx] != k.s[second_next_idx])
            {
                break;
            }
            out_cuts[out_count] = pack(first_occurrence, first_next_idx, second_next_idx, second_occurrence);
            ++out_count;
            ++first_next_idx;
            --second_next_idx;
        }
    }
}

static inline void apply_cut(const key& src, key& dest, uint32_t first_start, uint32_t first_end, uint32_t second_start, uint32_t second_end) noexcept
{
    uint8_t *out = dest.s;
    uint32_t new_len = 0;

    if (first_start)
    {
        memcpy(out + new_len, src.s, first_start);
        new_len += first_start;
    }
    uint32_t mid_begin = first_end + 1;

    // middle
    uint32_t mid_len = second_start - mid_begin;
    if (mid_len) 
    {
        memcpy(out + new_len, src.s + mid_begin, mid_len);
        new_len += mid_len;
    }

    uint32_t tail_begin = second_end + 1;
    uint32_t tail_len = src.len - tail_begin;
    if (tail_len)
    {
        memcpy(out + new_len, src.s + tail_begin, tail_len);
        new_len += tail_len;
    }
    dest.len = new_len;
}

// recursive DFS solver with memoization
uint8_t solve(const key &k, uint16_t *char_positions, ankerl::unordered_dense::map<key, uint8_t, key_hash> &memo, cut *depth_cuts, uint16_t *depth_cut_counts, key * temp_keys, const int32_t depth)
{
    if (k.len == 0)
    {
        return 0;
    }
    auto it = memo.find(k);
    if (it != memo.end())
    {
        return it->second;
    }

    // Reset character positions for re-normalization
    memset(char_positions, 0xFF, 32 * sizeof(uint16_t));
    build_char_positions(k, char_positions);

    cut * current_cuts = depth_cuts + (depth * MAX_CUTS);
    uint16_t cut_count = depth_cut_counts[depth];

    find_all_patterns(k, char_positions, current_cuts, cut_count);

    uint8_t min_path = UINT8_MAX;
    key &next_key = temp_keys[depth];
    for (uint16_t i = 0; i < cut_count; ++i)
    {
        const cut ct = current_cuts[i];
        uint32_t first_start, first_end, second_start, second_end;
        unpack(ct, first_start, first_end, second_start, second_end);

        apply_cut(k, next_key, first_start, first_end, second_start, second_end);

        uint8_t current_path = solve(next_key, char_positions, memo, depth_cuts, depth_cut_counts, temp_keys, depth + 1) + 1;
        if (current_path < min_path)
        {
            min_path = current_path;
        }
    }
    memo.try_emplace(k, min_path);
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

    uint8_t normalizer[256];
    ankerl::unordered_dense::map<key, uint8_t, key_hash> memo;
    uint16_t char_positions[MAX_LEN];
    memo.reserve(1 << 23);
    memset(char_positions, 0xFF, sizeof(char_positions));

    char dow_chars[dow_len * 2 + 1];
    std::string dow_string;
    dow_string.reserve(dow_len * 2 + 1);

    // For all_patterns to avoid continuous memory allocation (capped at alphabet of 32)
    static constexpr int32_t MAX_DEPTH = MAX_LEN + 1;
    cut depth_cuts[MAX_DEPTH * MAX_CUTS];
    uint16_t depth_cut_counts[MAX_DEPTH];
    key temp_keys[MAX_DEPTH];

    key root;

    using clock = std::chrono::steady_clock;

    double total_sec = 0.0;
    double min_sec = std::numeric_limits<double>::infinity();
    double max_sec = 0.0;
    auto t_all0 = clock::now();

    for (int i = 0; i < num_dows; ++i)
    {
        memset(normalizer, 0xFF, sizeof(normalizer));
        fread(dow_chars, sizeof(char), dow_len * 2 + 1, data_file);
        dow_chars[dow_len * 2] = '\0';
        normalize_dow(dow_chars, root, normalizer);
        auto t0 = clock::now();
        int32_t answer = solve(root, char_positions, memo, depth_cuts, depth_cut_counts, temp_keys, 0);
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
