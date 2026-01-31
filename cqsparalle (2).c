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

// Wilson Lower Bound
static double wilson_lower_bound(int positive, int total, double z) {
    if (total == 0) return 0.0;
    double phat = (double)positive / total;
    double denom = 1.0 + (z*z)/total;
    double num = phat + (z*z)/(2.0*total) - z * sqrt((phat*(1.0 - phat) + (z*z)/(4.0*total)) / total);
    return num / denom;
}

// البحث عن المنتج في المصفوفة
static int findProduct(struct Product *products, int count, const char *asin) {
    for (int i = 0; i < count; i++) {
        if (strcmp(products[i].asin, asin) == 0)
            return i;
    }
    return -1;
}

// استخراج asin و overall من السطر
static int extract_asin_overall(const char *line, char *asin_out, size_t asin_sz, double *overall_out) {
    const char *k = strstr(line, "\"asin\"");
    if (!k) return 0;
    k = strchr(k, ':'); if (!k) return 0;
    k++;
    while (*k==' ' || *k=='\t') k++;
    if (*k != '\"') return 0;
    k++;
    size_t j = 0;
    while (*k && *k != '\"' && j < asin_sz - 1) asin_out[j++] = *k++;
    asin_out[j] = '\0';
    if (j == 0) return 0;

    const char *p = strstr(line, "\"overall\"");
    if (!p) return 0;
    p = strchr(p, ':'); if (!p) return 0;
    p++;
    while (*p==' ' || *p=='\t') p++;
    char buf[64]; j = 0;
    while (*p && j < (int)sizeof(buf)-1) {
        if ((*p >= '0' && *p <= '9') || *p == '.' || *p == '-') buf[j++] = *p++;
        else if (*p == ',' || *p == ' ' || *p == '}' || *p == '\n') break;
        else p++;
    }
    buf[j] = '\0';
    if (j == 0) return 0;

    *overall_out = atof(buf);
    return 1;
}

