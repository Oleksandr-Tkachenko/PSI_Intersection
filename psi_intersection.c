#include "psi_intersection.h"

gboolean is_in_hash_table(GHashTable * t, char * element){
    return g_hash_table_contains(t, element);
}

gboolean hash_table_insert(GHashTable * t, char * element){
    return g_hash_table_insert(t, element, NULL);
}