#include <stdio.h>
#include <omp.h>

double balance = 100.0;

void withdraw_atomic(double amount, int thread_id) {
    if (balance >= amount) {
        // DANGEROUS: Race condition between the if and the atomic write.
        // A single atomic write does not protect the entire conditional block.
        #pragma omp atomic
        balance -= amount;
        printf("Thread %d withdrew $%.2f. New balance: $%.2f\n", thread_id, amount, balance);
    } else {
        printf("Thread %d failed to withdraw $%.2f. Insufficient balance.\n", thread_id, amount);
    }
}

int main() {
    omp_set_num_threads(2);
    #pragma omp parallel
    {
        // Both threads attempt to withdraw $75.
        withdraw_atomic(75.0, omp_get_thread_num());
    }
    printf("\nFinal balance with unsafe atomic: $%.2f\n", balance);
    return 0;
}