#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <omp.h>

#define LINE_SIZE 65536

struct Product {
    char asin[30];
    int total_reviews;
    int positive_reviews;
    double cqs;
};

double wilson_lower_bound(int positive, int total, double z) {
    if (total == 0) return 0.0;
    double phat = (double)positive / total;
    double denom = 1.0 + (z*z)/total;
    double num = phat + (z*z)/(2.0*total) - z * sqrt((phat*(1.0 - phat) + (z*z)/(4.0*total)) / total);
    return num / denom;
}

int findProduct(struct Product *products, int count, const char *asin) {
    for (int i = 0; i < count; i++) {
        if (strcmp(products[i].asin, asin) == 0) return i;
    }
    return -1;
}

int extract_asin_overall(const char *line, char *asin_out, size_t asin_sz, double *overall_out) {
    const char *k = strstr(line, "\"asin\"");
    if (!k) return 0;
    k = strchr(k, ':'); if (!k) return 0;
    k++;
    while (*k==' ' || *k=='\t') k++;
    if (*k != '\"') return 0;
    k++;
    size_t j = 0;
    while (*k && *k != '\"' && j < asin_sz - 1) { asin_out[j++] = *k++; }
    asin_out[j] = '\0';
    if (j == 0) return 0;

    const char *p = strstr(line, "\"overall\"");
    if (!p) return 0;
    p = strchr(p, ':'); if (!p) return 0;
    p++;
    while (*p==' ' || *p=='\t') p++;
    char buf[64]; j = 0;
    while (*p && j < sizeof(buf)-1) {
        if ((*p >= '0' && *p <= '9') || *p == '.' || *p == '-') {
            buf[j++] = *p++;
        } else if (*p == ',' || *p == ' ' || *p == '}' || *p == '\n') break;
        else p++;
    }
    buf[j] = '\0';
    if (j == 0) return 0;

    *overall_out = atof(buf);
    return 1;
}

int main(void) {

    const char *input_path = "reviews_Cell_Phones_and_Accessories.json";
    const char *output_path = "cqs_output_S.txt";

    // زمن البداية (قبل أي عمل)
    double t0 = omp_get_wtime();

    // --- القراءة + التجميع ---
    FILE *file = fopen(input_path, "r");
    if (!file) { printf("Error: cannot open input file!\n"); return 1; }

    int capacity = 1000;
    int productCount = 0;
    struct Product *products = (struct Product*)malloc(sizeof(struct Product) * capacity);
    if (!products) { printf("Error: malloc failed!\n"); fclose(file); return 1; }

    char line[LINE_SIZE];

    while (fgets(line, sizeof(line), file)) {
        char asin[30];
        double overall = 0.0;

        if (!extract_asin_overall(line, asin, sizeof(asin), &overall))
            continue;

        int idx = findProduct(products, productCount, asin);
        if (idx == -1) {
            if (productCount == capacity) {
                capacity *= 2;
                struct Product *tmp = (struct Product*)realloc(products, sizeof(struct Product) * capacity);
                if (!tmp) { printf("Error: realloc failed!\n"); free(products); fclose(file); return 1; }
                products = tmp;
            }
            strncpy(products[productCount].asin, asin, sizeof(products[productCount].asin)-1);
            products[productCount].asin[sizeof(products[productCount].asin)-1] = '\0';
            products[productCount].total_reviews = 1;
            products[productCount].positive_reviews = (overall >= 4.0) ? 1 : 0;
            products[productCount].cqs = 0.0;
            productCount++;
        } else {
            products[idx].total_reviews++;
            if (overall >= 4.0) products[idx].positive_reviews++;
        }
    }
    fclose(file);

    double t1 = omp_get_wtime(); // نهاية القراءة

    printf("Products counted: %d\n", productCount);
    printf("Read time:      %.6f s\n", t1 - t0);

    // --- الحساب (CQS) ---
    double z = 1.96;
    for (int i = 0; i < productCount; i++) {
        products[i].cqs = wilson_lower_bound(products[i].positive_reviews, products[i].total_reviews, z);
    }
    double t2 = omp_get_wtime(); // نهاية الحساب
    printf("Compute time:   %.6f s\n", t2 - t1);

    // --- الكتابة ---
    FILE *out = fopen(output_path, "w");
    if (!out) { printf("Error: cannot open output file!\n"); free(products); return 1; }

    fprintf(out, "%-15s %-15s %-15s %-15s\n", "ASIN", "Total", "Positive", "CQS");
    fprintf(out, "-------------------------------------------------------------\n");
    for (int i = 0; i < productCount; i++) {
        fprintf(out, "%-15s %-15d %-15d %-15.4f\n",
                products[i].asin,
                products[i].total_reviews,
                products[i].positive_reviews,
                products[i].cqs);
    }
    fclose(out);

    double t3 = omp_get_wtime(); // نهاية الكتابة
    printf("Write time:     %.6f s\n", t3 - t2);

    // --- الوقت الكلي ---
    printf("Total time:     %.6f s\n", t3 - t0);
    printf("Timer tick:     %.9f s\n", omp_get_wtick());

    free(products);
    return 0;
}
