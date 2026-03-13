#include <bits/stdc++.h>
#include <cstdio>
#include <atomic>
#include <thread>
#include "lib/unordered_dense.h"

// GUARANTEED to have the max unique alphabet be <= 32.
static constexpr int32_t MAX_LEN  = 32;
static constexpr int32_t MAX_STR  = MAX_LEN * 2;
static constexpr int32_t MAX_DEPTH = MAX_LEN + 1;
static constexpr int32_t MAX_CUTS = 2048;
static constexpr int32_t WORK_CHUNK = 8;

#define INPUT_FILE  "test/data.txt"
#define OUTPUT_FILE "test/output.txt"

using cut = uint32_t;

static inline cut pack(const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d) noexcept
{
    return static_cast<uint32_t>(a) | (static_cast<uint32_t>(b) << 8) | (static_cast<uint32_t>(c) << 16) | (static_cast<uint32_t>(d) << 24);
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
    uint8_t mate[MAX_STR];

    bool operator==(const key &o) const noexcept
    {
        return len == o.len && memcmp(mate, o.mate, len) == 0;
    }
};

struct key_hash
{
    size_t operator()(const key &k) const noexcept
    {
        return ankerl::unordered_dense::detail::wyhash::hash(k.mate, k.len);
    }
};

// One context per thread so hot path is isolated
struct alignas(64) context
{
    ankerl::unordered_dense::map<key, uint8_t, key_hash> memo_scratch;
    key temp_keys[MAX_DEPTH];
    cut cuts[MAX_DEPTH * MAX_CUTS];
    uint16_t first_pos[256];
};

// Shared read-only after setup
static int32_t g_num_dows;
static int32_t g_dow_len;

// preloaded DOM flattened arrays
static char *g_dow_data = nullptr; // [num_dows][dow_string_len+1]
static uint8_t *g_answers = nullptr; // [num_dows]

static std::atomic<int32_t> g_next_dow{0};

static inline void build_mate(const char *dow_chars, key &out, context &ctx) noexcept
{
    const int32_t n = g_dow_len * 2;
    out.len = static_cast<uint8_t>(n);
    memset(ctx.first_pos, 0xFF, sizeof(ctx.first_pos));
    for (int i = 0; i < n; ++i)
    {
        uint8_t c = static_cast<uint8_t>(dow_chars[i]);
        if (ctx.first_pos[c] == 0xFFFFu)
        {
            ctx.first_pos[c] = (uint16_t)i;
        }
        else
        {
            uint8_t left  = static_cast<uint8_t>(ctx.first_pos[c]);
            uint8_t right = static_cast<uint8_t>(i);
            out.mate[left]  = right;
            out.mate[right] = left;
        }
    }
}

