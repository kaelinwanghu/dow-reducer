#include <bits/stdc++.h>
#include <cstdio>
#include <cstdint>
#include <random>

static constexpr int MAX_DOW_LEN = 32;

static constexpr char ALPHABET[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#$%&()*+,-./:;<=>?@[]^_{|}~";

static constexpr int ALPHABET_SIZE = sizeof(ALPHABET) - 1;

static void die(const char* msg) {
    fprintf(stderr, "error: %s\n", msg);
    exit(1);
}

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <num_dows> <dow_len> <output_path>\n", argv[0]);
        return 1;
    }

    char* end1 = nullptr;
    char* end2 = nullptr;

    int32_t num_dows = strtol(argv[1], &end1, 10);
    int32_t dow_len  = strtol(argv[2], &end2, 10);

    if (*argv[1] == '\0' || *end1 != '\0')
    {
        die("num_dows must be an integer");
    }
    if (*argv[2] == '\0' || *end2 != '\0')
    {
        die("dow_len must be an integer");
    }
    if (num_dows <= 0)
    {
        die("num_dows must be > 0");
    }
    if (dow_len <= 0)
    {
        die("dow_len must be > 0");
    }
    if (dow_len > MAX_DOW_LEN)
    {
        die("dow_len must be <= 32");
    }

    const int32_t total_len = dow_len * 2;
    const char* output_path = argv[3];

    FILE* out = fopen(output_path, "wb");
    if (!out)
    {
        die("could not open output file");
    }

    static constexpr size_t WRITE_BUF_SIZE = 1 << 20;
    char *io_buf = static_cast<char *>(malloc(WRITE_BUF_SIZE));
    if (io_buf)
    {
        setvbuf(out, io_buf, _IOFBF, WRITE_BUF_SIZE);
    }

    fprintf(out, "%d\n%d\n", num_dows, dow_len);

    std::random_device rd;
    std::mt19937_64 rng((static_cast<uint64_t>(rd()) << 32) ^ static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

    char alphabet_pool[ALPHABET_SIZE];
    memcpy(alphabet_pool, ALPHABET, ALPHABET_SIZE);
    char word[2 * MAX_DOW_LEN + 1];

    for (int i = 0; i < num_dows; ++i)
    {
        // Randomize which  symbols are used
        std::shuffle(alphabet_pool, alphabet_pool + ALPHABET_SIZE, rng);

        for (int j = 0; j < dow_len; ++j)
        {
            const char c = alphabet_pool[j];
            word[2 * j] = c;
            word[2 * j + 1] = c;
        }

        // Randomize actual arrangement
        std::shuffle(word, word + total_len, rng);
        word[total_len] = '\n';

        if (fwrite(word, 1, total_len + 1, out) != static_cast<size_t>(total_len + 1))
        {
            fclose(out);
            free(io_buf);
            die("failed while writing output");
        }
    }

    fclose(out);
    free(io_buf);
    return 0;
}