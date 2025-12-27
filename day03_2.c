#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define MAX_RANK 13
#define MAX_OFFSET 121

typedef struct file_buf file_buf;
struct file_buf {
    off_t size;
    char* buf_p;
    bool ok;
};

static const uint64_t pow_10_array[] = {
    1, 10, 100, 1000, 10000, 100000, 1000000, 10000000,
    100000000, 1000000000, 10000000000, 100000000000, 1000000000000
};

uint64_t battery_joltage (const char *joltages, const size_t len) {
    uint64_t joltage_mem[2][MAX_OFFSET] = { 0 };

    for (size_t r = 1; r < MAX_RANK; r++) {
        size_t curr = r & 1;
        size_t prev = (r - 1) & 1;
        for (int o = len - r; o >= 0; o--) {
            const uint64_t take = (joltages[o] - '0') * pow_10_array[r - 1] + joltage_mem[prev][o + 1];
            const uint64_t skip = joltage_mem[curr][o + 1];

            joltage_mem[curr][o] = take > skip ? take : skip;
        }
    }

    return joltage_mem[(MAX_RANK - 1) & 1][0];
}

void print_elapsed_time(const char *s, const struct timespec *t0, const struct timespec *t1) {
    printf ("%s: %.6fs\n", s, ((t1->tv_sec - t0->tv_sec) * 1000000000L + t1->tv_nsec - t0->tv_nsec) / 1e9);
}

file_buf read_input_file (const char* filename) {
    struct file_buf fb;
    struct stat st;
    int fd;
    char* buf = nullptr;
    int ret;

    fb.ok = false;

    fd = open (filename, O_RDONLY);
    if (fd == -1) {
        perror ("open");
        goto out_close;
    }

    ret = fstat (fd, &st);
    if (ret == -1) {
        perror ("fstat");
        goto out_close;
    }

    buf = mmap (NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buf == MAP_FAILED) {
        perror ("mmap");
        goto out_close;
    }

    fb.size = st.st_size;
    fb.buf_p = buf;
    fb.ok = true;

out_close:
    close (fd);
    return fb;
}

uint64_t compute (const file_buf *fb) {
    char* buf = fb->buf_p;
    char* line_start = buf;
    uint64_t total = 0;

    for (size_t i = 0; i < fb->size; i++) {
        if (buf[i] == '\n') {
            size_t len = &buf[i] - line_start;
            total += battery_joltage (line_start, len);
            line_start = &buf[i] + 1;
        }
    }

    return total;
}

int main (int argc, char* argv[]) {
    struct timespec s0, s1;
    struct file_buf fb;

    clock_gettime (CLOCK_MONOTONIC, &s0);
    if (argc != 2)  {
        fprintf (stderr, "Usage: %s <input_file>\n", argv[0]);
        exit (EXIT_FAILURE);
    }

    struct timespec t0, t1;
    clock_gettime (CLOCK_MONOTONIC, &t0);

    fb = read_input_file (argv[1]);
    if (! fb.ok) {
        fprintf (stderr, "Failed to read file input\n");
        exit (EXIT_FAILURE);
    }
    clock_gettime (CLOCK_MONOTONIC, &t1);
    print_elapsed_time ("read input file", &t0, &t1);

    clock_gettime (CLOCK_MONOTONIC, &t0);
    uint64_t total = compute (&fb);
    if (total == -1) {
        fprintf (stderr, "Failure in compute");
        exit (EXIT_FAILURE);
    }
    clock_gettime (CLOCK_MONOTONIC, &t1);
    print_elapsed_time ("compute", &t0, &t1);

    printf ("result: %lu\n", total);
    clock_gettime (CLOCK_MONOTONIC, &s1);
    print_elapsed_time ("total program execution time", &s0, &s1);

    return EXIT_SUCCESS;
}
