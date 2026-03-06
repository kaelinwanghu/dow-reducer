#include <bits/stdc++.h>
#include <cstdio>
#include "lib/unordered_dense.h"

// GUARANTEED to have the max unique alphabet be <= 32. The program literally will not work otherwise
static constexpr int32_t MAX_LEN = 32;
static constexpr int32_t MAX_STR = MAX_LEN * 2;
static constexpr int32_t MAX_DEPTH = MAX_LEN + 1;
static constexpr int32_t MAX_CUTS = 2048;
static constexpr int32_t GLOBAL_MEMO_LEN = 10;

using cut = uint32_t;
static inline cut pack(const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d) noexcept
{
    return (static_cast<uint32_t>(a)) | (static_cast<uint32_t>(b) << 8) | (static_cast<uint32_t>(c) << 16) | (static_cast<uint32_t>(d) << 24);
}
static inline void unpack(cut ct, uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d) noexcept
{
    a = ct & 0xFF;
    b = (ct >> 8) & 0xFF;
    c = (ct >> 16) & 0xFF;
    d = (ct >> 24) & 0xFF;
}

struct key
{
    uint8_t len;
    uint8_t s[MAX_LEN * 2]; // 0 to len is valid

    bool operator==(const key &other) const noexcept
    {
        uint8_t current_len = len;
        return current_len == other.len && memcmp(this->s, other.s, current_len) == 0;
    }
};

// <32 alphabetical characters with indices <64, cuts can be packed into a single byte easily
struct key_hash
{
    size_t operator()(const key &k) const noexcept
    {
        return ankerl::unordered_dense::detail::wyhash::hash(k.s, k.len);
    }
};

struct context
{
    int32_t num_dows;
    int32_t dow_len;
    uint32_t current_stamp;
    ankerl::unordered_dense::map<key, uint8_t, key_hash> memo_global;  // len <= GLOBAL_MEMO_LEN only
    ankerl::unordered_dense::map<key, uint8_t, key_hash> memo_scratch; // cleared for each dow
    uint16_t char_positions[MAX_LEN];
    uint32_t stamp[MAX_LEN];
    uint8_t normalizer[256];
    key temp_keys[MAX_DEPTH];
    cut cuts[MAX_DEPTH * MAX_CUTS];
};

static context ctx;

// Normalizes dow_chars while filling in the normalizer map and precomputing the char_positions
static inline void normalize_dow(const char *dow_chars, key &out) noexcept
{
    int32_t dow_len = ctx.dow_len * 2;
    out.len = dow_len;
    uint8_t normalizing_char = 0;
    for (int i = 0; i < dow_len; ++i)
    {
        uint8_t current_char = static_cast<uint8_t>(dow_chars[i]);
        uint8_t normalized = ctx.normalizer[current_char];
        if (normalized == 0xFF)
        {
            normalized = normalizing_char;
            ++normalizing_char;
            ctx.normalizer[current_char] = normalized;
        }

        out.s[i] = normalized;
    }
}

// uses dow_string to build character positions
static inline void build_char_positions(const key &k) noexcept
{
    uint32_t new_stamp = ++ctx.current_stamp;
    int32_t dow_size = k.len;
    for (int i = 0; i < dow_size; ++i)
    {
        uint8_t current_char = static_cast<uint8_t>(k.s[i]);
        if (ctx.stamp[current_char] != new_stamp)
        {
            ctx.stamp[current_char] = new_stamp;
            ctx.char_positions[current_char] = (static_cast<uint16_t>(i) << 8) | 0xFF;
        }
        else
        {
            uint16_t pos = ctx.char_positions[current_char];
            ctx.char_positions[current_char] = (pos & 0xFF00u) | static_cast<uint16_t>(i);
        }
    }
}

// This is cobbled together but like efficient for the most part. Dunno why it works
static inline uint16_t find_maximal_patterns(const key &k, cut *out_cuts) noexcept
{
    const int32_t n = k.len;
    uint16_t count = 0;

    // bitmask over positions
    uint64_t skip_mask = 0;

    for (int32_t idx = 0; idx < n; ++idx)
    {
        const uint8_t current_char = k.s[idx];
        const uint16_t pos = ctx.char_positions[current_char];
        const int32_t first = (pos >> 8) & 0xFF;
        const int32_t second = pos & 0xFF;

        if (idx != first)
        {
            continue;
        }

        // Skip by left index
        if ((skip_mask >> idx) & 1ULL)
        {
            continue;
        }

        const int32_t orig_first = first;
        const int32_t orig_second = second;

        int32_t a = first;
        int32_t b = second;

        // Prefer return first
        if ((b - 1 > a + 1) && (k.s[a + 1] == k.s[b - 1]))
        {
            while ((a + 1 < b) && (b - 1 > a + 1))
            {
                if (k.s[a + 1] != k.s[b - 1])
                {
                    break;
                }
                ++a;
                --b;
            }

            // skip left indices
            for (int32_t j = orig_first; j <= a; ++j)
            {
                skip_mask |= (1ULL << j);
            }

            out_cuts[count++] = pack(static_cast<uint8_t>(orig_first), static_cast<uint8_t>(a), static_cast<uint8_t>(b), static_cast<uint8_t>(orig_second));
        }
        // repeat
        else if ((a + 1 < b) && (b + 1 < n) && (k.s[a + 1] == k.s[b + 1]))
        {
            while ((a + 1 < b) && (b + 1 < n))
            {
                if (k.s[a + 1] != k.s[b + 1])
                {
                    break;
                }
                ++a;
                ++b;
            }

            for (int32_t j = orig_first; j <= a; ++j)
            {
                skip_mask |= (1ULL << j);
            }

            out_cuts[count++] = pack(static_cast<uint8_t>(orig_first), static_cast<uint8_t>(a), static_cast<uint8_t>(orig_second), static_cast<uint8_t>(b));
        }
        else
        {
            skip_mask |= (1ULL << orig_first);

            out_cuts[count++] = pack(static_cast<uint8_t>(orig_first), static_cast<uint8_t>(orig_first), static_cast<uint8_t>(orig_second), static_cast<uint8_t>(orig_second));
        }
    }

    return count;
}

