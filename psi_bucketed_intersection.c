#include "psi_bucketed_intersection.h"

int psi_intersection_strcmp(gconstpointer a, gconstpointer b);
GSList * psi_intersection_unique_elems(GSList * l);

void psi_bucketed_intersection(PSI_INTERSECTION_CTX* ctx) {

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

    if (ctx->lookup)
        psi_intersection_lookup(ctx);
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
        psi_get_64bit_sha256(element, &bucket_n, ctx->element_size);
        bucket_n /= divisor;
        add_elem_to_queue(a, ctx, element, bucket_n);
    }
}

void psi_buckets_intersection(PSI_INTERSECTION_CTX* ctx) {
    FILE * f = psi_try_fopen(ctx->path_result, "w");
#pragma omp parallel for shared(ctx) num_threads(ctx->threads)
    for (size_t i = 0; i < ctx->bucket_n; i++) {
        char path_a[64];
        char path_b[64];
        snprintf(path_a, 64, "%sa%zu", ctx->path_folder_buckets, i);
        snprintf(path_b, 64, "%sb%zu", ctx->path_folder_buckets, i);

        PSI_SIMPLE_INTERSECTION_CTX simple_ctx[1];
        simple_ctx->result = NULL;
        strncpy(simple_ctx->path_a, path_a, 64);
        strncpy(simple_ctx->path_b, path_b, 64);

        simple_ctx->element_size = ctx->element_size;

        psi_simple_intersection(simple_ctx);

        if (simple_ctx->result != NULL)
#pragma omp critical
        {
            ctx->result = g_slist_concat(simple_ctx->result, ctx->result);
        }
    }

    if (g_slist_length(ctx->result) == 0)
        printf("No elements in intersection\n");
    else {
        ctx->result = psi_intersection_unique_elems(ctx->result);
        printf("%u unique elements in intersection\n", g_slist_length(ctx->result));
        printf("Writing to %s\n", ctx->path_result);
        g_slist_foreach(ctx->result, (GFunc) psi_intersection_elem_dump, f);
    }
    fclose(f);
}

void save_all_queues(PSI_INTERSECTION_CTX* ctx, gboolean a) {
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
    printf("Element size : %u\n", (uint) ctx->element_size);
    printf("Path A : %s\n", ctx->path_a);
    printf("Path B : %s\n", ctx->path_b);
    printf("Path folder buckets : %s\n", ctx->path_folder_buckets);
    printf("Path result : %s\n", ctx->path_result);
    printf("Queue buffer size : %zu\n", ctx->queue_buffer_size);
    printf("Read buffer size : %zu\n", ctx->read_buffer_size);
    printf("Number of threads : %u\n", (uint) ctx->threads);
    if (ctx->lookup) {
        printf("Lookup : Yes\n");
        printf("Path lookup : %s\n", ctx->path_lookup);
    } else
        printf("Lookup : No\n");
    printf("\n");
}

void psi_intersection_lookup(PSI_INTERSECTION_CTX* ctx) {
    if (ctx->protocol == PSI_NH)
        psi_intersection_lookup_nh(ctx);
    else
        psi_intersection_lookup_ot(ctx);
}

void psi_intersection_lookup_nh(PSI_INTERSECTION_CTX* ctx) {
    GSList * l = NULL;
    FILE * res = psi_try_fopen(ctx->path_result, "rb");
    FILE * lookup = psi_try_fopen(ctx->path_lookup, "rb");
    char tmp[140], lookup_buf[33];
    strncat(tmp, ctx->path_result, 120);
    strcat(tmp, "_true");
    printf("Writing true intersection elements to %s\n", tmp);
    FILE * true_res = psi_try_fopen(tmp, "wb");

    GHashTable * t = g_hash_table_new(g_str_hash, g_str_equal);
    uint8_t elem[ctx->element_size], hash[16];
    while (1) {
        char * c_buf = (char*) malloc(33 * sizeof*c_buf);
        if (fread(c_buf, 32, 1, res) < 1)
            break;
        fseeko(res, 1, SEEK_CUR);
        c_buf[32] = '\0';
        if (!hash_table_insert(t, c_buf))
            printf("Collision by inserting new element to hash table %s\n", c_buf);
    }

    while (fread(elem, ctx->element_size, 1, lookup) > 0) {
        get_16_byte_sha256(elem, hash, ctx->element_size);
        bytes_to_chars(hash, lookup_buf, 16);
        lookup_buf[32] = '\0';
        if (is_in_hash_table(t, lookup_buf)) {
            char * res = (char*) malloc((ctx->element_size * 2 + 1) * sizeof*res);
            bytes_to_chars(elem, res, ctx->element_size);
            l = psi_lookup_add_to_list(l, res, ctx->element_size);
        }
    }

    printf("Got %u true elements\n", g_slist_length(l));
    g_slist_foreach(l, (GFunc) psi_intersection_elem_dump, true_res);
}

