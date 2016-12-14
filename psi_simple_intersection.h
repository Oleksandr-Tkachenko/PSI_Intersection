/* 
 * File:   psi_simple_intersection.h
 * Author: Oleksandr Tkachenko <oleksandr.tkachenko at crisp-da.de>
 *
 * Created on May 5, 2016, 12:53 AM
 */

#ifndef PSI_SIMPLE_INTERSECTION_H
#define PSI_SIMPLE_INTERSECTION_H

#include <glib-2.0/glib.h>

#include "psi_structures.h"
#include "psi_misc.h"
#include "psi_hashing.h"

#ifdef __cplusplus
extern "C" {
#endif

    void psi_simple_intersection(PSI_SIMPLE_INTERSECTION_CTX *ctx);
    void psi_simple_intersection_insert(GHashTable * t, FILE* f, char ** buffer, off_t size, uint8_t es);
    void psi_simple_intersection_check(GHashTable * t, FILE * f, GSList ** result, off_t size, uint8_t es);
    gboolean is_in_hash_table(GHashTable * t, char * element);
    gboolean hash_table_insert(GHashTable * t, char * element);
    char ** slice_alloc_char_buffer_new(size_t n1, size_t n2);
    gboolean is_empty(uint8_t * a, uint n);

#ifdef __cplusplus
}
#endif

#endif /* PSI_SIMPLE_INTERSECTION_H */

