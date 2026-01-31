#include <stdio.h>
#include <omp.h>

int main() {
    // Enable nested parallelism programmatically
    omp_set_nested(1);

    printf("Nested parallelism enabled: %d\n", omp_get_nested());

    #pragma omp parallel num_threads(2)
    {
        int outer_id = omp_get_thread_num();
        printf("Outer region: thread %d of %d\n",
               outer_id, omp_get_num_threads());

        // Inner parallel region
        #pragma omp parallel num_threads(3)
        {
            int inner_id = omp_get_thread_num();
            printf("  Inner region: outer %d -> inner %d of %d\n",
                   outer_id, inner_id, omp_get_num_threads());
        }
    }

    return 0;
}