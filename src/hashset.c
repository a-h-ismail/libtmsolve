/*
Copyright (C) 2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "hashset.h"
#include <stdlib.h>
#include <string.h>

int _str_compare(const void *a, const void *b, void *udata)
{
    const char **va = a, **vb = b;
    return strcmp(*va, *vb);
}

uint64_t _str_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const char **v = item;
    return hashmap_xxhash3(*v, strlen(*v), seed0, seed1);
}

hashset *hashset_new()
{
    return hashmap_new(sizeof(char **), 0, rand(), rand(), _str_hash, _str_compare, NULL, NULL);
}

const char *hashset_set(hashset *set, char *item)
{
    return hashmap_set(set, &item);
}

const char *hashset_remove(hashset *set, char *item)
{
    return hashmap_delete(set, &item);
}

const char *hashset_get(hashset *set, char *item)
{
    return hashmap_get(set, &item);
}

size_t hashset_count(hashset *set)
{
    return hashmap_count(set);
}

char **hashset_to_array(hashset *set, size_t *len, bool sort)
{
    return hashmap_to_array(set, len, sort);
}

void hashset_free(hashset *set)
{
    hashmap_free(set);
}
