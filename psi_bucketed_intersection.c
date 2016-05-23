#include "psi_bucketed_intersection.h"

void psi_bucketed_intersection(PSI_INTERSECTION_CTX* ctx) {
    snprintf(ctx->path_result, 128, "%sresult", ctx->path_root);
    slice_alloc_byte_buffer(&(ctx->buffer), ctx->read_buffer_size, 16);
    ctx->queues = (PSI_Queue**) malloc(sizeof (PSI_Queue*) * ctx->bucket_n);
    for (size_t i = 0; i < ctx->bucket_n; i++) {
        ctx->queues[i] = g_slice_alloc(sizeof (PSI_Queue));
        ctx->queues[i]->buffer = (uint8_t**) malloc(sizeof (uint8_t*) * ctx->queue_buffer_size);
        for (size_t j = 0; j < ctx->queue_buffer_size; j++)
            ctx->queues[i]->buffer[j] = g_slice_alloc(16);
    }
    psi_split_into_buckets(ctx);
    psi_buckets_intersection(ctx);
    psi_destroy_buckets(ctx);
    
    for (size_t i = 0; i < ctx->bucket_n; i++) {
        for (size_t j = 0; j < ctx->queue_buffer_size; j++)
            g_slice_free1(16, ctx->queues[i]->buffer[j]);
        free(ctx->queues[i]->buffer);
        g_slice_free1(sizeof (PSI_Queue), ctx->queues[i]);
    }
    free(ctx->queues);
    slice_free_byte_buffer(&(ctx->buffer), ctx->read_buffer_size, 16);
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
    char prefix;
    char path[64];
    uint8_t element[16];
    off_t size;
    uint64_t bucket_n, divisor;

    divisor = (0xFFFFFFFFFFFFFFFF / ctx->bucket_n);

    if (a) {
        prefix = 'a';
        snprintf(path, 64, "%sA/%d", ctx->path_root, (int) pow(10, ctx->element_pow));
    } else {
        prefix = 'b';
        snprintf(path, 64, "%sB/%d", ctx->path_root, (int) pow(10, ctx->element_pow));
    }
    size = fsize(path);
    f = fopen(path, "rb");
    if (f == NULL) {
        printf("Error opening file for reading elements\n");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < size / 16; i++) {
        if (fread(element, 16, 1, f) < 1)
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

static void save_all_queues(PSI_INTERSECTION_CTX* ctx, gboolean a) {
    char buffer[128];
    for (size_t i = 0; i < ctx->bucket_n; i++) {
        //get_bucket_path(a, buffer, ctx->path_folder_buckets, (uint16_t) i);
        //save_queue(a, ctx->queues[i], buffer, i);
        save_queue(a, ctx->queues[i], ctx->path_folder_buckets, i);
    }
}

void init_queues(PSI_INTERSECTION_CTX* ctx) {
    for (size_t i = 0; i < ctx->bucket_n; i++)
        ctx->queues[i]->counter = 0;
}

void add_elem_to_queue(gboolean a, PSI_INTERSECTION_CTX* ctx, uint8_t element[16], uint64_t bucket_n) {
    if (bucket_n >= ctx->bucket_n) {
        printf("Wrong calculated bucket_n\n");
        exit(EXIT_FAILURE);
    }
    if (ctx->queues[bucket_n]->counter == ctx->queue_buffer_size)
        save_queue(a, ctx->queues[bucket_n], ctx->path_folder_buckets, bucket_n);
    if (ctx->queues[bucket_n]->counter > ctx->queue_buffer_size) {
        printf("Error. Queue counter is > than limit\n");
        exit(EXIT_FAILURE);
    }
    memcpy(ctx->queues[bucket_n]->buffer[ctx->queues[bucket_n]->counter], element, 16);
    ctx->queues[bucket_n]->counter++;
}

void save_queue(gboolean a, PSI_Queue * q, char* path, uint64_t bucket_n) {
    char bucket_path[64];
    if (a) snprintf(bucket_path, 64, "%sa%" PRIu64, path, bucket_n);
    else snprintf(bucket_path, 64, "%sb%" PRIu64, path, bucket_n);

    FILE * f_bucket = fopen(bucket_path, "ab");
    if (f_bucket == NULL) {
        printf("Error opening file for bucket\n");
        exit(EXIT_FAILURE);
    }

    for (uint i = 0; i < q->counter; i++)
        if (fwrite(q->buffer[i], 16, 1, f_bucket) < 1)
            printf("Error saving queue\n");

    q->counter = 0;
    fclose(f_bucket);
}

void print_result(char* element, FILE* f) {
    printf("%s\n", element);
    fputs(element, f);
    fputc('\n', f);
}

static void psi_destroy_buckets(PSI_INTERSECTION_CTX* ctx){
    char buffer[128];
    snprintf(buffer, 128, "rm -rf \"%s\"", ctx->path_folder_buckets);
    system(buffer);
}