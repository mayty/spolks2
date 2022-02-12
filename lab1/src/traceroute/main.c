#include "traceroute.h"
#include <shared/network.h>

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

void print_usage(int argc, char* argv[]) {
    const char* app = argc ? argv[0] : "traceroute";
    printf("Usage: %s <ip>\n", app);

    #define PRINT_USAGE_EXAMPLE(example) \
        printf("    %s " example "\n", app);

    printf("Examples:\n");

    PRINT_USAGE_EXAMPLE("192.168.1.1");
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        print_usage(argc, argv);
        return EXIT_FAILURE;
    }

    struct sockaddr_in dest;
    int result = get_addr(&dest, argv[1]);
    if (result) {
        fprintf(stderr, "Can't parse address\n");
        return EXIT_FAILURE;
    }

    traceroute(dest);

    return EXIT_SUCCESS;
}
