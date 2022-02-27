#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashcrack/hashcrack.h"

#define REPORT_FILE "report_single.txt"
#define MAX_HASHES_COUNT 128
#define MAX_LENGTH 4

void usage(int argc, char* argv[]) {
    fprintf(stderr, "%s <hashes file>\n", argc ? argv[0] : "hashcrack");
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        usage(argc, argv);
        return EXIT_FAILURE;
    }

    FILE* input = fopen(argv[1], "r");
    if (!input) {
        fprintf(stderr, "Can't open file %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    MD5Hash hashes[MAX_HASHES_COUNT];

    int count = 0;
    for (; count < MAX_HASHES_COUNT;) {
        char line[MD5_HEX_LEN*2];
        char* get_result = fgets(line, sizeof(line), input);
        if (get_result == NULL)
            break;

        MD5Hash hash;
        int invalid = md5_from_str(line, &hash);
        if (invalid) {
            continue;
        }

        hashes[count++] = hash;
    }

    fclose(input);

    int cracked[MAX_HASHES_COUNT] = {0};
    char originals[MAX_HASHES_COUNT][MAX_LENGTH+1];

    for (int i = 0; i < count; i++) {
        cracked[i] = crack(hashes[i], originals[i], MAX_LENGTH);
    }

    FILE* output = fopen(REPORT_FILE, "w");
    if (!output) {
        fprintf(stderr, "Can't save report to file "REPORT_FILE"\n");

        return EXIT_FAILURE;
    }

    for (int i = 0; i < count; i++) {
        char hash_str[MD5_HEX_LEN+1];
        md5_to_str(hash_str, &hashes[i]);
        hash_str[MD5_HEX_LEN] = '\0';

        originals[i][MAX_LENGTH] = '\0';

        fprintf(output, "%s %s\n", hash_str, cracked[i] ? originals[i] : "-");
    }

    fclose(output);

    return EXIT_SUCCESS;
}
