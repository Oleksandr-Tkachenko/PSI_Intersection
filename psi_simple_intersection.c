#include "psi_simple_intersection.h"

void psi_simple_intersection(PSI_SIMPLE_INTERSECTION_CTX *ctx) {

    ctx->fsize[0] = fsize(ctx->path_a);
    ctx->fsize[1] = fsize(ctx->path_b);

    if (ctx->fsize[0] == 0 || ctx->fsize[1] == 0) {
        ctx->result = NULL;
        return;
    }

    GHashTable * t = g_hash_table_new(g_str_hash, g_str_equal);

    FILE * a = fopen(ctx->path_a, "rb");
    FILE * b = fopen(ctx->path_b, "rb");
    if (a == NULL || b == NULL) {
        printf("Error opening bucket for simple intersection\n");
        exit(EXIT_FAILURE);
    }

    ctx->buffer = NULL;

    if (ctx->fsize[1] > ctx->fsize[0]) {
        size_t n1 = ctx->fsize[0] / ctx->element_size;
        size_t n2 = ctx->element_size * 2 + 1;
        ctx->buffer = slice_alloc_char_buffer_new(n1, n2);
        psi_simple_intersection_insert(t, a, ctx->buffer, ctx->fsize[0], ctx->element_size);
        psi_simple_intersection_check(t, b, &(ctx->result), ctx->fsize[1], ctx->element_size);
        slice_free_char_buffer(&(ctx->buffer), n1, n2);
    } else {
        size_t n1 = ctx->fsize[1] / ctx->element_size;
        size_t n2 = ctx->element_size * 2 + 1;
        ctx->buffer = slice_alloc_char_buffer_new(n1, n2);
        psi_simple_intersection_insert(t, b, ctx->buffer, ctx->fsize[1], ctx->element_size);
        psi_simple_intersection_check(t, a, &(ctx->result), ctx->fsize[0], ctx->element_size);
        slice_free_char_buffer(&(ctx->buffer), n1, n2);
    }
    g_hash_table_destroy(t);

    fclose(a);
    fclose(b);
}

void psi_simple_intersection_insert(GHashTable * t, FILE* f, char ** buffer, off_t size, uint8_t es) {
    uint8_t hash_buffer[es];
    for (size_t i = 0; i < size / es; i++) {
        if (fread(hash_buffer, es, 1, f) < 1)
            break;
        bytes_to_chars(hash_buffer, buffer[i], es);
        if (!is_empty(hash_buffer, es) && !hash_table_insert(t, buffer[i]))
            printf("Collision by inserting new element to hash table %s\n", buffer[i]);
    }
}

void psi_simple_intersection_check(GHashTable * t, FILE * f, GSList ** result, off_t size, uint8_t es) {
    uint8_t hash_buffer[es];
    char str_buffer[es * 2 + 1];
    for (size_t i = 0; i < size / es; i++) {
        if (fread(hash_buffer, es, 1, f) < 1)
            break;
        bytes_to_chars(hash_buffer, str_buffer, es);
        if (!is_empty(hash_buffer, es) && is_in_hash_table(t, str_buffer)) {
            char* element = (char*) malloc(sizeof (char) * (es * 2 + 1));
            strncpy(element, str_buffer, es * 2);
            element[es * 2] = '\0';
            *result = g_slist_append(*result, element);
        }
    }
}

gboolean is_in_hash_table(GHashTable * t, char * element) {
    return g_hash_table_contains(t, element);
}

gboolean hash_table_insert(GHashTable * t, char * element) {
    return g_hash_table_insert(t, element, NULL);
}

char ** slice_alloc_char_buffer_new(size_t n1, size_t n2) {
    char ** buffer = (char**) malloc(sizeof (*buffer) * n1);
    if (buffer == NULL)
        printf("Error by malloc\n");
    for (size_t i = 0; i < n1; i++)
        buffer[i] = g_slice_alloc(n2);
    return buffer;
}

gboolean is_empty(uint8_t * a, uint n) {
    for (size_t i = 0; i < n; i++)
        if (a[i] != 0)
            return FALSE;
    return TRUE;
}