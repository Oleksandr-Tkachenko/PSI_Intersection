#include "psi_bucketed_intersection.h"

void psi_bucketed_intersection(PSI_INTERSECTION_CTX* ctx) {
    snprintf(ctx->path_result, 128, "%sres_intersection", ctx->path_root);

    show_settings(ctx);
    //ctx->buffer = slice_alloc_byte_buffer_new(ctx->read_buffer_size, ctx->element_size);
    ctx->buffer = (uint8_t **) malloc(sizeof (*ctx->buffer) * ctx->read_buffer_size);
    for (size_t i = 0; i < ctx->read_buffer_size; i++)
        ctx->buffer[i] = g_slice_alloc(ctx->element_size);

    ctx->queues = (PSI_Queue**) malloc(sizeof (PSI_Queue*) * ctx->bucket_n);

    for (size_t i = 0; i < ctx->bucket_n; i++) {
        ctx->queues[i] = g_slice_alloc(sizeof (PSI_Queue));
        ctx->queues[i]->buffer = (uint8_t**) malloc(sizeof (uint8_t*) * ctx->queue_buffer_size);
        for (size_t j = 0; j < ctx->queue_buffer_size; j++)
            ctx->queues[i]->buffer[j] = g_slice_alloc(ctx->element_size);
    }

    psi_split_into_buckets(ctx);
    psi_buckets_intersection(ctx);
    psi_destroy_buckets(ctx);

    for (size_t i = 0; i < ctx->bucket_n; i++) {
        for (size_t j = 0; j < ctx->queue_buffer_size; j++)
            g_slice_free1(ctx->element_size, ctx->queues[i]->buffer[j]);
        free(ctx->queues[i]->buffer);
        g_slice_free1(sizeof (PSI_Queue), ctx->queues[i]);
    }
    free(ctx->queues);
    slice_free_byte_buffer(&(ctx->buffer), ctx->read_buffer_size, ctx->element_size);
}

void psi_split_into_buckets(PSI_INTERSECTION_CTX* ctx) {
    psi_mkdir(ctx->path_folder_buckets);
    init_queues(ctx);
    psi_split_single(TRUE, ctx);
    save_all_queues(ctx, TRUE);
    init_queues(ctx);
    psi_split_single(FALSE, ctx);
    save_all_queues(ctx, FALSE);
}

void psi_split_single(gboolean a, PSI_INTERSECTION_CTX* ctx) {
    FILE * f;
    char path[128];
    uint8_t element[ctx->element_size];
    off_t size;
    uint64_t bucket_n, divisor;

    divisor = (0xFFFFFFFFFFFFFFFF / ctx->bucket_n);

    if (a) {
        snprintf(path, 128, "%s", ctx->path_a);
    } else {
        snprintf(path, 128, "%s", ctx->path_b);
    }
    size = fsize(path);
    f = fopen(path, "rb");
    if (f == NULL) {
        printf("Error opening file for reading elements\n");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < size / ctx->element_size; i++) {
        if (fread(element, ctx->element_size, 1, f) < 1)
            printf("Error parsing elements\n");
        psi_get_64bit_sha256(element, &bucket_n);
        bucket_n /= divisor;
        add_elem_to_queue(a, ctx, element, bucket_n);
    }
}

