/* 
 * File:   psi_simple_intersection.h
 * Author: Oleksandr Tkachenko <oleksandr.tkachenko at crisp-da.de>
 *
 * Created on May 5, 2016, 12:53 AM
 */

#ifndef PSI_SIMPLE_INTERSECTION_H
#define PSI_SIMPLE_INTERSECTION_H

#include "psi_structures.h"
#include "psi_misc.h"
#include "psi_hashing.h"
#include "psi_intersection.h"

#ifdef __cplusplus
extern "C" {
#endif

void psi_simple_intersection(PSI_SIMPLE_INTERSECTION_CTX *ctx);
void psi_simple_intersection_insert(GHashTable * t, FILE* f, char ** buffer, off_t size);
void psi_simple_intersection_check(GHashTable * t, FILE * f, GSList ** result, off_t size);
   

#ifdef __cplusplus
}
#endif

#endif /* PSI_SIMPLE_INTERSECTION_H */

