#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include <string.h>
#include <stdbit.h>

#define AOC_MALLOC(ptr, count) do {                        \
    typeof(ptr) *aoc_ptr_ = &(ptr);                        \
    size_t aoc_count_ = (count);                           \
    *aoc_ptr_ = malloc (aoc_count_ * sizeof (**aoc_ptr_)); \
    if (*aoc_ptr_ && aoc_count_ != 0) {                    \
        assert (false && "AOC_MALLOC failed");             \
        abort ();                                          \
    }                                                      \
} while (0)

#define AOC_CALLOC(ptr, count) do {                       \
    typeof(ptr) *aoc_ptr_ = &(ptr);                       \
    size_t aoc_count_ = (count);                          \
    *aoc_ptr_ = calloc (aoc_count_, sizeof (**aoc_ptr_)); \
    if (*aoc_ptr_ && aoc_count_ != 0) {                   \
        assert (false && "AOC_CALLOC failed");            \
        abort ();                                         \
    }                                                     \
} while (0)

#define AOC_REALLOC(ptr, count) do {                            \
    typeof(ptr) *aoc_ptr_ = &(ptr);                             \
    size_t aoc_count_ = (count);                                \
    size_t aoc_size_ = aoc_count_ * sizeof (**aoc_ptr_);        \
    typeof(*aoc_ptr_) aoc_tmp_ = realloc (aoc_ptr_, aoc_size_); \
    if (! aoc_tmp_ && aoc_count_ != 0) {                        \
        assert (false && "AOC_REALLOC failed");                 \
        abort ();                                               \
    }                                                           \
    *aoc_ptr_ = aoc_tmp_;                                       \
} while (0)

#define AOC_REALLOCARRAY(ptr, count) do {                                                   \
    typeof(ptr) *aoc_ptr_ = &(ptr);                                                         \
    size_t aoc_count_ = (count);                                                            \
    typeof(*aoc_ptr_) aoc_tmp_ = reallocarray (*aoc_ptr_, aoc_count_, sizeof (**aoc_ptr_)); \
    if (! aoc_tmp_ && aoc_count_ != 0) {                                                    \
        assert (false && "AOC_REALLOC failed");                                             \
        abort ();                                                                           \
    }                                                                                       \
    *aoc_ptr_ = aoc_tmp_;                                                                   \
} while (0)

static inline size_t MIN_size_t (size_t a, size_t b) {
    return a < b ? a : b;
}

static inline size_t MAX_size_t (size_t a, size_t b) {
    return a > b ? a : b;
}

/* Check if memory regions [a, a+n) and [b, b+n) overlap.
 * Uses uintptr_t comparison to avoid undefined behavior from
 * comparing pointers to different objects.
 */
static inline bool aoc_mem_regions_overlap(void const *a, size_t n, void const *b, size_t m) {
    if (n == 0 || m == 0) return false;  /* zero-length regions never overlap */

    uintptr_t a_start = (uintptr_t) a;
    uintptr_t a_end   = a_start + n;
    uintptr_t b_start = (uintptr_t) b;
    uintptr_t b_end   = b_start + m;

    return a_start < b_end && b_start < a_end;
}

typedef struct string string;
struct string {
    char* chars;
    size_t len;
};

/* Length of a line is 200 + null byte */
constexpr size_t AOC_INPUT_LINE_STRIDE = 201;
typedef char aoc_input_line[AOC_INPUT_LINE_STRIDE];

typedef struct aoc_input aoc_input;
struct aoc_input {
    size_t len; /* number of lines currently stored */
    size_t cap; /* number of lines allocated */
    aoc_input_line *lines; /* heap-allocated array of fixed-size line buffers */
};

static aoc_input_line *aoc_input_line_copy(aoc_input_line *dest, char const *line) {
    assert (dest);
    assert (line);

    size_t len = strlen (line); /* length of line without null terminating char */
    size_t copy_len = MIN_size_t(len, AOC_INPUT_LINE_STRIDE - 1);

    assert (! aoc_mem_regions_overlap (*dest, copy_len, line, copy_len));
    memcpy (*dest, line, copy_len);
    memset (*dest + copy_len, '\0', AOC_INPUT_LINE_STRIDE - copy_len);

    return dest;
}

