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
        slice_alloc_char_buffer(&(ctx->buffer), ctx->fsize[0] / 16, 16 * 2 + 1);
        psi_simple_intersection_insert(t, a, ctx->buffer, ctx->fsize[0]);
        psi_simple_intersection_check(t, b, &(ctx->result), ctx->fsize[1]);
        slice_free_char_buffer(&(ctx->buffer), ctx->fsize[0] / 16, 16 * 2 + 1);
    } else {
        slice_alloc_char_buffer(&(ctx->buffer), ctx->fsize[1] / 16, 16 * 2 + 1);
        psi_simple_intersection_insert(t, b, ctx->buffer, ctx->fsize[1]);
        psi_simple_intersection_check(t, a, &(ctx->result), ctx->fsize[0]);
        slice_free_char_buffer(&(ctx->buffer), ctx->fsize[1] / 16, 16 * 2 + 1);
    }

    g_hash_table_destroy(t);

    fclose(a);
    fclose(b);
}

void psi_simple_intersection_insert(GHashTable * t, FILE* f, char ** buffer, off_t size) {
    uint8_t hash_buffer[16];
    for (size_t i = 0; i < size / 16; i++) {
        if (fread(hash_buffer, 16, 1, f) < 1)
            break;
        bytes_to_chars(hash_buffer, buffer[i], 16);
        if (!hash_table_insert(t, buffer[i]))
            printf("Collision by inserting new element to hash table %s\n", buffer[i]);
    }
}

void psi_simple_intersection_check(GHashTable * t, FILE * f, GSList ** result, off_t size) {
    uint8_t hash_buffer[16];
    char str_buffer[16 * 2 + 1];
    for (size_t i = 0; i < size / 16; i++) {
        if (fread(hash_buffer, 16, 1, f) < 1)
            break;
        bytes_to_chars(hash_buffer, str_buffer, 16);
        if (is_in_hash_table(t, str_buffer)) {
            char* element = (char*) malloc(sizeof (char) * (16 * 2 + 1));
            strcpy(element, str_buffer);
            *result = g_slist_append(*result, element);
        }
    }
}
