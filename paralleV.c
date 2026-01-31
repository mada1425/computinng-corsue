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
    double num = phat + (z*z)/(2.0*total) - z * sqrt((phat*(1.0 - phat) + (z*z)/(4.0*total)) / total);
    return num / denom;
}

static int findProduct(struct Product *products, int count, const char *asin) {
    for (int i = 0; i < count; i++) {
        if (strcmp(products[i].asin, asin) == 0) return i;
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
    const char *input_path  = "reviews_Cell_Phones_and_Accessories.json";
    const char *output_path = "cqs_output_P1.txt";

    double t0 = omp_get_wtime();  // بداية الوقت الكلي

    // 1) قراءة الملف وتخزين الأسطر
    FILE *file = fopen(input_path, "r");
    if (!file) { printf("Error: cannot open input file!\n"); return 1; }

    size_t lines_cap = 1<<14; // 16384
    size_t lines_cnt = 0;
    char **lines = (char**)malloc(lines_cap * sizeof(char*));
    if (!lines) { printf("malloc lines failed\n"); fclose(file); return 1; }

    char buf[LINE_SIZE];
    while (fgets(buf, sizeof(buf), file)) {
        size_t L = strlen(buf);
        char *s = (char*)malloc(L + 1);
        if (!s) { printf("malloc line failed\n"); fclose(file); return 1; }
        memcpy(s, buf, L + 1);
        if (lines_cnt == lines_cap) {
            lines_cap *= 2;
            char **tmp = (char**)realloc(lines, lines_cap * sizeof(char*));
            if (!tmp) { printf("realloc lines failed\n"); fclose(file); return 1; }
            lines = tmp;
        }
        lines[lines_cnt++] = s;
    }
    fclose(file);

    double t1 = omp_get_wtime();  // نهاية القراءة

    // 2) تجميع موازي: قوائم محلية لكل خيط
    int max_threads = omp_get_max_threads();
    struct Product **local_lists = (struct Product**)calloc(max_threads, sizeof(struct Product*));
    int *local_counts = (int*)calloc(max_threads, sizeof(int));
    int *local_caps   = (int*)calloc(max_threads, sizeof(int));
    if (!local_lists || !local_counts || !local_caps) {
        printf("calloc locals failed\n");
        return 1;
    }

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int cap = 2048;
        struct Product *list = (struct Product*)malloc(sizeof(struct Product)*cap);
        int count = 0;
        int oom = 0; // فلاج نفاد ذاكرة محلي

        if (!list) {
            printf("thread %d: malloc failed\n", tid);
        } else {
            #pragma omp for schedule(static)
            for (long long i = 0; i < (long long)lines_cnt; i++) {
                if (oom) continue; // لو حصل OOM، تخطَّ الأسطر التالية

                char asin[30];
                double overall = 0.0;
                if (!extract_asin_overall(lines[i], asin, sizeof(asin), &overall)) continue;

                // ابحث/حدّث محليًا
                int idx = -1;
                for (int k = 0; k < count; k++) {
                    if (strcmp(list[k].asin, asin) == 0) { idx = k; break; }
                }

                if (idx == -1) {
                    if (count == cap) {
                        cap *= 2;
                        struct Product *tmp = (struct Product*)realloc(list, sizeof(struct Product)*cap);
                        if (!tmp) {
                            printf("Thread %d: realloc failed, skipping further inserts.\n", tid);
                            oom = 1;
                            continue;
                        }
                        list = tmp;
                    }
                    strncpy(list[count].asin, asin, sizeof(list[count].asin)-1);
                    list[count].asin[sizeof(list[count].asin)-1] = '\0';
                    list[count].total_reviews    = 1;
                    list[count].positive_reviews = (overall >= 4.0) ? 1 : 0;
                    list[count].cqs = 0.0;
                    count++;
                } else {
                    list[idx].total_reviews++;
                    if (overall >= 4.0) list[idx].positive_reviews++;
                }
            }
        }

        local_lists[tid]  = list;
        local_counts[tid] = count;
        local_caps[tid]   = cap;
    }

    // 3) دمج تسلسلي
    int capacity = 4096, productCount = 0;
    struct Product *products = (struct Product*)malloc(sizeof(struct Product)*capacity);
    if (!products) { printf("malloc products failed\n"); return 1; }

    for (int t = 0; t < max_threads; t++) {
        struct Product *lst = local_lists[t];
        int cnt = local_counts[t];
        for (int i = 0; i < cnt; i++) {
            int idx = findProduct(products, productCount, lst[i].asin);
            if (idx == -1) {
                if (productCount == capacity) {
                    capacity *= 2;
                    struct Product *tmp = (struct Product*)realloc(products, sizeof(struct Product)*capacity);
                    if (!tmp) { printf("realloc products failed\n"); return 1; }
                    products = tmp;
                }
                products[productCount] = lst[i];
                productCount++;
            } else {
                products[idx].total_reviews    += lst[i].total_reviews;
                products[idx].positive_reviews += lst[i].positive_reviews;
            }
        }
        free(lst);
    }
    free(local_lists); free(local_counts); free(local_caps);

    for (size_t i = 0; i < lines_cnt; i++) free(lines[i]);
    free(lines);

    // 4) حساب CQS (جزء من وقت الحساب)
    const double z = 1.96;
    #pragma omp parallel for
    for (int i = 0; i < productCount; i++) {
        products[i].cqs = wilson_lower_bound(products[i].positive_reviews, products[i].total_reviews, z);
    }

    double t2 = omp_get_wtime();  // نهاية الحساب (aggregation + merge + CQS)

    // 5) كتابة الملف
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

    double t3 = omp_get_wtime();  // نهاية الكتابة

    // ملخص الأزمنة بنفس شكل السيريل
    printf("Products counted: %d\n", productCount);
    printf("Read time:      %.6f s\n", t1 - t0);
    printf("Compute time:   %.6f s\n", t2 - t1);
    printf("Write time:     %.6f s\n", t3 - t2);
    printf("Total time:     %.6f s\n", t3 - t0);

    free(products);
    return 0;
}
