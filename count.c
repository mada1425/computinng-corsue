#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 65536

int main(void) {
    FILE *file = fopen("cqs_output_MPI_simple", "r");
    if (!file) {
        printf("Error: cannot open file!\n");
        return 1;
    }

    char line[LINE_SIZE];
    int productCount = 0;

    char asin[30];
    int total, positive;
    double cqs;

    while (fgets(line, sizeof(line), file)) {
        // تجاهل سطر الهيدر وسطر الشرطات وأي سطر فاضي
        if (strncmp(line, "ASIN", 4) == 0) continue;
        if (line[0] == '-' || line[0] == '\n') continue;

        /* نحاول نقرأ:
           ASIN   Total   Positive   CQS
           مثال: 011040047X  1  0  0.0000
        */
        if (sscanf(line, "%29s %d %d %lf", asin, &total, &positive, &cqs) == 4) {
            productCount++;
        }
    }

    fclose(file);

    printf("Products counted in file: %d\n", productCount);

    return 0;
}
