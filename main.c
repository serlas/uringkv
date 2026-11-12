#include <liburing.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <zlib.h>
#include <pthread.h>
#include <getopt.h>

#include "wal.h"
#include "sst.h"
#include "index.h"
#include "metrics.h"

#define QUEUE_DEPTH_DEFAULT 256
#define SEGMENT_SIZE_DEFAULT (128LL * 1024 * 1024)
#define GROUP_COMMIT_COUNT 100

struct uringkv {
    struct io_uring ring;
    char *path;
    int queue_depth;
    long long segment_size;
    int wal_fd;
    void *index_map;
    size_t index_size;
    pthread_t compaction_thread;
    metrics_t metrics;
};

void usage() {
    printf("uringkv --path <dir> --queue-depth <N> --segment <sizeM>\n");
    printf("uringkv put <key> <value>\n");
    printf("uringkv get <key>\n");
    printf("uringkv del <key>\n");
    printf("uringkv scan <start_key> <end_key>\n");
    printf("uringkv bench --keys <N> --read <%%> --write <%%> --duration <s>\n");
    exit(1);
}

int main(int argc, char **argv) {
    struct uringkv kv = {0};
    kv.path = "/tmp/uringkv";
    kv.queue_depth = QUEUE_DEPTH_DEFAULT;
    kv.segment_size = SEGMENT_SIZE_DEFAULT;

    if (argc < 2) usage();

    if (strcmp(argv[1], "--path") == 0) {
        kv.path = argv[2];
        // Parse other args
        // Init ring
        io_uring_queue_init(kv.queue_depth, &kv.ring, IORING_SETUP_SQPOLL);
        // Register fixed files/buffers
        kv.wal_fd = open(strcat(kv.path, "/wal.000"), O_CREAT | O_RDWR | O_APPEND, 0600);
        io_uring_register_files(&kv.ring, &kv.wal_fd, 1);
        char *fixed_buf = malloc(4096 * 10);
        struct iovec iovs[10];
        for (int i = 0; i < 10; i++) iovs[i] = (struct iovec){.iov_base = fixed_buf + i*4096, .iov_len = 4096};
        io_uring_register_buffers(&kv.ring, iovs, 10);

        // Recovery: Replay WAL
        wal_replay(&kv);

        // Start compaction
        pthread_create(&kv.compaction_thread, NULL, compaction_thread, &kv);

        // Run as daemon
        while (1) sleep(1);
    } else if (strcmp(argv[1], "put") == 0) {
        put(&kv, argv[2], argv[3]);
    } else if (strcmp(argv[1], "get") == 0) {
        char *val = get(&kv, argv[2]);
        printf("%s\n", val);
        free(val);
    } else if (strcmp(argv[1], "del") == 0) {
        del(&kv, argv[2]);
    } else if (strcmp(argv[1], "scan") == 0) {
        scan(&kv, argv[2], argv[3]);
    } else if (strcmp(argv[1], "bench") == 0) {
        // Parse --keys, --read, --write, --duration
        // Run loop with random put/get, measure
        metrics_print(&kv.metrics);
    } else usage();

    // Cleanup
    explicit_bzero(kv.key, sizeof(kv.key)); // If AES
    io_uring_queue_exit(&kv.ring);
    return 0;
}