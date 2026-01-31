#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define LINE_SIZE 65536

typedef struct {
    char asin[30];
    int  total_reviews;
    int  positive_reviews;
    double cqs;
} Product;

/* ===================== Wilson Lower Bound ===================== */
double wilson_lower_bound(int positive, int total, double z) {
    if (total == 0) return 0.0;

    double p = (double)positive / total;

    double numerator = p + (z*z)/(2.0*total)
        - z * sqrt((p*(1.0-p) + (z*z)/(4.0*total)) / total);
    double denominator = 1.0 + (z*z)/total;

    return numerator / denominator;
}

int extract_asin_overall(const char *line,
                         char *asin_out, size_t asin_sz,
                         double *overall_out)
{
    const char *k = strstr(line, "\"asin\"");
    if (!k) return 0;
    k = strchr(k, ':'); if (!k) return 0;
    k++;
    while (*k==' ' || *k=='\t') k++;
    if (*k != '\"') return 0;
    k++;

    size_t j = 0;
    while (*k && *k != '\"' && j < asin_sz - 1) {
        asin_out[j++] = *k++;
    }
    asin_out[j] = '\0';
    if (j == 0) return 0;

    const char *p = strstr(line, "\"overall\"");
    if (!p) return 0;
    p = strchr(p, ':'); if (!p) return 0;
    p++;
    while (*p==' ' || *p=='\t') p++;

    char buf[64];
    j = 0;
    while (*p && j < sizeof(buf)-1) {
        if ((*p >= '0' && *p <= '9') || *p == '.' || *p == '-') {
            buf[j++] = *p++;
        } else if (*p == ',' || *p == ' ' || *p == '}' || *p == '\n') {
            break;
        } else {
            p++;
        }
    }
    buf[j] = '\0';
    if (j == 0) return 0;

    *overall_out = atof(buf);
    return 1;
}


