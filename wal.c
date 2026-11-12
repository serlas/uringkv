#include "wal.h"
int wal_append(struct uringkv *kv, const char *entry, size_t len) {
    int crc = crc32(0, (Bytef*)entry, len - 4);
    memcpy((char*)entry + len - 4, &crc, 4);
    struct io_uring_sqe *sqe = io_uring_get_sqe(&kv->ring);
    struct iovec iov = { .iov_base = (void*)entry, .iov_len = len };
    io_uring_prep_writev(sqe, 0, &iov, 1, lseek(kv->wal_fd, 0, SEEK_END));
    io_uring_submit(&kv->ring);
    if (++kv->metrics.fsync_count % GROUP_COMMIT_COUNT == 0) fdatasync(kv->wal_fd);
    return 0;
}

int wal_replay(struct uringkv *kv) {
    // Read WAL, validate CRC, apply to index/SST
    // Batch readv
}