int main(void) {
    const char *input_path  = "reviews_Cell_Phones_and_Accessories.json";
    const char *output_path = "cqs_output_PL.txt";

    double t_start = omp_get_wtime();

    FILE *file = fopen(input_path, "r");
    if (!file) {
        fprintf(stderr, "Error: cannot open input file!\n");
        return 1;
    }

    // قراءة كل الأسطر في الذاكرة
    size_t lines_cap = 1 << 14;
    size_t lines_cnt = 0;
    char **lines = (char**)malloc(lines_cap * sizeof(char*));
    if (!lines) { fprintf(stderr, "Memory error\n"); fclose(file); return 1; }

    char buf[LINE_SIZE];
    while (fgets(buf, sizeof(buf), file)) {
        size_t L = strlen(buf);
        char *s = (char*)malloc(L + 1);
        if (!s) { fprintf(stderr, "Memory error (line)\n"); fclose(file); return 1; }
        memcpy(s, buf, L + 1);
        if (lines_cnt == lines_cap) {
            size_t newcap = lines_cap * 2;
            char **tmp = realloc(lines, newcap * sizeof(char*));
            if (!tmp) { fprintf(stderr, "Memory error (realloc lines)\n"); fclose(file); return 1; }
            lines = tmp;
            lines_cap = newcap;
        }
        lines[lines_cnt++] = s;
    }
    fclose(file);

    double t_read_end = omp_get_wtime();

    // إعداد القوائم المحلية
    int threads = omp_get_thread_num();
    struct Product **local_lists = calloc(max_threads, sizeof(struct Product*));
    int *local_counts = calloc(max_threads, sizeof(int));
    int *local_caps   = calloc(max_threads, sizeof(int));
    if (!local_lists || !local_counts || !local_caps) {
        fprintf(stderr, "Memory alloc failed for local arrays\n");
        return 1;
    }

    int global_error = 0; // فلاج مشترك

    double t_compute_start = omp_get_wtime();

    // معالجة متوازية
    #pragma omp parallel shared(global_error, local_lists, local_counts, local_caps)
    {
        int tid = omp_get_thread_num();
        int cap = 2048;
        int count = 0;
        struct Product *list = malloc(sizeof(struct Product) * cap);
        if (!list) {
            fprintf(stderr, "Thread %d: memory alloc failed\n", tid);
            global_error = 1;
        }

        #pragma omp for schedule(runtime)
        for (long long i = 0; i < (long long)lines_cnt; i++) {
            if (global_error) continue; // لو صار خطأ، نتخطى الباقي

            char asin[30];
            double overall;
            if (!extract_asin_overall(lines[i], asin, sizeof(asin), &overall))
                continue;

            int idx = -1;
            for (int k = 0; k < count; k++) {
                if (strcmp(list[k].asin, asin) == 0) {
                    idx = k;
                    break;
                }
            }

            if (idx == -1) {
                if (count == cap) {
                    int newcap = cap * 2;
                    struct Product *tmp = realloc(list, sizeof(struct Product) * newcap);
                    if (!tmp) {
                        fprintf(stderr, "Thread %d: realloc failed\n", tid);
                        global_error = 1;
                        continue;
                    }
                    list = tmp;
                    cap = newcap;
                }
                strncpy(list[count].asin, asin, sizeof(list[count].asin) - 1);
                list[count].asin[sizeof(list[count].asin) - 1] = '\0';
                list[count].total_reviews = 1;
                list[count].positive_reviews = (overall >= 4.0) ? 1 : 0;
                list[count].cqs = 0.0;
                count++;
            } else {
                list[idx].total_reviews++;
                if (overall >= 4.0) list[idx].positive_reviews++;
            }
        }

        // save local results
        local_lists[tid] = list;
        local_counts[tid] = count;
        local_caps[tid] = cap;
    }

    if (global_error) {
        fprintf(stderr, "Error occurred during parallel processing.\n");
        return 1;
    }

    // دمج النتائج
    int capacity = 4096, productCount = 0;
    struct Product *products = malloc(sizeof(struct Product) * capacity);
    if (!products) { fprintf(stderr, "Memory alloc failed for products\n"); return 1; }

    for (int t = 0; t < max_threads; t++) {
        struct Product *lst = local_lists[t];
        int cnt = local_counts[t];
        for (int i = 0; i < cnt; i++) {
            int idx = findProduct(products, productCount, lst[i].asin);
            if (idx == -1) {
                if (productCount == capacity) {
                    int newcap = capacity * 2;
                    struct Product *tmp = realloc(products, sizeof(struct Product) * newcap);
                    if (!tmp) { fprintf(stderr, "Realloc failed for products\n"); return 1; }
                    products = tmp;
                    capacity = newcap;
                }
                products[productCount++] = lst[i];
            } else {
                products[idx].total_reviews += lst[i].total_reviews;
                products[idx].positive_reviews += lst[i].positive_reviews;
            }
        }
        free(lst);
    }

    for (size_t i = 0; i < lines_cnt; i++) free(lines[i]);
    free(lines);
    free(local_lists);
    free(local_counts);
    free(local_caps);

    // حساب CQS موازي
    const double z = 1.96;
    #pragma omp parallel for schedule(runtime)
    for (int i = 0; i < productCount; i++) {
        products[i].cqs = wilson_lower_bound(products[i].positive_reviews, products[i].total_reviews, z);
    }
    double t_compute_end = omp_get_wtime(); /* <-- تم إضافة الفاصلة المنقوطة هنا */

    // كتابة الناتج
    FILE *out = fopen(output_path, "w");
    if (!out) {
        fprintf(stderr, "Error writing output file\n");
        free(products);
        return 1;
    }

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


    double t_write_end = omp_get_wtime();

    // عرض الوقت مثل serial
    printf("Products counted: %d\n", productCount);
    printf("Read time:      %.6f s\n", t_read_end - t_start);
    printf("Compute time:   %.6f s\n", t_compute_end - t_compute_start);
    printf("Write time:     %.6f s\n", t_write_end - t_compute_end);
    printf("Total time:     %.6f s\n", t_write_end - t_start);

    free(products);
    return 0;
}
