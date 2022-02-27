#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>

#include "hashcrack/hashcrack.h"

#define REPORT_FILE "report_non_blocking.txt"
#define MAX_HASHES_COUNT 10240
#define MAX_LENGTH 4

#define TAG_EXIT 1
#define TAG_HASH 2
#define TAG_ORIGINAL 3

int master(int argc, char* argv[]);
void slave(int rank);

void usage(int argc, char* argv[]) {
    fprintf(stderr, "%s <hashes file>\n", argc ? argv[0] : "hashcrack");
}

#pragma pack(push, 1)

struct Data {
    int num;
    MD5Hash hash;
};

struct Result {
    int num;
    char original[MAX_LENGTH];
};

#pragma pack(pop)

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

    struct Data data[MAX_HASHES_COUNT];
    for (int i = 0; i < count; i++) {
        data[i].hash = hashes[i];
        data[i].num = i;
    }

    MPI_Request requests_send[MAX_HASHES_COUNT];

    int done_count = 0;
    int sent_count = 0;
    int cracked[MAX_HASHES_COUNT] = {0};
    char originals[MAX_HASHES_COUNT][MAX_LENGTH+1];
    memset(originals, 0, MAX_HASHES_COUNT * (MAX_LENGTH + 1));

    int slave_busy[slave_count];
    memset(&slave_busy, 0, sizeof(int) * slave_count);

    MPI_Request request;
    struct Result hash_result;
    MPI_Irecv(&hash_result, sizeof(struct Result), MPI_BYTE, MPI_ANY_SOURCE, TAG_ORIGINAL, MPI_COMM_WORLD, &request);

    while (done_count < count) {
        if (sent_count < count) {
            int found = 0;
            for (int slave = 1; slave <= slave_count; slave++) {
                if (slave_busy[slave-1] < 2) {
                    int hash_num = sent_count;
                    MPI_Isend(&data[hash_num], sizeof(struct Data), MPI_BYTE, slave, TAG_HASH, MPI_COMM_WORLD, &requests_send[hash_num]);

                    printf("task %d -> slave %d %s\n", hash_num, slave, slave_busy[slave-1] ? "(async)" : "");

                    slave_busy[slave-1]++;
                    sent_count++;

                    found = 1;
                    break;
                }
            }

            if (found)
                continue;
        }

        printf("wait...\n");

        MPI_Status status;
        MPI_Wait(&request, &status);

        struct Result this_hash_result;
        memcpy(&this_hash_result, &hash_result, sizeof(struct Result));

        if (done_count != count - 1) {
            MPI_Irecv(&hash_result, sizeof(struct Result), MPI_BYTE, MPI_ANY_SOURCE, TAG_ORIGINAL, MPI_COMM_WORLD, &request);
        }

        printf("task %d <- slave %d\n", this_hash_result.num, status.MPI_SOURCE);

        memcpy(&originals[this_hash_result.num], hash_result.original, MAX_LENGTH);

        cracked[this_hash_result.num] = originals[this_hash_result.num][0] != 0;
        slave_busy[status.MPI_SOURCE-1]--;
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
    struct Data next_data;
    MPI_Status status;
    MPI_Request request;
    MPI_Irecv(&next_data, sizeof(struct Data), MPI_BYTE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &request);

    int sent = 0;
    MPI_Request send_request;
    MPI_Status send_status;
    struct Result hash_result;

    while (1) {
        MPI_Wait(&request, &status);

        if (status.MPI_TAG == TAG_EXIT)
            break;

        struct Data data = next_data;

        MPI_Irecv(&next_data, sizeof(struct Data), MPI_BYTE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &request);

        char value[MAX_LENGTH];
        int result = crack(data.hash, value, MAX_LENGTH);
        if (!result) {
            memset(value, 0, MAX_LENGTH);
        }

        if (sent) {
            MPI_Wait(&send_request, &send_status);
        }

        hash_result.num = data.num;
        memcpy(hash_result.original, value, MAX_LENGTH);

        MPI_Isend(&hash_result, sizeof(struct Result), MPI_BYTE, 0, TAG_ORIGINAL, MPI_COMM_WORLD, &send_request);

        sent = 1;
    }
}