void add_or_update_product(Product **list,
                           int *count,
                           int *capacity,
                           const char *asin,
                           double overall)
{
    for (int i = 0; i < *count; i++) {
        if (strcmp((*list)[i].asin, asin) == 0) {
            (*list)[i].total_reviews++;
            if (overall >= 4.0) {
                (*list)[i].positive_reviews++;
            }
            return;
        }
    }

    if (*count == *capacity) {
        int new_cap = (*capacity == 0) ? 1024 : (*capacity * 2);
        Product *tmp = realloc(*list, new_cap * sizeof(Product));
        if (!tmp) {
            fprintf(stderr, "realloc failed in add_or_update_product\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        *list = tmp;
        *capacity = new_cap;
    }

    Product *p = &((*list)[*count]);
    strncpy(p->asin, asin, sizeof(p->asin) - 1);
    p->asin[sizeof(p->asin) - 1] = '\0';
    p->total_reviews    = 1;
    p->positive_reviews = (overall >= 4.0) ? 1 : 0;
    p->cqs              = 0.0;

    (*count)++;
}


int findProduct(Product *products, int count, const char *asin) {
    for (int i = 0; i < count; i++) {
        if (strcmp(products[i].asin, asin) == 0)
            return i;
    }
    return -1;
}


int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const char *input_path  = "reviews_Cell_Phones_and_Accessories.json";
    const char *output_path = "cqs_output_MPI_simple.txt";

    double comp_local = 0.0, comp_time = 0.0;
    double comm_local = 0.0, comm_time = 0.0;
    double total_start = 0.0, total_end = 0.0;

    MPI_Barrier(MPI_COMM_WORLD);
    total_start = MPI_Wtime(); 

    MPI_Barrier(MPI_COMM_WORLD);
    double comp_start = MPI_Wtime();

    FILE *f = fopen(input_path, "r");
    if (!f) {
        fprintf(stderr, "Rank %d: cannot open input file %s\n",
                rank, input_path);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    Product *local_products = NULL;
    int local_count = 0;
    int local_cap   = 0;

    char line_buf[LINE_SIZE];
    long long line_index = 0;

    while (fgets(line_buf, sizeof(line_buf), f)) {

        if ((line_index % size) == rank) {
            char asin[30];
            double overall;

            if (extract_asin_overall(line_buf, asin, sizeof(asin), &overall)) {
                add_or_update_product(&local_products,
                                      &local_count,
                                      &local_cap,
                                      asin, overall);
            }
        }

        line_index++;
    }

    fclose(f);

    MPI_Barrier(MPI_COMM_WORLD);
    double comp_end = MPI_Wtime();
    comp_local = comp_end - comp_start;

    MPI_Reduce(&comp_local, &comp_time, 1, MPI_DOUBLE,
               MPI_MAX, 0, MPI_COMM_WORLD);


    int local_bytes = local_count * (int)sizeof(Product);

    int *recv_counts = NULL;
    int *displs      = NULL;
    Product *all_products = NULL;
    int total_bytes = 0;

    if (rank == 0) {
        recv_counts = malloc(size * sizeof(int));
        if (!recv_counts) {
            fprintf(stderr, "Rank 0: malloc failed (recv_counts)\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double comm_start = MPI_Wtime();

    MPI_Gather(&local_bytes, 1, MPI_INT,
               recv_counts,   1, MPI_INT,
               0, MPI_COMM_WORLD);

    if (rank == 0) {
        displs = malloc(size * sizeof(int));
        if (!displs) {
            fprintf(stderr, "Rank 0: malloc failed (displs)\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        displs[0] = 0;
        total_bytes = recv_counts[0];
        for (int i = 1; i < size; i++) {
            displs[i] = displs[i-1] + recv_counts[i-1];
            total_bytes += recv_counts[i];
        }

        all_products = malloc(total_bytes);
        if (!all_products) {
            fprintf(stderr, "Rank 0: malloc failed (all_products)\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Gatherv(local_products, local_bytes, MPI_BYTE,
                all_products,  recv_counts, displs, MPI_BYTE,
                0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    double comm_end = MPI_Wtime();
    comm_local = comm_end - comm_start;

    MPI_Reduce(&comm_local, &comm_time, 1, MPI_DOUBLE,
               MPI_MAX, 0, MPI_COMM_WORLD);


    if (rank == 0) {
        int total_products = total_bytes / (int)sizeof(Product);

        int capacity = 4096;
        int productCount = 0;
        Product *products = malloc(capacity * sizeof(Product));
        if (!products) {
            fprintf(stderr, "Rank 0: malloc failed (products)\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        for (int i = 0; i < total_products; i++) {
            Product *p = &all_products[i];
            int idx = findProduct(products, productCount, p->asin);
            if (idx == -1) {
                if (productCount == capacity) {
                    int newcap = capacity * 2;
                    Product *tmp = realloc(products,
                                           newcap * sizeof(Product));
                    if (!tmp) {
                        fprintf(stderr, "Rank 0: realloc failed (products)\n");
                        MPI_Abort(MPI_COMM_WORLD, 1);
                    }
                    capacity = newcap;
                    products = tmp;
                }
                products[productCount++] = *p;
            } else {
                products[idx].total_reviews    += p->total_reviews;
                products[idx].positive_reviews += p->positive_reviews;
            }
        }

        const double z = 1.96;
        for (int i = 0; i < productCount; i++) {
            products[i].cqs =
                wilson_lower_bound(products[i].positive_reviews,
                                   products[i].total_reviews,
                                   z);
        }

        FILE *out = fopen(output_path, "w");
        if (!out) {
            fprintf(stderr, "Rank 0: cannot open output file %s\n",
                    output_path);
            free(products);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        fprintf(out, "%-15s %-10s %-10s %-10s\n",
                "ASIN", "Total", "Positive", "CQS");
        fprintf(out, "-------------------------------------------------------------\n");

        for (int i = 0; i < productCount; i++) {
            fprintf(out, "%-15s %-10d %-10d %-10.4f\n",
                    products[i].asin,
                    products[i].total_reviews,
                    products[i].positive_reviews,
                    products[i].cqs);
        }

        fclose(out);

        free(products);
        free(all_products);
        free(recv_counts);
        free(displs);
    }


    free(local_products);

    MPI_Barrier(MPI_COMM_WORLD);
    total_end = MPI_Wtime();

    if (rank == 0) {
        printf("Compute time: %.6f s\n", comp_time);
        printf("Comm time  : %.6f s\n", comm_time);
        printf("Total time:                       %.6f s\n",
               total_end - total_start);
    }

    MPI_Finalize();
    return 0;
}
