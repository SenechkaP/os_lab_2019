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
#include <fcntl.h>
#include <errno.h>

#include <getopt.h>
#include <signal.h>

#include "find_min_max.h"
#include "utils.h"

static pid_t *g_child_pids = NULL;
static int g_pnum = 0;
static volatile sig_atomic_t g_timed_out = 0;

void alarm_handler(int signum) {
  (void)signum;
  g_timed_out = 1;
  if (g_child_pids) {
    for (int i = 0; i < g_pnum; i++) {
      if (g_child_pids[i] > 0) {
        kill(g_child_pids[i], SIGKILL);
      }
    }
  }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  int timeout = 0;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"timeout", required_argument, 0, 0},
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
              fprintf(stderr, "seed must be positive\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              fprintf(stderr, "array_size must be positive\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              fprintf(stderr, "pnum must be positive\n");
              return 1;
            }
            break;
          case 3:
            timeout = atoi(optarg);
            if (timeout <= 0) {
              fprintf(stderr, "timeout must be positive\n");
              return 1;
            }
            break;
          case 4:
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
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--by_files] [--timeout \"seconds\"]\n",
           argv[0]);
    return 1;
  }

  if (pnum > array_size) {
    fprintf(stderr, "pnum should not be greater than array_size\n");
    return 1;
  }

  g_pnum = pnum;
  g_child_pids = malloc(sizeof(pid_t) * pnum);
  if (!g_child_pids) {
    perror("malloc child_pids");
    return 1;
  }
  for (int i = 0; i < pnum; i++) g_child_pids[i] = -1;

  int *array = malloc(sizeof(int) * array_size);
  if (!array) {
    perror("malloc");
    free(g_child_pids);
    return 1;
  }
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  int (*pipes)[2] = NULL;
  if (!with_files) {
    pipes = malloc(sizeof(int[2]) * pnum);
    if (!pipes) {
      perror("malloc pipes");
      free(array);
      free(g_child_pids);
      return 1;
    }
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[i]) == -1) {
        perror("pipe");
        for (int j = 0; j < i; j++) {
          close(pipes[j][0]);
          close(pipes[j][1]);
        }
        free(pipes);
        free(array);
        free(g_child_pids);
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      if (child_pid == 0) {
        int portion_size = array_size / pnum;
        int begin = i * portion_size;
        int end = (i == pnum - 1) ? array_size : begin + portion_size;

        struct MinMax mm = GetMinMax(array, begin, end);
        sleep(timeout); // demo

        if (with_files) {
          char filename[256];
          snprintf(filename, sizeof(filename), "tmp_min_max_%d.txt", i);
          FILE *f = fopen(filename, "w");
          if (!f) {
            fprintf(stderr, "Child %d: fopen failed: %s\n", i, strerror(errno));
            _exit(1);
          }
          fprintf(f, "%d %d\n", mm.min, mm.max);
          fclose(f);
        } else {
          close(pipes[i][0]);

          if (write(pipes[i][1], &mm.min, sizeof(mm.min)) == -1) {
            _exit(1);
          }
          if (write(pipes[i][1], &mm.max, sizeof(mm.max)) == -1) {
            _exit(1);
          }
          close(pipes[i][1]);
        }
        _exit(0);
      } else {
        g_child_pids[i] = child_pid;
        if (!with_files) {
          close(pipes[i][1]);
        }
      }

    } else {
      printf("Fork failed!\n");
      if (!with_files && pipes) {
        for (int j = 0; j < pnum; j++) {
          close(pipes[j][0]);
          close(pipes[j][1]);
        }
        free(pipes);
      }
      free(array);
      free(g_child_pids);
      return 1;
    }
  }

  if (timeout > 0) {
    if (signal(SIGALRM, alarm_handler) == SIG_ERR) {
      perror("signal");
    } else {
      alarm((unsigned int)timeout);
    }
  }

  while (active_child_processes > 0) {
    int status = 0;
    pid_t w = waitpid(-1, &status, WNOHANG);
    if (w == 0) {
      usleep(100 * 1000); // 100ms
      continue;
    } else if (w > 0) {
      active_child_processes -= 1;
    } else { // w == -1
      if (errno == ECHILD) {
        break;
      } else {
        perror("waitpid");
        break;
      }
    }
  }

  if (timeout > 0) alarm(0);

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int local_min = INT_MAX;
    int local_max = INT_MIN;

    if (with_files) {
      char filename[256];
      snprintf(filename, sizeof(filename), "tmp_min_max_%d.txt", i);
      FILE *f = fopen(filename, "r");
      if (!f) {
        fprintf(stderr, "Parent: fopen %s failed: %s\n", filename, strerror(errno));
        continue;
      }
      if (fscanf(f, "%d %d", &local_min, &local_max) != 2) {
        fprintf(stderr, "Parent: fscanf %s failed\n", filename);
      }
      fclose(f);
      remove(filename);
    } else {
      ssize_t r;
      r = read(pipes[i][0], &local_min, sizeof(local_min));
      if (r != sizeof(local_min)) {
        if (r == 0) {
          fprintf(stderr, "Parent: pipe %d: unexpected EOF when reading min\n", i);
        } else {
          perror("read min from pipe");
        }
      }
      r = read(pipes[i][0], &local_max, sizeof(local_max));
      if (r != sizeof(local_max)) {
        if (r == 0) {
          fprintf(stderr, "Parent: pipe %d: unexpected EOF when reading max\n", i);
        } else {
          perror("read max from pipe");
        }
      }
      close(pipes[i][0]);
    }

    if (local_min < min_max.min) min_max.min = local_min;
    if (local_max > min_max.max) min_max.max = local_max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  if (pipes) free(pipes);
  free(array);

  if (g_child_pids) free(g_child_pids);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);

  if (g_timed_out) {
    printf("Program timed out after %d seconds; parent sent SIGKILL to child processes.\n", timeout);
  }

  fflush(NULL);
  return 0;
}