void psi_intersection_lookup_ot(PSI_INTERSECTION_CTX* ctx) {
    size_t cuckoo_el_size = 19, masks_el_size = ctx->element_size, init_elem_size = 16;

    GSList *masks_l = NULL;
    //result masks
    FILE * res = psi_try_fopen(ctx->path_result, "r");
    FILE * lookup_cuckoo = psi_try_fopen(ctx->path_lookup_cuckoo, "rb");
    FILE * lookup_masks = psi_try_fopen(ctx->path_lookup_masks, "rb");

    char tmp[140], masks_buf[masks_el_size * 2 + 1];
    //char lookup_buf[init_elem_size * 2 + 1];
    tmp[0] = '\0';
    strncat(tmp, ctx->path_result, 120);
    strcat(tmp, "_true");
    printf("Writing true intersection elements to %s\n", tmp);
    FILE * true_res = psi_try_fopen(tmp, "wb");
    GHashTable * t = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
    //uint8_t elem[ctx->element_size];
    //uint8_t hash[16];

    GHashTable * masks_t = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
    uint8_t buf[masks_el_size];

    fseeko(res, 0, SEEK_SET);
    //add elements for looking up indexes
    while (1) {
        char * c_buf = (char*) malloc((masks_el_size * 2 + 1) * sizeof*c_buf);
        if (fread(buf, masks_el_size * 2, 1, res) < 1)
            break;
        memcpy(c_buf, buf, masks_el_size * 2);
        c_buf[masks_el_size * 2] = '\0';
        fseeko(res, 1, SEEK_CUR);
        if (strlen(c_buf) == masks_el_size * 2 && !hash_table_insert(masks_t, c_buf)) {
            printf("Collision occured by inserting new element to mask table %s\n", c_buf);
            for (size_t i = 0; i < masks_el_size * 2; i++)
                printf("%c", c_buf[i]);
            printf("\n");
        }
    }

    //find indexes and save according elements from cuckoo table
    while (1) {
        if (fread(buf, masks_el_size, 1, lookup_masks) < 1)
            break;
        bytes_to_chars(buf, masks_buf, masks_el_size);
        masks_buf[masks_el_size * 2] = '\0';

        if (is_in_hash_table(masks_t, masks_buf)) {
            uint64_t * r = malloc(sizeof*r);
            *r = (ftello(lookup_masks) - masks_el_size) / masks_el_size;
            masks_l = g_slist_prepend(masks_l, r);
        }
    }

    fclose(res);
    //overwriting result file with cuckoo table elements
    GSList * e = masks_l;
    uint8_t cuckoo_read_buf[init_elem_size];
    char cuckoo_write_buf[init_elem_size * 2 + 1];
    while (e != NULL) {
        fseeko(lookup_cuckoo, *((uint64_t*) (e->data)) * cuckoo_el_size, SEEK_SET);
        if (fread(cuckoo_read_buf, init_elem_size, 1, lookup_cuckoo) < 1)
            printf("Error reading cuckoo element from cuckoo table for lookup\n");
        bytes_to_chars(cuckoo_read_buf, cuckoo_write_buf, init_elem_size);
        cuckoo_write_buf[init_elem_size * 2] = '\n';
        if (fwrite(cuckoo_write_buf, init_elem_size * 2 + 1, 1, true_res) < 1)
            printf("Error writing cuckoo element to result file\n");
        e = e->next;
    }

    e = NULL;
    g_slist_free_full(masks_l, free);
    g_hash_table_destroy(masks_t);

    fclose(true_res);
    g_hash_table_destroy(t);
}

GSList * psi_lookup_add_to_list(GSList * l, char * elem, uint8_t e_size) {
    return g_slist_prepend(l, elem);
}

void psi_write_and_show(char * elem, FILE * f) {
    static char tmp[128];
    snprintf(tmp, 127, "%s\n", elem);
    if (fwrite(tmp, strlen(tmp), 1, f) < 1)
        printf("Error writing to result_true\n");
    printf("%s\n", elem);
}

GSList * psi_intersection_unique_elems(GSList * l) {
    GSList * res = NULL;
    GSList * i = l;
    while (i != NULL) {
        if (res == NULL) res = g_slist_prepend(res, i->data);
        else if (g_slist_find_custom(res, (char*) i->data, psi_intersection_strcmp));
        else
            res = g_slist_prepend(res, i->data);
        i = i->next;
    }
    return res;
}

int psi_intersection_strcmp(gconstpointer a, gconstpointer b) {
    return strcmp((char*) a, (char*) b);
}

GSList * g_slist_dump(GSList * l, FILE * f) {
    g_slist_foreach(l, (GFunc) psi_intersection_elem_dump, (gpointer) f);
    g_slist_free_full(l, free);
    return NULL;
}

void psi_intersection_elem_dump(char * elem, FILE * f) {
    static char tmp[128];
    snprintf(tmp, 127, "%s\n", elem);
    if (fwrite(tmp, strlen(tmp), 1, f) < 1)
        printf("Error writing to result_true\n");
}