static inline uint16_t find_maximal_patterns(const key &k, cut *out_cuts) noexcept
{
    const int32_t n = k.len;
    uint16_t count  = 0;
    uint64_t skip_mask = 0;

    for (int32_t idx = 0; idx < n; ++idx)
    {
        if ((skip_mask >> idx) & 1ULL)
        {
            continue;
        }

        const uint8_t second = k.mate[idx];
        if (idx >= second)
        {
            continue;
        }

        const int32_t first = idx;
        const int32_t orig_first = first;
        const int32_t orig_second = second;

        int32_t a = first, b = second;
        int32_t a_next = a + 1, b_next = b - 1;
        int32_t b_next_repeat = b + 1;

        if ((b_next > a_next) && (k.mate[a_next] == b_next))
        {
            while ((a_next < b) && (b_next > a_next))
            {
                if (k.mate[a_next] != b_next) break;
                ++a; a_next = a + 1;
                --b; b_next = b - 1;
            }
            for (int32_t j = orig_first; j <= a; ++j)
            {
                skip_mask |= (1ULL << j);
            }
            out_cuts[count++] = pack(static_cast<uint8_t>(orig_first), static_cast<uint8_t>(a), static_cast<uint8_t>(b), static_cast<uint8_t>(orig_second));
        }
        else if ((a_next < b) && (b_next_repeat < n) && (k.mate[a_next] == b_next_repeat))
        {
            while ((a_next < b) && (b_next_repeat < n))
            {
                if (k.mate[a_next] != b_next_repeat) break;
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
    uint32_t mid_len   = second_start - mid_begin;
    uint32_t tail_begin = second_end + 1;
    uint32_t new_len   = src.len - (first_end - first_start + 1) - (second_end - second_start + 1);
    dest.len = static_cast<uint8_t>(new_len);
    uint8_t *out = dest.mate;

    auto get_new_pos = [&](uint32_t p) -> uint8_t
    {
        if (p < first_start) return static_cast<uint8_t>(p);
        if (p < second_start) return static_cast<uint8_t>(first_start + (p - mid_begin));
        return static_cast<uint8_t>(first_start + mid_len + (p - tail_begin));
    };

    uint32_t idx = 0;
    for (uint32_t i = 0; i < first_start; ++i) out[idx++] = get_new_pos(src.mate[i]);
    for (uint32_t i = mid_begin;  i < second_start; ++i) out[idx++] = get_new_pos(src.mate[i]);
    for (uint32_t i = tail_begin; i < src.len; ++i) out[idx++] = get_new_pos(src.mate[i]);
}

static uint8_t solve(context &ctx, const key &k, const int32_t depth)
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
        uint32_t fs, fe, ss, se;
        unpack(current_cuts[i], fs, fe, ss, se);
        apply_cut(k, next_key, fs, fe, ss, se);
        uint8_t v = solve(ctx, next_key, depth + 1) + 1;
        if (v < best) best = v;
    }

    ctx.memo_scratch.try_emplace(k, best);
    return best;
}

static void worker_thread(context *ctx, int32_t scratch_reserve)
{
    const int32_t dow_stride = g_dow_len * 2 + 1; // +1 for newline/null
    key root;

    while (true)
    {
        int32_t start = g_next_dow.fetch_add(WORK_CHUNK, std::memory_order_relaxed);
        if (start >= g_num_dows) break;

        int32_t end = std::min(start + WORK_CHUNK, g_num_dows);
        for (int32_t i = start; i < end; ++i)
        {
            const char *dow_chars = g_dow_data + i * dow_stride;
            ctx->memo_scratch.clear();
            build_mate(dow_chars, root, *ctx);
            g_answers[i] = solve(*ctx, root, 0);
        }
    }
}

// Assumes valid input!
int main(int argc, char *argv[])
{
    int32_t num_threads = std::thread::hardware_concurrency();
    if (num_threads <= 0)
    {
        num_threads = 1;
    }

    FILE *data_file = fopen(INPUT_FILE, "r");
    fscanf(data_file, "%d", &g_num_dows);
    fscanf(data_file, "%d\n", &g_dow_len);

    const int32_t dow_string_len = g_dow_len * 2;
    const int32_t dow_stride = dow_string_len + 1; // newline included

    // Pre-load all DOW strings into a flat buffer
    g_dow_data = static_cast<char *>(malloc(g_num_dows * dow_stride));
    for (int32_t i = 0; i < g_num_dows; ++i)
    {
        char *dst = g_dow_data + i * dow_stride;
        fread(dst, 1, dow_stride, data_file);
        dst[dow_string_len] = '\0';
    }
    fclose(data_file);

    g_answers = static_cast<uint8_t *>(malloc(g_num_dows));

    // Allocate threaded contexts. I don't love the C++ version but I'm using a C++ struct so I kinda have to
    int scratch_reserve = std::min(1 << (g_dow_len - 3), 1 << 20);
    std::vector<context *> ctxs(num_threads);
    for (int t = 0; t < num_threads; ++t)
    {
        ctxs[t] = new context();
        ctxs[t]->memo_scratch.reserve(scratch_reserve);
    }

    using clock = std::chrono::steady_clock;
    auto t_all0 = clock::now();

    // Launch threads
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back(worker_thread, ctxs[t], scratch_reserve);
    }
    for (auto &th : threads)
    {
        th.join();
    }

    auto t_all1 = clock::now();
    double wall_sec = std::chrono::duration<double>(t_all1 - t_all0).count();

    char *output_buf = static_cast<char *>(malloc(g_num_dows * 3));
    uint32_t output_idx = 0;
    for (int32_t i = 0; i < g_num_dows; ++i)
    {
        uint8_t answer = g_answers[i];
        if (answer >= 10)
        {
            output_buf[output_idx++] = '0' + answer / 10;
            output_buf[output_idx++] = '0' + answer % 10;
        }
        else
        {
            output_buf[output_idx++] = '0' + answer;
        }
        output_buf[output_idx++] = '\n';
    }
    FILE *output_file = fopen(OUTPUT_FILE, "w");
    fwrite(output_buf, 1, output_idx, output_file);
    fclose(output_file);

    printf("\n--- timing ---\n");
    printf("num_dows: %d\n", g_num_dows);
    printf("dow_len: %d\n", g_dow_len);
    printf("num_threads: %d\n", num_threads);
    printf("wall time: %.6f s\n", wall_sec);
    printf("avg per DOW: %.6f s\n", wall_sec / g_num_dows);

    // Cleanup
    for (int t = 0; t < num_threads; ++t)
    {
        delete ctxs[t];
    }
    free(g_dow_data);
    free(g_answers);
    free(output_buf);
    return 0;
}