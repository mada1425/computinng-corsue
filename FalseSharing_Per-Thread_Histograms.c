// I got help from ChatGPT to fix syntax issues in this code.
// I used it to more understand #include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define NUM_RESOURCES 3  

int main() {
    const char *RES[NUM_RESOURCES] = {"/index.html", "/about.html", "/video/abc.mp4"};

    FILE *f = fopen("large_web_log.txt", "r");
    if (!f) { printf("error opening file\n"); return 1; }

    int n = 0, uid, st; char ts[64], res[256];
    while (fscanf(f, "%63s %d %255s %d", ts, &uid, res, &st) == 4) n++;
    if (n == 0) { printf("[unpadded] empty or format mismatch\n"); fclose(f); return 0; }

    rewind(f);
    int *idx = (int*)malloc((size_t)n * sizeof(int));
    if (!idx) { printf("malloc failed\n"); fclose(f); return 1; }

     for (int i = 0; i < n; i++) {
        fscanf(f, "%63s %d %255s %d", ts, &uid, res, &st);
        int k = -1;
        if      (strcmp(res, RES[0]) == 0) k = 0;
        else if (strcmp(res, RES[1]) == 0) k = 1;
        else if (strcmp(res, RES[2]) == 0) k = 2;
 idx[i] = k;    }

    fclose(f);

    int T = 0;
    #pragma omp parallel
    {
        #pragma omp single
        T = omp_get_num_threads();
    }

    int *counts = (int*)calloc((size_t)T * NUM_RESOURCES, sizeof(int));
    if (!counts) { printf("calloc failed\n"); free(idx); return 1; }

    double t0 = omp_get_wtime();

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int base = tid * NUM_RESOURCES;

        #pragma omp for 
        for (int i = 0; i < n; i++) {
            int k = idx[i];
            if (k >= 0) counts[base + k] += 1;          }
    }

    int total[NUM_RESOURCES] = {0};
    for (int t = 0; t < T; t++) {
        int base = t * NUM_RESOURCES;
        for (int k = 0; k < NUM_RESOURCES; k++)
            total[k] += counts[base + k];
    }

    double t1 = omp_get_wtime();

    printf("[Per-Thread UNPADDED] threads=%d  time=%.8f s\n", T, t1 - t0);
    for (int k = 0; k < NUM_RESOURCES; k++)
        printf("%s : %d\n", RES[k], total[k]);

    free(counts);
    free(idx);
    return 0;
}
