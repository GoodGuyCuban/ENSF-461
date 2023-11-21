#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define min(a,b) ((a) < (b) ? (a) : (b))

// Parse a vector of integers from a file, one per line.
// Return the number of integers parsed.
int parse_ints(FILE *file, int **ints) {
    int i;
    int size = 0;
    int capacity = 100; // Initial capacity
    int *results = malloc(sizeof(int) * capacity);
    if (results == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(1);
    }
    while (fscanf(file, "%d", &i) != EOF) {
        if (size == capacity) { // If the array is full, double its size
            capacity *= 2;
            results = realloc(results, sizeof(int) * capacity);
            if (results == NULL) {
                fprintf(stderr, "Failed to allocate memory\n");
                exit(1);
            }
        }
        results[size] = i;
        size++;
    }
    *ints = results;
    return size;
}


// Write a vector of integers to a file, one per line.
void write_ints(FILE *file, int *ints, int size) {
    int i;
    for (i = 0; i < size; i++) {
        fprintf(file, "%d\n", ints[i]);
    }
}


// Return the result of a sequential prefix scan of the given vector of integers.
int* SEQ(int *ints, int size) {
    int *results = malloc(size * sizeof(int));
    int i;
    int sum = 0;
    for (i = 0; i < size; i++) {
        sum += ints[i];
        results[i] = sum;
    }
    return results;
}


// Return the result of Hillis/Steele, but with each pass executed sequentially
int* HSS(int *ints, int size) {
    /*
    • Then, implement a serial version of Hillis/Steele (all the passes, but
    executed one after another rather than in parallel)
    • You may assume that the input vector’s length is a power of 2
    */
    int *results = malloc(size * sizeof(int));
    int *temp = malloc(size * sizeof(int));
    if (results == NULL || temp == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(1);
    }
    memcpy(results, ints, size * sizeof(int)); // Copy the original array to results

    int pass;
    int i;
    for (pass = 0; (1 << pass) < size; pass++) {
        memcpy(temp, results, size * sizeof(int)); // Copy the results array to temp
        for (i = 0; i < size; i++) {
            if (i >= (1 << pass)) {
                results[i] = temp[i] + temp[i - (1 << pass)];
            }
        }
    }
    free(temp);
    return results;
}

typedef struct {
    int *ints;
    int *results;
    int *temp;
    int start;
    int end;
    int pass;
} ThreadData;

void* HSP_helper(void *arg) {
    ThreadData *data = (ThreadData*)arg;
    int i;
    for (i = data->start; i < data->end; i++) {
        if (i >= (1 << data->pass)) {
            data->results[i] = data->temp[i] + data->temp[i - (1 << data->pass)];
        }
    }
    return NULL;
}

// Return the result of Hillis/Steele, parallelized using pthread
int* HSP(int *ints, int size, int numthreads) {
    int *results = malloc(size * sizeof(int));
    int *temp = malloc(size * sizeof(int));
    if (results == NULL || temp == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(1);
    }
    memcpy(results, ints, size * sizeof(int));

    pthread_t *threads = malloc(numthreads * sizeof(pthread_t));
    ThreadData *threadData = malloc(numthreads * sizeof(ThreadData));

    int pass;
    for (pass = 0; (1 << pass) < size; pass++) {
        memcpy(temp, results, size * sizeof(int));

        int effectiveThreads = min(1 << pass, numthreads);
        int i;
        for (i = 0; i < effectiveThreads; i++) {
            threadData[i].ints = ints;
            threadData[i].results = results;
            threadData[i].temp = temp;
            threadData[i].start = i * (size / effectiveThreads);
            threadData[i].end = (i == effectiveThreads - 1) ? size : (i + 1) * (size / effectiveThreads);
            threadData[i].pass = pass;
            pthread_create(&threads[i], NULL, HSP_helper, &threadData[i]);
        }

        for (i = 0; i < effectiveThreads; i++) {
            pthread_join(threads[i], NULL);
        }
    }

    free(temp);
    free(threads);
    free(threadData);

    return results;
}



int main(int argc, char** argv) {
    
    if ( argc != 5 ) {
        printf("Usage: %s <mode> <#threads> <input file> <output file>\n", argv[0]);
        return 1;
    }
    

    
    char *mode = argv[1];
    int num_threads = atoi(argv[2]);
    FILE *input = fopen(argv[3], "r");
    FILE *output = fopen(argv[4], "w");
    

    //manual  testing
    
    /*
    char *mode = "HSP";
    int num_threads = 2;
    FILE *input = fopen("tests/test2.in", "r");
    FILE *output = fopen("test2.txt", "w");
    */
    

    int *ints;
    int size;
    size = parse_ints(input, &ints);

    int *result;
    if (strcmp(mode, "SEQ") == 0) {
        result = SEQ(ints, size);
    } else if (strcmp(mode, "HSS") == 0) {
        result = HSS(ints, size);
    } else if (strcmp(mode, "HSP") == 0) {
        result = HSP(ints, size, num_threads);
    } else {
        printf("Unknown mode: %s\n", mode);
        return 1;
    }

    write_ints(output, result, size);
    fclose(input);
    fclose(output);
    return 0;
}
