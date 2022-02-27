#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>

#include "hashcrack/hashcrack.h"

#define REPORT_FILE "report_blocking.txt"
#define MAX_HASHES_COUNT 128
#define MAX_LENGTH 4

#define TAG_EXIT 1
#define TAG_HASH_INFO 2
#define TAG_HASH 3
#define TAG_ORIGINAL_INFO 4
#define TAG_ORIGINAL 5

int master(int argc, char* argv[]);
void slave(int rank);

void usage(int argc, char* argv[]) {
    fprintf(stderr, "%s <hashes file>\n", argc ? argv[0] : "hashcrack");
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int result = EXIT_SUCCESS;
    if (rank == 0) {
        result = master(argc, argv);
    } else {
        slave(rank);
    }

    MPI_Finalize();

    return result;
}

int master(int argc, char* argv[]) {
    int slave_count;
    MPI_Comm_size(MPI_COMM_WORLD, &slave_count);
    slave_count -= 1;

    if (argc != 2) {
        usage(argc, argv);

        for (int i = 1; i <= slave_count; i++) {
            MPI_Send(NULL, 0, MPI_BYTE, i, TAG_EXIT, MPI_COMM_WORLD);
        }

        return EXIT_FAILURE;
    }

    FILE* input = fopen(argv[1], "r");
    if (!input) {
        fprintf(stderr, "Can't open file %s\n", argv[1]);

        for (int i = 1; i < slave_count; i++) {
            MPI_Send(NULL, 0, MPI_BYTE, i, TAG_EXIT, MPI_COMM_WORLD);
        }

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

    int done_count = 0;
    int sent_count = 0;
    int cracked[MAX_HASHES_COUNT] = {0};
    char originals[MAX_HASHES_COUNT][MAX_LENGTH+1];
    memset(originals, 0, MAX_HASHES_COUNT * (MAX_LENGTH + 1));

    int slave_busy[slave_count];
    memset(&slave_busy, 0, sizeof(int) * slave_count);

    while (done_count < count) {
        if (sent_count < count) {
            int found = 0;
            for (int slave = 1; slave <= slave_count; slave++) {
                if (!slave_busy[slave-1]) {
                    int hash_num = sent_count;
                    MPI_Send(&hash_num, 1, MPI_INT, slave, TAG_HASH_INFO, MPI_COMM_WORLD);
                    MPI_Send(&hashes[sent_count], sizeof(MD5Hash), MPI_BYTE, slave, TAG_HASH, MPI_COMM_WORLD);

                    printf("task %d -> slave %d\n", hash_num, slave);

                    slave_busy[slave-1] = 1;
                    sent_count++;

                    found = 1;
                    break;
                }
            }

            if (found)
                continue;
        }


        printf("wait...\n");

        int hash_num;
        MPI_Status status;
        MPI_Recv(&hash_num, 1, MPI_INT, MPI_ANY_SOURCE, TAG_ORIGINAL_INFO, MPI_COMM_WORLD, &status);
        MPI_Recv(originals[hash_num], sizeof(MAX_LENGTH), MPI_CHAR, status.MPI_SOURCE, TAG_ORIGINAL, MPI_COMM_WORLD, &status);

        printf("task %d <- slave %d\n", hash_num, status.MPI_SOURCE);

        cracked[hash_num] = originals[hash_num][0] != 0;
        slave_busy[status.MPI_SOURCE-1] = 0;
        done_count++;
    }

    for (int i = 1; i <= slave_count; i++) {
        MPI_Send(NULL, 0, MPI_BYTE, i, TAG_EXIT, MPI_COMM_WORLD);
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

        fprintf(output, "%s %s\n", hash_str, cracked[i] ? originals[i] : "-");
    }

    fclose(output);

    return EXIT_SUCCESS;
}

void slave(int rank) {
    while (1) {
        int hash_num;
        MD5Hash hash;

        MPI_Status status;
        MPI_Recv(&hash_num, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (status.MPI_TAG == TAG_EXIT) {
            break;
        }

        if (status.MPI_TAG != TAG_HASH_INFO) {
            fprintf(stderr, "%d: got message with unexcepted tag; ignored\n", rank);
            continue;
        }

        MPI_Recv(&hash, sizeof(MD5Hash), MPI_BYTE, 0, TAG_HASH, MPI_COMM_WORLD, &status);

        char value[MAX_LENGTH];
        int result = crack(hash, value, MAX_LENGTH);
        if (!result) {
            memset(value, 0, MAX_LENGTH);
        }

        MPI_Send(&hash_num, 1, MPI_INT, 0, TAG_ORIGINAL_INFO, MPI_COMM_WORLD);
        MPI_Send(value, MAX_LENGTH, MPI_CHAR, 0, TAG_ORIGINAL, MPI_COMM_WORLD);
    }
}