static inline void apply_cut(const key &src, key &dest, uint32_t first_start, uint32_t first_end, uint32_t second_start, uint32_t second_end) noexcept
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
static inline uint8_t solve(const key &k, const int32_t depth)
{
    if (k.len == 0)
    {
        return 0;
    }
    if (k.len <= GLOBAL_MEMO_LEN)
    {
        auto it = ctx.memo_global.find(k);
        if (it != ctx.memo_global.end())
            return it->second;
    }
    else
    {
        auto it = ctx.memo_scratch.find(k);
        if (it != ctx.memo_scratch.end())
        {
            return it->second;
        }
    }
    build_char_positions(k);

    cut *current_cuts = ctx.cuts + (depth * MAX_CUTS);

    uint16_t cut_count = find_maximal_patterns(k, current_cuts);

    uint8_t best = UINT8_MAX;
    key &next_key = ctx.temp_keys[depth];
    for (uint16_t i = 0; i < cut_count; ++i)
    {
        const cut ct = current_cuts[i];
        uint32_t first_start, first_end, second_start, second_end;
        unpack(ct, first_start, first_end, second_start, second_end);

        apply_cut(k, next_key, first_start, first_end, second_start, second_end);

        uint8_t current_path = solve(next_key, depth + 1) + 1;
        if (current_path < best)
        {
            best = current_path;
        }
    }
    if (k.len <= GLOBAL_MEMO_LEN) ctx.memo_global.try_emplace(k, best);
    else ctx.memo_scratch.try_emplace(k, best);
    return best;
}

int main()
{
    // Note: input must be well-formed, or else this is gonna UB
    FILE *data_file = fopen("test/data.txt", "r");
    fscanf(data_file, "%d", &ctx.num_dows);
    fscanf(data_file, "%d\n", &ctx.dow_len);

    ctx.memo_global.reserve(1 << 22);
    ctx.memo_scratch.reserve(1 << 16);

    int32_t dow_string_len = ctx.dow_len * 2;
    char dow_chars[dow_string_len + 1];

    key root;
    // 2 digits and a newline
    char output_buf[ctx.num_dows * 3];
    uint32_t output_idx = 0;

    using clock = std::chrono::steady_clock;

    double total_sec = 0.0;
    double min_sec = std::numeric_limits<double>::infinity();
    double max_sec = 0.0;
    auto t_all0 = clock::now();

    // size_t prev_memo_size = 0;
    for (int i = 0; i < ctx.num_dows; ++i)
    {
        // if (i != 0 && i % 100 == 0)
        // {
        //     printf("idx=%d memo_size=%zu prev_memo_size=%zu bucket_count=%zu load=%f\n",
        //            i,
        //            ctx.memo.size(),
        //            ctx.memo.size() - prev_memo_size,
        //            ctx.memo.bucket_count(),
        //            ctx.memo.load_factor());
        //     prev_memo_size = ctx.memo.size();
        // }
        ctx.memo_scratch.clear();
        memset(ctx.normalizer, 0xFF, sizeof(ctx.normalizer));
        fread(dow_chars, sizeof(char), dow_string_len + 1, data_file);
        dow_chars[dow_string_len] = '\0';
        normalize_dow(dow_chars, root);
        auto t0 = clock::now();
        uint8_t answer = solve(root, 0);
        auto t1 = clock::now();

        if (answer >= 10)
        {
            output_buf[output_idx++] = ('0' + answer / 10);
            output_buf[output_idx++] = ('0' + answer % 10);
        }
        else
        {
            output_buf[output_idx++] = '0' + answer;
        }
        output_buf[output_idx++] = '\n';

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
    FILE *output_file = fopen("test/output.txt", "w");
    fwrite(output_buf, 1, output_idx, output_file);

    auto t_all1 = clock::now();
    double wall_sec = std::chrono::duration<double>(t_all1 - t_all0).count();
    double avg_sec = (total_sec / ctx.num_dows);

    printf("\n--- timing ---\n");
    printf("num_dows: %d\n", ctx.num_dows);
    printf("dow_len: %d\n", ctx.dow_len);
    printf("total solve time (sum per DOW): %.6f s\n", total_sec);
    printf("wall time (including IO/normalize/etc): %.6f s\n", wall_sec);
    printf("avg per DOW: %.6f s\n", avg_sec);
    printf("min per DOW: %.6f s\n", std::isfinite(min_sec) ? min_sec : 0.0);
    printf("max per DOW: %.6f s\n", max_sec);
    printf("global memo entries: %zu\n", ctx.memo_global.size());
    printf("scratch memo entries: %zu\n", ctx.memo_scratch.size());

    fclose(data_file);
    fclose(output_file);
    return 0;
}
