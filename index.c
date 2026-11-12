void *index_init(struct uringkv *kv) {
    int fd = open(strcat(kv->path, "/index"), O_CREAT | O_RDWR, 0600);
    ftruncate(fd, 1 << 20);
    kv->index_map = mmap(NULL, 1 << 20, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    kv->index_size = 1 << 20;
    // Hash table impl (simple chained)
}

void index_put(void *map, const char *key, off_t offset) {
    // Hash key, store offset
}