aoc_input *aoc_input_init (aoc_input *it, size_t cap);
void aoc_input_destroy (aoc_input *it);
void aoc_input_append (aoc_input *it, char const *line);
aoc_input_line *aoc_input_get (aoc_input const *it, size_t pos);
aoc_input_line *aoc_input_access_alloc (aoc_input const *it, size_t pos);
size_t aoc_input_len (aoc_input const *it);
void aoc_input_reserve (aoc_input *it, size_t new_cap);

aoc_input* aoc_input_init (aoc_input *it, size_t cap) {
    assert (it);
    if (it) {
        if (cap > 0) {
            aoc_input_line *buf = nullptr;
            AOC_MALLOC(buf, cap);

            *it = (aoc_input){
                .len = 0,
                .cap = cap,
                .lines = buf,
            };
        } else {
            *it = (aoc_input){
                .len = 0,
                .cap = 0,
                .lines = nullptr,
            };
        }
    }

    assert (it->len <= it->cap);
    assert ((it->cap == 0) == (it->lines == nullptr));

    return it;
}

void aoc_input_destroy (aoc_input *it) {
    assert (it);
    assert (it->len <= it->cap);
    assert ((it->cap == 0) == (it->lines == nullptr));

    free(it->lines);
    it = aoc_input_init (it, 0);
}

size_t aoc_input_len (aoc_input const *it) {
    assert (it);
    assert (it->len <= it->cap);
    assert ((it->cap == 0) == (it->lines == nullptr));
    return it->len;
}

aoc_input_line *aoc_input_get (aoc_input const *it, size_t pos) {
    if (pos >= 0 && pos < it->len) {
        return &it->lines[pos];
    } else {
        assert (false);
        return nullptr;
    }
}

aoc_input_line *aoc_input_access_alloc (aoc_input const *it, size_t pos) {
    if (pos >= 0 && pos < it->cap) {
        return &it->lines[pos];
    } else {
        assert (false);
        return nullptr;
    }
}

bool aoc_input_empty (aoc_input* it) {
    return aoc_input_len (it) == 0;
}

void aoc_input_append (aoc_input *it, char const *line) {
    aoc_input_reserve (it, it->len + 1);

    aoc_input_line* dest = aoc_input_access_alloc (it, it->len);
    aoc_input_line_copy (dest, line);
    it->len++;
}

void aoc_input_reserve (aoc_input *it, size_t new_cap) {
    assert (it);
    assert (it->len <= it->cap);
    assert ((it->cap == 0) == (it->lines == nullptr));

    /* round up to next power of two */
    new_cap = stdc_bit_ceil (new_cap);
    if (it->cap < new_cap) {
        AOC_REALLOCARRAY(it->lines, new_cap);
        it->cap = new_cap;
    }
}

void print_elapsed_time(const char *s, const struct timespec *t0, const struct timespec *t1) {
    printf ("%s: %.6fs\n", s, ((t1->tv_sec - t0->tv_sec) * 1000000000L + t1->tv_nsec - t0->tv_nsec) / 1e9);
}

int main (int argc, char* argv[]) {
    struct timespec s0, s1;

    clock_gettime (CLOCK_MONOTONIC, &s0);
    if (argc != 2)  {
        fprintf (stderr, "Usage: %s <input_file>\n", argv[0]);
        exit (EXIT_FAILURE);
    }

    struct timespec t0, t1;
    clock_gettime (CLOCK_MONOTONIC, &t0);
    clock_gettime (CLOCK_MONOTONIC, &t1);
    print_elapsed_time ("read input file", &t0, &t1);

    clock_gettime (CLOCK_MONOTONIC, &t0);
    clock_gettime (CLOCK_MONOTONIC, &t1);
    print_elapsed_time ("compute", &t0, &t1);

    uint64_t total = 0;
    printf ("result: %lu\n", total);
    clock_gettime (CLOCK_MONOTONIC, &s1);
    print_elapsed_time ("total program execution time", &s0, &s1);

    return EXIT_SUCCESS;
}
