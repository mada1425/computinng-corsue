#include <stdio.h>
#include <omp.h>

int main() {
    #pragma omp parallel for ordered
    for (int i = 0; i < 4; i++) {
        // This part runs in parallel
        printf("Parallel processing item %d by thread %d\n", i, omp_get_thread_num());

        // This part is executed sequentially in loop order
        #pragma omp ordered
        {
            printf("Ordered processing item %d\n", i);
        }
    }
    return 0;
}