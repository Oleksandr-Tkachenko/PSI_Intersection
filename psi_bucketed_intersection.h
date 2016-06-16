/* 
 * File:   psi_bucketed_intersection.h
 * Author: Oleksandr Tkachenko <oleksandr.tkachenko at crisp-da.de>
 *
 * Created on May 5, 2016, 12:53 AM
 */

#ifndef PSI_BUCKETED_INTERSECTION_H
#define PSI_BUCKETED_INTERSECTION_H

#include <stdio.h>
#include <inttypes.h>
#include <omp.h>
#include <math.h>

#include "psi_structures.h"
#include "psi_misc.h"
#include "psi_hashing.h"
#include "psi_simple_intersection.h"

#ifdef __cplusplus
extern "C" {
#endif

    void psi_bucketed_intersection(PSI_INTERSECTION_CTX* ctx);
    void psi_split_into_buckets(PSI_INTERSECTION_CTX* ctx);
    void psi_split_single(gboolean a, PSI_INTERSECTION_CTX* ctx);
    void psi_buckets_intersection(PSI_INTERSECTION_CTX* ctx);
    void init_queues(PSI_INTERSECTION_CTX* ctx);
    void add_elem_to_queue(gboolean a, PSI_INTERSECTION_CTX* ctx, uint8_t element[ctx->element_size], uint64_t n);
    void save_queue(gboolean a, PSI_Queue * q, char* path, uint64_t n, uint8_t es);
    void print_result(char* element, FILE*f);
    void save_all_queues(PSI_INTERSECTION_CTX* ctx, gboolean a);
    void psi_destroy_buckets(PSI_INTERSECTION_CTX* ctx);
    void show_settings(PSI_INTERSECTION_CTX* ctx);

#ifdef __cplusplus
}
#endif

#endif /* PSI_BUCKETED_INTERSECTION_H */

