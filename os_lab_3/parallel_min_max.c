#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include <fcntl.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
  
    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed must be a positive number\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size must be a positive number\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum must be a positive number\n");
              return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case '?':
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--by_files]\n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  if (array == NULL) {
    printf("Memory allocation failed\n");
    return 1;
  }
  
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  int pipe_fd[2][pnum]; 
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipe_fd[i]) == -1) {
        perror("pipe");
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);


  pid_t *child_pids = malloc(sizeof(pid_t) * pnum);
  if (child_pids == NULL) {
    printf("Memory allocation failed\n");
    return 1;
  }

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      if (child_pid == 0) {

        if (!with_files) {

          for (int j = 0; j < pnum; j++) {
            if (j != i) {
              close(pipe_fd[j][0]);
              close(pipe_fd[j][1]);
            } else {
              close(pipe_fd[j][0]);
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
          if (fmin == NULL || fmax == NULL) {
            printf("Failed to create files\n");
            exit(1);
          }
          fprintf(fmin, "%d", local_min_max.min);
          fprintf(fmax, "%d", local_min_max.max);
          fclose(fmin);
          fclose(fmax);
        } else {

          write(pipe_fd[i][1], &local_min_max.min, sizeof(int));
          write(pipe_fd[i][1], &local_min_max.max, sizeof(int));
          close(pipe_fd[i][1]);
        }
        
        free(array);
        exit(0);
      } else {

        child_pids[i] = child_pid;
        active_child_processes += 1;
      }
    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }


  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipe_fd[i][1]);
    }
  }


  while (active_child_processes > 0) {
    int status;
    pid_t finished_pid = wait(&status);
    if (finished_pid > 0) {
      active_child_processes -= 1;
    }
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {

      char filename_min[20], filename_max[20];
      sprintf(filename_min, "min_%d", i);
      sprintf(filename_max, "max_%d", i);
      
      FILE *fmin = fopen(filename_min, "r");
      FILE *fmax = fopen(filename_max, "r");
      
      if (fmin != NULL) {
        fscanf(fmin, "%d", &min);
        fclose(fmin);
        remove(filename_min); 
      }
      if (fmax != NULL) {
        fscanf(fmax, "%d", &max);
        fclose(fmax);
        remove(filename_max); 
      }
    } else {

      read(pipe_fd[i][0], &min, sizeof(int));
      read(pipe_fd[i][0], &max, sizeof(int));
      close(pipe_fd[i][0]); 
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(child_pids);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  
  return 0;
}
