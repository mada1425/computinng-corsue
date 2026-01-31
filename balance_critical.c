#include <stdio.h>
#include <omp.h>

double balance = 100.0;

void withdraw_critical(double amount, int thread_id) {
    #pragma omp critical
    {
        // SAFE: The entire conditional block is protected by the critical section.
        if (balance >= amount) {
            balance -= amount;
            printf("Thread %d withdrew $%.2f. New balance: $%.2f\n", thread_id, amount, balance);
        } else {
            printf("Thread %d failed to withdraw $%.2f. Insufficient balance.\n", thread_id, balance);
        }
    }
}

int main() {
    omp_set_num_threads(2);
    #pragma omp parallel
    {
        // Both threads attempt to withdraw $75.
        withdraw_critical(75.0, omp_get_thread_num());
    }
    printf("\nFinal balance with safe critical: $%.2f\n", balance);
    return 0;
}