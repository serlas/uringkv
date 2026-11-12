void metrics_record(metrics_t *m, struct timespec start, struct timespec end) {
    long ns = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
    m->latencies[m->count % 1000] = ns;
    m->count++;
    m->throughput = m->ops / m->duration;
}

void metrics_print(metrics_t *m) {
    printf("p50: %ld ns\n", m->latencies[500]);
}