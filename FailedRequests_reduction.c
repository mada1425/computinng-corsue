#include <stdio.h>
#include <omp.h>

int main() {
    FILE *file;
    char time[50], resource[100];
    int user_id, status;
    int total = 0;

    file = fopen("large_web_log.txt", "r");
    if (file == NULL) {
        printf("error for open the file\n");
        return 1;
    }

    while (fscanf(file, "%s %d %s %d", time, &user_id, resource, &status) == 4) {
        total++;
    }
    rewind(file); 

    int statuses[total];
    for (int i = 0; i < total; i++) {
        fscanf(file, "%s %d %s %d", time, &user_id, resource, &status);
        statuses[i] = status;
    }
    fclose(file);

    int failed = 0;
    double start_time = omp_get_wtime();

    #pragma omp parallel for reduction(+:failed)
    for (int i = 0; i < total; i++) {
        if (statuses[i] != 200) {
            failed += 1;
        }
    }

    double end_time = omp_get_wtime();

    printf("[reduction] failed=%d  total=%d  time=%.8f s\n", failed, total,  end_time - start_time);
    return 0;
}
