#include <bits/stdc++.h>
#include <cstdio>
#include "lib/unordered_dense.h"

// GUARANTEED to have the max unique alphabet be <= 32. The program literally will not work otherwise
static constexpr int32_t MAX_LEN = 32;
static constexpr int32_t MAX_STR = MAX_LEN * 2;
static constexpr int32_t MAX_DEPTH = MAX_LEN + 1;
static constexpr int32_t MAX_CUTS = 2048;
#define INPUT_FILE "test/data20.txt"
#define OUTPUT_FILE "test/output20.txt"

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
    uint8_t mate[MAX_STR]; // 0 to len is valid

    bool operator==(const key &other) const noexcept
    {
        uint8_t current_len = len;
        return current_len == other.len && memcmp(this->mate, other.mate, current_len) == 0;
    }
};

// <32 alphabetical characters with indices <64, cuts can be packed into a single byte easily
struct key_hash
{
    size_t operator()(const key &k) const noexcept
    {
        return ankerl::unordered_dense::detail::wyhash::hash(k.mate, k.len);
    }
};

struct context
{
    int32_t num_dows;
    int32_t dow_len;
    ankerl::unordered_dense::map<key, uint8_t, key_hash> memo_scratch; // cleared for each dow
    key temp_keys[MAX_DEPTH];
    uint16_t first_pos[256];
    cut cuts[MAX_DEPTH * MAX_CUTS];
};

static context ctx;

static inline void build_mate(const char *dow_chars, key& out) noexcept
{
    const int32_t n = ctx.dow_len * 2;
    out.len = static_cast<uint8_t>(n);
    memset(ctx.first_pos, 0xFF, sizeof(ctx.first_pos));
    for (int i = 0; i < n; ++i)
    {
        uint8_t c = static_cast<uint8_t>(dow_chars[i]);
        if (ctx.first_pos[c] == 0xFFFFu)
        {
            ctx.first_pos[c] = static_cast<uint16_t>(i);
        }
        else
        {
            uint8_t left = static_cast<uint8_t>(ctx.first_pos[c]);
            uint8_t right = static_cast<uint8_t>(i);
            out.mate[left] = right;
            out.mate[right] = left;
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
        if ((skip_mask >> idx) & 1ULL)
        {
            continue;
        }
        const uint8_t second = k.mate[idx];
        // only left
        if (idx >= second)
        {
            continue;
        }
        const int32_t first = idx;
        const int32_t orig_first = first;
        const int32_t orig_second = second;

        int32_t a = first;
        int32_t b = second;
        int32_t a_next = a + 1;
        int32_t b_next = b - 1;
        int32_t b_next_repeat = b + 1;

        // Prefer return first
        if ((b_next > a_next) && (k.mate[a_next] == b_next))
        {
            while ((a_next < b) && (b_next > a_next))
            {
                if (k.mate[a_next] != b_next)
                {
                    break;
                }
                ++a;
                a_next = a + 1;
                --b;
                b_next = b - 1;
            }

            // skip left indices
            for (int32_t j = orig_first; j <= a; ++j)
            {
                skip_mask |= (1ULL << j);
            }

            out_cuts[count++] = pack(static_cast<uint8_t>(orig_first), static_cast<uint8_t>(a), static_cast<uint8_t>(b), static_cast<uint8_t>(orig_second));
        }
        // repeat
        else if ((a_next < b) && (b_next_repeat < n) && (k.mate[a_next] == b_next_repeat))
        {
            while ((a_next < b) && (b_next_repeat < n))
            {
                if (k.mate[a_next] != b_next_repeat)
                {
                    break;
                }
                ++a;
                a_next = a + 1;
                ++b;
                b_next_repeat = b + 1;
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

    uint32_t mid_begin = first_end + 1;
    uint32_t mid_len = second_start - mid_begin;
    uint32_t tail_begin = second_end + 1;
    uint32_t new_len = src.len - (first_end - first_start + 1) - (second_end - second_start + 1);
    dest.len = static_cast<uint8_t>(new_len);
    uint8_t *out = dest.mate;

    auto get_new_pos = [&](uint32_t p){
        if (p < first_start)
        {
            return static_cast<uint8_t>(p);
        }
        if (p < second_start)
        {
            return static_cast<uint8_t>(first_start + (p - mid_begin));
        }
        // tail
        return static_cast<uint8_t>(first_start + mid_len + (p - tail_begin));
    };

    uint32_t idx = 0;
    for (uint32_t i = 0; i < first_start; ++i)
    {
        out[idx++] = get_new_pos(src.mate[i]);
    }
    for (uint32_t i = mid_begin; i < second_start; ++i)
    {
        out[idx++] = get_new_pos(src.mate[i]);
    }
    for (uint32_t i = tail_begin; i < src.len; ++i)
    {
        out[idx++] = get_new_pos(src.mate[i]);
    }
}

// recursive DFS solver with memoization
static inline uint8_t solve(const key &k, const int32_t depth)
{
    if (k.len == 0)
    {
        return 0;
    }
    auto it = ctx.memo_scratch.find(k);
    if (it != ctx.memo_scratch.end())
    {
        return it->second;
    }

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
    ctx.memo_scratch.try_emplace(k, best);
    return best;
}

int main()
{
    // Note: input must be well-formed, or else this is gonna UB
    FILE *data_file = fopen(INPUT_FILE, "r");
    fscanf(data_file, "%d", &ctx.num_dows);
    fscanf(data_file, "%d\n", &ctx.dow_len);

    ctx.memo_scratch.reserve(std::min(1 << (ctx.dow_len - 2), 1 << 20));

    int32_t dow_string_len = ctx.dow_len * 2;
    char dow_chars[MAX_STR + 1];

    key root;
    // 2 digits and a newline
    char * output_buf = static_cast<char *>(malloc(ctx.num_dows * 3));
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
        fread(dow_chars, sizeof(char), dow_string_len + 1, data_file);
        dow_chars[dow_string_len] = '\0';
        build_mate(dow_chars, root);
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
    FILE *output_file = fopen(OUTPUT_FILE, "w");
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
    printf("scratch memo entries: %zu\n", ctx.memo_scratch.size());

    fclose(data_file);
    fclose(output_file);
    free(output_buf);
    return 0;
}
