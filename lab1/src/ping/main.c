#include "ping.h"
#include <shared/network.h>
#include <shared/random.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define DEFAULT_COUNT 3
#define DEFAULT_TIMEOUT 1.0
#define DEFAULT_PAYLOAD_SIZE 16
#define MAX_IP_COUNT 8
#define IP_DELIM ","

struct ThreadData {
    struct sockaddr_in dest;
    int count;
    double timeout;
    int payload_size;
};

void* thread(void* arg);

void print_usage(int argc, char* argv[]) {
    const char* app = argc ? argv[0] : "ping";
    printf("Usage: %s <ip> [count = 3] [timeout (s) = 1.0] [payload size (bytes) = 16]\n", app);

    #define PRINT_USAGE_EXAMPLE(example) \
        printf("    %s " example "\n", app);

    printf("Examples:\n");

    PRINT_USAGE_EXAMPLE("192.168.1.1");
    PRINT_USAGE_EXAMPLE("192.168.1.1 10");
    PRINT_USAGE_EXAMPLE("192.168.1.1 10 0.7");
    PRINT_USAGE_EXAMPLE("192.168.1.1 10 0.7 128");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argc, argv);
        return EXIT_FAILURE;
    }

    int ip_count = 0;
    struct sockaddr_in dest[MAX_IP_COUNT];

    char* all_ip_str = argv[1];
    char* ip_str = strtok(all_ip_str, IP_DELIM);
    while (ip_count < MAX_IP_COUNT && ip_str != NULL) {
        int result = get_addr(&dest[ip_count], ip_str);
        if (result) {
            fprintf(stderr, "Can't parse address\n");
            return EXIT_FAILURE;
        }

        ip_str = strtok(NULL, IP_DELIM);
        ip_count++;
    }

    int count = DEFAULT_COUNT;
    {
        if (argc >= 3) {
            count = atoi(argv[2]);
        }

        if (count < 0) {
            fprintf(stderr, "Wrong count value\n");
            return EXIT_FAILURE;
        }
    }

    double timeout = DEFAULT_TIMEOUT;
    {
        if (argc >= 4) {
            timeout = atof(argv[3]);
        }

        if (timeout <= 0) {
            fprintf(stderr, "Wrong timeout value\n");
            return 1;
        }
    }

    int payload_size = DEFAULT_PAYLOAD_SIZE;
    {
        if (argc >= 5) {
            payload_size = atoi(argv[4]);
        }

        if (payload_size <= 0) {
            fprintf(stderr, "Wrong payload size value\n");
            return 1;
        }
    }

    random_init();

    pthread_t threads[MAX_IP_COUNT];
    struct ThreadData threads_data[MAX_IP_COUNT];
    for (int i = 0; i < ip_count; i++) {
        struct ThreadData* data = &threads_data[i];
        data->dest = dest[i];
        data->count = count;
        data->timeout = timeout;
        data->payload_size = payload_size;

        pthread_create(&threads[i], NULL, thread, (void*) data);
    }

    for (int i = 0; i < ip_count; i++) {
        pthread_join(threads[i], NULL);
    }

    return EXIT_SUCCESS;
}

void* thread(void* arg) {
    struct ThreadData* data = arg;

    ping(data->dest, data->count, data->timeout, data->payload_size);

    pthread_exit(NULL);
}
