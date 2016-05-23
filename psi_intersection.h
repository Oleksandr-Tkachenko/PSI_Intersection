/* 
 * File:   psi_intersection.h
 * Author: Oleksandr Tkachenko <oleksandr.tkachenko at crisp-da.de>
 *
 * Created on May 5, 2016, 2:05 PM
 */

#ifndef PSI_INTERSECTION_H
#define PSI_INTERSECTION_H

#include <glib-2.0/glib.h>


gboolean is_in_hash_table(GHashTable * t, char * element);
gboolean hash_table_insert(GHashTable * t, char * element);

#ifdef __cplusplus
extern "C" {
#endif




#ifdef __cplusplus
}
#endif

#endif /* PSI_INTERSECTION_H */