void psi_buckets_intersection(PSI_INTERSECTION_CTX* ctx) {

#pragma omp parallel for shared(ctx) num_threads(ctx->threads)
    for (size_t i = 0; i < ctx->bucket_n; i++) {
        char path_a[64];
        char path_b[64];
        snprintf(path_a, 64, "%sa%zu", ctx->path_folder_buckets, i);
        snprintf(path_b, 64, "%sb%zu", ctx->path_folder_buckets, i);
        //snprintf(path_a, 64, "%s", ctx->path_folder_buckets);
        //snprintf(path_b, 64, "%s", ctx->path_folder_buckets);

        PSI_SIMPLE_INTERSECTION_CTX simple_ctx[1];
        simple_ctx->result = NULL;
        strncpy(simple_ctx->path_a, path_a, 64);
        strncpy(simple_ctx->path_b, path_b, 64);
        
        simple_ctx->element_size = ctx->element_size;

        psi_simple_intersection(simple_ctx);

        if (simple_ctx->result != NULL)
#pragma omp critical
        {
            ctx->result = g_slist_concat(ctx->result, simple_ctx->result);
        }
    }

    if (g_slist_length(ctx->result) == 0)
        printf("No elements in intersection\n");
    else {
        printf("%d elements in intersection\n", g_slist_length(ctx->result));
        FILE * f = fopen(ctx->path_result, "w");
        if (f == NULL)
            printf("Error writing result to file\n");
        g_slist_foreach(ctx->result, (GFunc) print_result, f);
        fclose(f);
    }

}

void save_all_queues(PSI_INTERSECTION_CTX* ctx, gboolean a) {
    //char buffer[128];
    for (size_t i = 0; i < ctx->bucket_n; i++)
        save_queue(a, ctx->queues[i], ctx->path_folder_buckets, i, ctx->element_size);
}

void init_queues(PSI_INTERSECTION_CTX* ctx) {
    for (size_t i = 0; i < ctx->bucket_n; i++)
        ctx->queues[i]->counter = 0;
}

void add_elem_to_queue(gboolean a, PSI_INTERSECTION_CTX* ctx, uint8_t element[ctx->element_size], uint64_t bucket_n) {
    if (bucket_n >= ctx->bucket_n) {
        printf("Wrong calculated bucket_n\n");
        exit(EXIT_FAILURE);
    }
    if (ctx->queues[bucket_n]->counter == ctx->queue_buffer_size)
        save_queue(a, ctx->queues[bucket_n], ctx->path_folder_buckets, bucket_n, ctx->element_size);
    if (ctx->queues[bucket_n]->counter > ctx->queue_buffer_size) {
        printf("Error. Queue counter is > than limit\n");
        exit(EXIT_FAILURE);
    }
    memcpy(ctx->queues[bucket_n]->buffer[ctx->queues[bucket_n]->counter], element, ctx->element_size);
    ctx->queues[bucket_n]->counter++;
}

void save_queue(gboolean a, PSI_Queue * q, char* path, uint64_t bucket_n, uint8_t es) {
    char bucket_path[64];
    if (a) snprintf(bucket_path, 64, "%sa%" PRIu64, path, bucket_n);
    else snprintf(bucket_path, 64, "%sb%" PRIu64, path, bucket_n);

    FILE * f_bucket = fopen(bucket_path, "ab");
    if (f_bucket == NULL) {
        printf("Error opening file for bucket\n");
        exit(EXIT_FAILURE);
    }

    for (uint i = 0; i < q->counter; i++)
        if (fwrite(q->buffer[i], es, 1, f_bucket) < 1)
            printf("Error saving queue\n");

    q->counter = 0;
    fclose(f_bucket);
}

void print_result(char* element, FILE* f) {
    printf("%s\n", element);
    fputs(element, f);
    fputc('\n', f);
}

void psi_destroy_buckets(PSI_INTERSECTION_CTX* ctx) {
    char buffer[128];
    snprintf(buffer, 128, "rm -rf \"%s\"", ctx->path_folder_buckets);
    system(buffer);
}

void show_settings(PSI_INTERSECTION_CTX* ctx) {
    printf("\n");
    printf("Number of buckets : %zu\n", ctx->bucket_n);
    printf("Element size : %u\n", ctx->element_size);
    printf("Path A : %s\n", ctx->path_a);
    printf("Path B : %s\n", ctx->path_b);
    printf("Path root : %s\n", ctx->path_root);
    printf("Path folder buckets : %s\n", ctx->path_folder_buckets);
    printf("Queue buffer size : %zu\n", ctx->queue_buffer_size);
    printf("Read buffer size : %zu\n", ctx->read_buffer_size);
    printf("Number of threads : %u\n", ctx->threads);
    printf("\n");
}