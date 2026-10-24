#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

#define MAX_DP_CELLS 5000000U
#define LABEL_WIDTH 24
#define CELL_WIDTH 6

typedef struct {
    int n;
    int capacity;
    int *weights;
    int *values;
    int item_count;
    int has_header;
} TestCase;

static void free_test_cases(TestCase *tests, size_t count) {
    size_t i;
    for (i = 0; i < count; ++i) {
        free(tests[i].weights);
        free(tests[i].values);
    }
    free(tests);
}

static int is_blank(const char *s) {
    while (*s != '\0') {
        if (!isspace((unsigned char)*s)) return 0;
        ++s;
    }
    return 1;
}

static int is_test_marker(const char *line) {
    while (isspace((unsigned char)*line)) ++line;
    return strncmp(line, "#Test", 5) == 0;
}

/* Also accepts an optional extraction prefix such as [L89] before a pair. */
static int parse_integer_pair(const char *line, int *first, int *second) {
    char *end;
    long a, b;
    const char *p = line;

    while (isspace((unsigned char)*p)) ++p;
    if (*p == '[') {
        const char *close = strchr(p, ']');
        if (close != NULL) {
            p = close + 1;
            while (isspace((unsigned char)*p)) ++p;
        }
    }

    a = strtol(p, &end, 10);
    if (end == p || a < INT_MIN || a > INT_MAX) return 0;
    p = end;
    while (isspace((unsigned char)*p)) ++p;

    b = strtol(p, &end, 10);
    if (end == p || b < INT_MIN || b > INT_MAX) return 0;
    p = end;
    while (isspace((unsigned char)*p)) ++p;

    if (*p != '\0' && *p != '\n' && *p != '\r' && *p != '#') return 0;

    *first = (int)a;
    *second = (int)b;
    return 1;
}

static int append_test_case(TestCase **tests, size_t *count, size_t *allocated) {
    TestCase *grown;
    size_t new_size;

    if (*count == *allocated) {
        new_size = (*allocated == 0) ? 4 : (*allocated * 2);
        if (new_size < *allocated || new_size > SIZE_MAX / sizeof(TestCase)) {
            return 0;
        }
        grown = (TestCase *)realloc(*tests, new_size * sizeof(TestCase));
        if (grown == NULL) return 0;
        *tests = grown;
        *allocated = new_size;
    }

    memset(&(*tests)[*count], 0, sizeof(TestCase));
    ++(*count);
    return 1;
}

static int read_test_cases(FILE *input, TestCase **out_tests, size_t *out_count) {
    char line[4096];
    TestCase *tests = NULL;
    size_t count = 0, allocated = 0;
    long line_number = 0;
    int active = -1;
    int first, second;

    while (fgets(line, sizeof(line), input) != NULL) {
        ++line_number;

        if (is_test_marker(line)) {
            if (active >= 0) {
                TestCase *previous = &tests[active];
                if (!previous->has_header || previous->item_count != previous->n) {
                    fprintf(stderr,
                            "Input error before line %ld: a #Test block has incomplete item data.\n",
                            line_number);
                    free_test_cases(tests, count);
                    return 0;
                }
            }
            if (!append_test_case(&tests, &count, &allocated)) {
                fprintf(stderr, "Memory allocation failed while reading test cases.\n");
                free_test_cases(tests, count);
                return 0;
            }
            active = (int)(count - 1);
            continue;
        }

        if (is_blank(line)) continue;
        if (active < 0) continue;
        if (!parse_integer_pair(line, &first, &second)) continue;

        {
            TestCase *current = &tests[active];
            if (!current->has_header) {
                size_t n;
                if (first <= 0 || second < 0) {
                    fprintf(stderr,
                            "Invalid n or capacity at input line %ld. Expected: n W, with n > 0 and W >= 0.\n",
                            line_number);
                    free_test_cases(tests, count);
                    return 0;
                }
                current->n = first;
                current->capacity = second;
                n = (size_t)current->n;
                if (n > SIZE_MAX / sizeof(int)) {
                    fprintf(stderr, "Too many items in test case %d.\n", active + 1);
                    free_test_cases(tests, count);
                    return 0;
                }
                current->weights = (int *)malloc(n * sizeof(int));
                current->values = (int *)malloc(n * sizeof(int));
                if (current->weights == NULL || current->values == NULL) {
                    fprintf(stderr, "Memory allocation failed for test case %d.\n", active + 1);
                    free_test_cases(tests, count);
                    return 0;
                }
                current->has_header = 1;
            } else if (current->item_count < current->n) {
                if (first < 0 || second < 0) {
                    fprintf(stderr,
                            "Negative weight/value at input line %ld. Weights and values must be non-negative.\n",
                            line_number);
                    free_test_cases(tests, count);
                    return 0;
                }
                current->weights[current->item_count] = first;
                current->values[current->item_count] = second;
                ++current->item_count;
            } else {
                fprintf(stderr,
                        "Unexpected numeric pair at input line %ld: each test case must contain exactly n item lines.\n",
                        line_number);
                free_test_cases(tests, count);
                return 0;
            }
        }
    }

    if (ferror(input)) {
        fprintf(stderr, "An error occurred while reading the input.\n");
        free_test_cases(tests, count);
        return 0;
    }

    if (count == 0) {
        fprintf(stderr, "No test cases found. Add test cases beginning with #Test.\n");
        free_test_cases(tests, count);
        return 0;
    }

    {
        size_t i;
        for (i = 0; i < count; ++i) {
            if (!tests[i].has_header || tests[i].item_count != tests[i].n) {
                fprintf(stderr,
                        "Test case %zu is incomplete: expected %d items, found %d.\n",
                        i + 1, tests[i].n, tests[i].item_count);
                free_test_cases(tests, count);
                return 0;
            }
        }
    }

    *out_tests = tests;
    *out_count = count;
    return 1;
}

