#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
// I got help from ChatGPT to fix syntax issues in this code.
// I used it to more understand 
#define NUM_RESOURCES 3

int main() {
    const char *RES[NUM_RESOURCES] = {"/index.html", "/about.html", "/video/abc.mp4"};
    int counts[NUM_RESOURCES] = {0};

    FILE *f = fopen("large_web_log.txt", "r");
    if (!f) { printf("error opening file\n"); return 1; }

    int n = 0, uid, st;
    char ts[64], res[256];
    while (fscanf(f, "%63s %d %255s %d", ts, &uid, res, &st) == 4) n++;
    if (n == 0) { printf("empty\n"); fclose(f); return 0; }

    rewind(f);
    int *values = (int*)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++) {
        fscanf(f, "%63s %d %255s %d", ts, &uid, res, &st);
        int k = -1;
        if      (strcmp(res, RES[0]) == 0) k = 0;
        else if (strcmp(res, RES[1]) == 0) k = 1;
        else if (strcmp(res, RES[2]) == 0) k = 2;
        values[i] = k; 
    }
    fclose(f);

    for (int i = 0; i < NUM_RESOURCES; i++) counts[i] = 0;
    double start_time = omp_get_wtime();

   #pragma omp parallel for
for (int i = 0; i < n; i++) {
    int bin_index = values[i];
    if (bin_index >= 0) {
        #pragma omp critical
        {
            counts[bin_index]++;
        }
    }
}

    double end_time = omp_get_wtime();
    printf("Atomic Time (for histogram): %.8f seconds\n", end_time - start_time);

    for (int b = 0; b < NUM_RESOURCES; b++)
        printf("%s : %d\n", RES[b], counts[b]);

    free(values);
    return 0;
}
