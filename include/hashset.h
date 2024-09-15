/*
Copyright (C) 2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef HASHSET_HASHMAP
#define HASHSET_HASHMAP
#include "hashmap.h"

typedef struct hashmap hashset;

hashset *hashset_new();
const char *hashset_set(hashset *set, char *item);
const char *hashset_remove(hashset *set, char *item);
const char *hashset_get(hashset *set, char *item);
char **hashset_to_array(hashset *set, size_t *len, bool sort);
void hashset_free(hashset *set);

#endif