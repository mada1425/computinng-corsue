#include <stdio.h>
#include <omp.h>

const int num_iterations = 100000;
FILE* file_a;
FILE* file_b;

void log_error(const char* msg, int thread_id) {
    #pragma omp critical (error_lock)
    {
        fprintf(file_a, "Thread %d: ERROR: %s\n", thread_id, msg);
    }
}

void log_status(const char* msg, int thread_id) {
    #pragma omp critical (status_lock)
    {
        fprintf(file_b, "Thread %d: STATUS: %s\n", thread_id, msg);
    }
}

int main() {
    double start_time, end_time;
    file_a = fopen("error.log", "w");
    file_b = fopen("status.log", "w");
    start_time = omp_get_wtime();

    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        #pragma omp for
        for (int i = 0; i < num_iterations; ++i) {
            log_error("Failed to process item.", thread_id);
            log_status("Item processed successfully.", thread_id);
        }
    }
    end_time = omp_get_wtime();

    fclose(file_a);
    fclose(file_b);

    printf("Logs generated using named critical sections. Time = %f seconds\n",
           end_time - start_time);
    return 0;
}