static void print_dp_table(const long long *dp, int items_done, int n, int capacity,
                           const int *weights, const int *values) {
    int row, cap;
    size_t cols = (size_t)capacity + 1;

    printf("\nCapacity ->");
    for (cap = 0; cap <= capacity; ++cap) printf("%*d", CELL_WIDTH, cap);
    printf("\n");

    for (row = 0; row <= items_done; ++row) {
        char label[80];
        if (row == 0) {
            snprintf(label, sizeof(label), "0 items");
        } else {
            snprintf(label, sizeof(label), "Item %d (w=%d,v=%d)",
                     row, weights[row - 1], values[row - 1]);
        }
        printf("%-*s", LABEL_WIDTH, label);
        for (cap = 0; cap <= capacity; ++cap) {
            printf("%*lld", CELL_WIDTH, dp[(size_t)row * cols + (size_t)cap]);
        }
        printf("\n");
    }
    (void)n;
}

static int solve_and_visualize(const TestCase *test, int test_number,
                               FILE *results_file, long long *answer_out) {
    size_t rows = (size_t)test->n + 1;
    size_t cols = (size_t)test->capacity + 1;
    size_t cells;
    long long *dp;
    int i, cap;

    if (rows > SIZE_MAX / cols) {
        fprintf(stderr, "DP table size overflow in test case %d.\n", test_number);
        return 0;
    }
    cells = rows * cols;
    if (cells > MAX_DP_CELLS || cells > SIZE_MAX / sizeof(long long)) {
        fprintf(stderr,
                "Test case %d needs a DP table that is too large (%zu cells; limit %u).\n",
                test_number, cells, MAX_DP_CELLS);
        return 0;
    }

    dp = (long long *)calloc(cells, sizeof(long long));
    if (dp == NULL) {
        fprintf(stderr, "Memory allocation failed for the DP table in test case %d.\n", test_number);
        return 0;
    }

    printf("\n============================================================\n");
    printf("#Test %d | n = %d | W = %d\n", test_number, test->n, test->capacity);
    printf("Building the 0/1 Knapsack DP table item by item...\n");
    printf("============================================================\n");

    for (i = 1; i <= test->n; ++i) {
        int item_weight = test->weights[i - 1];
        long long item_value = test->values[i - 1];

        for (cap = 0; cap <= test->capacity; ++cap) {
            long long best = dp[(size_t)(i - 1) * cols + (size_t)cap];
            if (item_weight <= cap) {
                long long take = item_value +
                    dp[(size_t)(i - 1) * cols + (size_t)(cap - item_weight)];
                if (take > best) best = take;
            }
            dp[(size_t)i * cols + (size_t)cap] = best;
        }

        printf("\nSTEP %d/%d: Added item (weight = %d, value = %lld)\n",
               i, test->n, item_weight, item_value);
        print_dp_table(dp, i, test->n, test->capacity, test->weights, test->values);
        fflush(stdout);
    }

    *answer_out = dp[(size_t)test->n * cols + (size_t)test->capacity];
    printf("\nMaximum total value for #Test %d = %lld\n", test_number, *answer_out);

    fprintf(results_file, "#Test %d | Input (n W): %d %d | Result: %lld\n",
            test_number, test->n, test->capacity, *answer_out);
    fflush(results_file);

    free(dp);
    return 1;
}

int main(int argc, char *argv[]) {
    FILE *input = NULL;
    FILE *results_file;
    TestCase *tests = NULL;
    size_t test_count = 0, i;
    int exit_code = EXIT_FAILURE;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [input_file]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            perror("Could not open input file");
            return EXIT_FAILURE;
        }
    } else {
        input = fopen("input.txt", "r");
        if (input == NULL) input = stdin;
    }

    if (!read_test_cases(input, &tests, &test_count)) {
        if (input != stdin) fclose(input);
        return EXIT_FAILURE;
    }
    if (input != stdin) fclose(input);

    results_file = fopen("result.txt", "w");
    if (results_file == NULL) {
        perror("Could not create result.txt");
        free_test_cases(tests, test_count);
        return EXIT_FAILURE;
    }

    fprintf(results_file, "0/1 Knapsack Dynamic Programming Results\n\n");
    printf("Loaded %zu test case(s). Results will be saved to result.txt.\n", test_count);

    for (i = 0; i < test_count; ++i) {
        long long answer;
        if (!solve_and_visualize(&tests[i], (int)i + 1, results_file, &answer)) {
            goto cleanup;
        }
    }

    printf("\n============================================================\n");
    printf("All test cases completed. Results saved in result.txt.\n");
    printf("============================================================\n");
    exit_code = EXIT_SUCCESS;

cleanup:
    fclose(results_file);
    free_test_cases(tests, test_count);
    return exit_code;
}
