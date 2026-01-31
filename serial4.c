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

static double wilson_lower_bound(int positive, int total, double z) {
    if (total == 0) return 0.0;
    double phat = (double)positive / total;
    double denom = 1.0 + (z*z)/total;
    double num = phat + (z*z)/(2.0*total)
               - z * sqrt((phat*(1.0-phat) + (z*z)/(4.0*total)) / total);
    return num / denom;
}

static int findProduct(struct Product *products, int count, const char *asin) {
    for (int i = 0; i < count; i++) {
        if (strcmp(products[i].asin, asin) == 0)
            return i;
    }
    return -1;
}

static int extract_asin_overall(const char *line, char *asin_out, size_t asin_sz, double *overall_out) {
    const char *k = strstr(line, "\"asin\"");
    if (!k) return 0;
    k = strchr(k, ':'); if (!k) return 0;
    k++;
    while (*k==' ' || *k=='\t') k++;
    if (*k != '\"') return 0;
    k++;

    size_t j = 0;
    while (*k && *k != '\"' && j < asin_sz - 1)
        asin_out[j++] = *k++;
    asin_out[j] = '\0';
    if (j == 0) return 0;

    const char *p = strstr(line, "\"overall\"");
    if (!p) return 0;
    p = strchr(p, ':'); if (!p) return 0;
    p++;
    while (*p==' ' || *p=='\t') p++;

    char buf[64]; j = 0;
    while (*p && j < sizeof(buf)-1) {
        if ((*p>='0' && *p<='9') || *p=='.' || *p=='-')
            buf[j++] = *p++;
        else if (*p==',' || *p==' ' || *p=='}' || *p=='\n')
            break;
        else p++;
    }
    buf[j] = '\0';
    if (j == 0) return 0;

    *overall_out = atof(buf);
    return 1;
}

int main(void) {

    const char *input_path  = "reviews_Cell_Phones_and_Accessories.json";
    const char *output_path = "cqs_output_serial4.txt";

    FILE *file = fopen(input_path, "r");
    if (!file) {
        fprintf(stderr, "Error opening input file.\n");
        return 1;
    }

    size_t lines_cap = 1 << 14;
    size_t lines_cnt = 0;
    char **lines = malloc(lines_cap * sizeof(char*));

    char buf[LINE_SIZE];

    while (fgets(buf, sizeof(buf), file)) {
        size_t L = strlen(buf);
        char *s = malloc(L + 1);
        memcpy(s, buf, L + 1);

        if (lines_cnt == lines_cap) {
            size_t newcap = lines_cap * 2;
            char **tmp = realloc(lines, newcap * sizeof(char*));
            lines = tmp;
            lines_cap = newcap;
        }

        lines[lines_cnt++] = s;
    }
    fclose(file);

    int capacity = 4096, productCount = 0;
    struct Product *products = malloc(sizeof(struct Product) * capacity);

    double t_compute_start = omp_get_wtime();

    for (size_t i = 0; i < lines_cnt; i++) {
        char asin[30];
        double overall;

        if (!extract_asin_overall(lines[i], asin, sizeof(asin), &overall))
            continue;

        int idx = findProduct(products, productCount, asin);

        if (idx == -1) {
            if (productCount == capacity) {
                int newcap = capacity * 2;
                struct Product *tmp = realloc(products, sizeof(struct Product) * newcap);
                products = tmp;
                capacity = newcap;
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

    const double z = 1.96;
    for (int i = 0; i < productCount; i++) {
        products[i].cqs = wilson_lower_bound(
            products[i].positive_reviews,
            products[i].total_reviews,
            z
        );
    }

    double t_compute_end = omp_get_wtime();

    FILE *out = fopen(output_path, "w");
    fprintf(out, "%-15s %-10s %-10s %-10s\n", "ASIN", "Total", "Positive", "CQS");
     fprintf(out, "-------------------------------------------------------------\n");
    for (int i = 0; i < productCount; i++) {
        fprintf(out, "%-15s %-10d %-10d %-10.4f\n",
                products[i].asin,
                products[i].total_reviews,
                products[i].positive_reviews,
                products[i].cqs);
    }
    fclose(out);

    printf("Compute time: %.6f s\n", t_compute_end - t_compute_start);

    for (size_t i = 0; i < lines_cnt; i++) free(lines[i]);
    free(lines);
    free(products);

    return 0;
}
