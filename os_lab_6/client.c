#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "common.h"


struct Server {
  char ip[255];
  int port;
};

struct ThreadData {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
};


bool ParseServerFile(const char* filename, struct Server** servers, int* servers_count) {
  FILE* file = fopen(filename, "r");
  if (!file) {
    return false;
  }

  char line[255];
  int count = 0;
  while (fgets(line, sizeof(line), file)) {
    count++;
  }

  *servers = malloc(sizeof(struct Server) * count);
  *servers_count = count;

  rewind(file);
  count = 0;
  while (fgets(line, sizeof(line), file)) {
    char* colon = strchr(line, ':');
    if (!colon) continue;
    
    *colon = '\0';
    strncpy((*servers)[count].ip, line, sizeof((*servers)[count].ip) - 1);
    (*servers)[count].port = atoi(colon + 1);
    count++;
  }

  fclose(file);
  return true;
}


void* ProcessServer(void* arg) {
  struct ThreadData* data = (struct ThreadData*)arg;
  
  struct hostent *hostname = gethostbyname(data->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", data->server.ip);
    data->result = 1;
    pthread_exit(NULL);
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(data->server.port);
  
  
  if (hostname->h_addr_list[0] != NULL) {
    server_addr.sin_addr.s_addr = *((unsigned long *)hostname->h_addr_list[0]);
  } 
  else {
    fprintf(stderr, "No address found for %s\n", data->server.ip);
    data->result = 1;
    pthread_exit(NULL);
  }

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  
  if (sock < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    data->result = 1;
    pthread_exit(NULL);
  }


  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    fprintf(stderr, "Connection to %s:%d failed\n", data->server.ip, data->server.port);
    close(sock);
    data->result = 1;
    pthread_exit(NULL);
  }

  char task[sizeof(uint64_t) * 3];
  memcpy(task, &data->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &data->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &data->mod, sizeof(uint64_t));


  if (send(sock, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send failed to %s:%d\n", data->server.ip, data->server.port);
    close(sock);
    data->result = 1;
    pthread_exit(NULL);
  }


  char response[sizeof(uint64_t)];
  if (recv(sock, response, sizeof(response), 0) < 0) {
    fprintf(stderr, "Receive failed from %s:%d\n", data->server.ip, data->server.port);
    close(sock);
    data->result = 1;
    pthread_exit(NULL);
  }


  memcpy(&data->result, response, sizeof(uint64_t));
  printf("Server %s:%d returned: %lu\n", data->server.ip, data->server.port, data->result);


  close(sock);
  pthread_exit(NULL);
}

int main(int argc, char **argv) {
  uint64_t k = 0;
  uint64_t mod = 0;
  char servers_file[255] = {'\0'};

  while (true) { 
    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1) break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        break;
      case 2:
        memcpy(servers_file, optarg, strlen(optarg));
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  
  if (k == 0 || mod == 0 || !strlen(servers_file)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  struct Server* servers = NULL;
  int servers_num = 0;
  if (!ParseServerFile(servers_file, &servers, &servers_num)) {
    fprintf(stderr, "Cannot read servers file: %s\n", servers_file);
    return 1;
  }


  printf("Found %d servers\n", servers_num);


  pthread_t threads[servers_num];
  struct ThreadData thread_data[servers_num];


  uint64_t numbers_per_server = k / servers_num;
  
  int remainder = (int)(k % servers_num);
  uint64_t current_start = 1;

  for (int i = 0; i < servers_num; i++) {
    thread_data[i].server = servers[i];
    thread_data[i].begin = current_start;
    thread_data[i].end = current_start + numbers_per_server - 1;
    
    
    if (i < remainder) {
      thread_data[i].end++;
    }
    
    thread_data[i].mod = mod;
    thread_data[i].result = 1;

    printf("Server %d (%s:%d): calculating from %lu to %lu\n", 
           i, servers[i].ip, servers[i].port, thread_data[i].begin, thread_data[i].end);

    pthread_create(&threads[i], NULL, ProcessServer, &thread_data[i]);
    current_start = thread_data[i].end + 1;
  }

  uint64_t total_result = 1;
  for (int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    total_result = MultModulo(total_result, thread_data[i].result, mod);
  }

  printf("Final result: %lu! mod %lu = %lu\n", k, mod, total_result);

  free(servers);
  return 0;
}
