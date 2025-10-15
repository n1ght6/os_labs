#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>

typedef struct {
    int start;
    int end;
    long long mod;
    long long partial_result;
} thread_data_t;

long long global_result = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* calculate_partial_factorial(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    long long partial = 1;
    
    for (int i = data->start; i <= data->end; i++) {
        partial = (partial * i) % data->mod;
    }
    
    data->partial_result = partial;
    

    pthread_mutex_lock(&mutex);
    global_result = (global_result * partial) % data->mod;
    pthread_mutex_unlock(&mutex);
    
    pthread_exit(NULL);
}

void print_usage() {
    printf("Usage: ./factorial -k <number> --pnum=<threads> --mod=<modulus>\n");
    printf("Example: ./factorial -k 10 --pnum=4 --mod=1000000\n");
}

int main(int argc, char* argv[]) {
    int k = 0;
    int pnum = 1;
    long long mod = 1;
    
    
    static struct option long_options[] = {
        {"pnum", required_argument, 0, 'p'},
        {"mod", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "k:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'k':
                k = atoi(optarg);
                break;
            case 'p':
                pnum = atoi(optarg);
                break;
            case 'm':
                mod = atoll(optarg);
                break;
            default:
                print_usage();
                return 1;
        }
    }
    
    if (k <= 0 || pnum <= 0 || mod <= 0) {
        print_usage();
        return 1;
    }
    
    printf("Calculating %d! mod %lld using %d threads\n", k, mod, pnum);
    
    pthread_t threads[pnum];
    thread_data_t thread_data[pnum];
    
  
    int elements_per_thread = k / pnum;
    int remainder = k % pnum;
    int current_start = 1;
    
    for (int i = 0; i < pnum; i++) {
        int current_end = current_start + elements_per_thread - 1;
        if (i < remainder) {
            current_end++;
        }
        
        thread_data[i].start = current_start;
        thread_data[i].end = current_end;
        thread_data[i].mod = mod;
        thread_data[i].partial_result = 1;
        
        printf("Thread %d: calculating from %d to %d\n", 
               i, current_start, current_end);
        
        pthread_create(&threads[i], NULL, calculate_partial_factorial, &thread_data[i]);
        
        current_start = current_end + 1;
    }
    
    
    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("Result: %d! mod %lld = %lld\n", k, mod, global_result);
    
    pthread_mutex_destroy(&mutex);
    return 0;
}
