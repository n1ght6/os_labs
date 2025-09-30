#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include <fcntl.h>

#include "find_min_max.h"
#include "utils.h"

volatile sig_atomic_t timeout_expired = 0;

void timeout_handler(int sig) {
    (void)sig;  // Подавляем предупреждение о неиспользуемом параметре
    timeout_expired = 1;
}

int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    int timeout = -1;
    bool with_files = false;

    while (1) {
        static struct option options[] = {
            {"seed", required_argument, 0, 's'},
            {"array_size", required_argument, 0, 'a'},
            {"pnum", required_argument, 0, 'p'},
            {"by_files", no_argument, 0, 'f'},
            {"timeout", required_argument, 0, 't'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "s:a:p:ft:", options, NULL);
        if (c == -1) break;

        switch (c) {
            case 's':
                seed = atoi(optarg);
                break;
            case 'a':
                array_size = atoi(optarg);
                break;
            case 'p':
                pnum = atoi(optarg);
                break;
            case 'f':
                with_files = true;
                break;
            case 't':
                timeout = atoi(optarg);
                break;
            default:
                printf("Usage: %s --seed <num> --array_size <num> --pnum <num> [--by_files] [--timeout <sec>]\n", argv[0]);
                return 1;
        }
    }

    if (seed == -1 || array_size == -1 || pnum == -1) {
        printf("Usage: %s --seed <num> --array_size <num> --pnum <num> [--by_files] [--timeout <sec>]\n", argv[0]);
        return 1;
    }

    if (timeout > 0) {
        signal(SIGALRM, timeout_handler);
    }

    int *array = malloc(sizeof(int) * array_size);
    if (array == NULL) {
        printf("Memory allocation failed\n");
        return 1;
    }
    GenerateArray(array, array_size, seed);

    int active_child_processes = 0;
    pid_t *child_pids = malloc(sizeof(pid_t) * pnum);
    if (child_pids == NULL) {
        printf("Memory allocation failed\n");
        free(array);
        return 1;
    }
    
    int (*pipe_fd)[2] = NULL;
    if (!with_files) {
        pipe_fd = malloc(pnum * sizeof(int[2]));
        if (pipe_fd == NULL) {
            printf("Memory allocation failed\n");
            free(array);
            free(child_pids);
            return 1;
        }
        for (int i = 0; i < pnum; i++) {
            if (pipe(pipe_fd[i]) == -1) {
                perror("pipe");
                free(array);
                free(child_pids);
                free(pipe_fd);
                return 1;
            }
        }
    }

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    for (int i = 0; i < pnum; i++) {
        pid_t child_pid = fork();
        if (child_pid == 0) {
            if (!with_files) {
                for (int j = 0; j < pnum; j++) {
                    if (j != i) {
                        close(pipe_fd[j][0]);
                        close(pipe_fd[j][1]);
                    }
                }
            }

            int begin = i * (array_size / pnum);
            int end = (i == pnum - 1) ? array_size : (i + 1) * (array_size / pnum);
            
            struct MinMax local_min_max = GetMinMax(array, begin, end);

            if (with_files) {
                char filename_min[20], filename_max[20];
                sprintf(filename_min, "min_%d", i);
                sprintf(filename_max, "max_%d", i);
                FILE *fmin = fopen(filename_min, "w");
                FILE *fmax = fopen(filename_max, "w");
                if (fmin) {
                    fprintf(fmin, "%d", local_min_max.min);
                    fclose(fmin);
                }
                if (fmax) {
                    fprintf(fmax, "%d", local_min_max.max);
                    fclose(fmax);
                }
            } else {
                close(pipe_fd[i][0]);
                write(pipe_fd[i][1], &local_min_max.min, sizeof(int));
                write(pipe_fd[i][1], &local_min_max.max, sizeof(int));
                close(pipe_fd[i][1]);
            }
            free(array);
            if (!with_files) free(pipe_fd);
            free(child_pids);
            exit(0);
        } else if (child_pid > 0) {
            child_pids[active_child_processes] = child_pid;
            active_child_processes++;
        } else {
            printf("Fork failed!\n");
            free(array);
            free(child_pids);
            if (!with_files) free(pipe_fd);
            return 1;
        }
    }

    if (!with_files) {
        for (int i = 0; i < pnum; i++) {
            close(pipe_fd[i][1]);
        }
    }

    if (timeout > 0) {
        alarm(timeout);
    }

    int completed_processes = 0;
    while (completed_processes < pnum) {
        int status;
        pid_t finished_pid = wait(&status);
        
        if (finished_pid > 0) {
            completed_processes++;
            for (int i = 0; i < active_child_processes; i++) {
                if (child_pids[i] == finished_pid) {
                    for (int j = i; j < active_child_processes - 1; j++) {
                        child_pids[j] = child_pids[j + 1];
                    }
                    active_child_processes--;
                    break;
                }
            }
        }
        
        if (timeout_expired && active_child_processes > 0) {
            printf("Timeout expired! Killing child processes...\n");
            for (int i = 0; i < active_child_processes; i++) {
                kill(child_pids[i], SIGKILL);
            }
            break;
        }
    }

    if (timeout > 0) {
        alarm(0);
    }

    struct MinMax min_max;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    for (int i = 0; i < pnum; i++) {
        int min_val, max_val;

        if (with_files) {
            char filename_min[20], filename_max[20];
            sprintf(filename_min, "min_%d", i);
            sprintf(filename_max, "max_%d", i);
            
            FILE *fmin = fopen(filename_min, "r");
            FILE *fmax = fopen(filename_max, "r");
            
            if (fmin != NULL) {
                fscanf(fmin, "%d", &min_val);
                fclose(fmin);
                remove(filename_min);
            } else {
                min_val = INT_MAX;
            }
            
            if (fmax != NULL) {
                fscanf(fmax, "%d", &max_val);
                fclose(fmax);
                remove(filename_max);
            } else {
                max_val = INT_MIN;
            }
        } else {
            min_val = INT_MAX;
            max_val = INT_MIN;
            
            if (read(pipe_fd[i][0], &min_val, sizeof(int)) > 0) {
                read(pipe_fd[i][0], &max_val, sizeof(int));
            }
            close(pipe_fd[i][0]);
        }

        if (min_val < min_max.min) min_max.min = min_val;
        if (max_val > min_max.max) min_max.max = max_val;
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);
    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);
    free(child_pids);
    if (!with_files) free(pipe_fd);
    
    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
    printf("Elapsed time: %fms\n", elapsed_time);
    
    return 0;
}
