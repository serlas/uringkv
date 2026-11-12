void *compaction_thread(void *arg) {
    struct uringkv *kv = arg;
    while (1) {
        if (lseek(kv->wal_fd, 0, SEEK_END) > kv->segment_size) {
            // Merge to new SST: Sort, write sorted file, update index
            // Use io_uring for reads/writes
            // Leveled or size-tiered (size-tiered here)
        }
        sleep(5);
    }
    return NULL;
}