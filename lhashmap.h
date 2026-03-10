#ifndef LHASHMAP_H
#define LHASHMAP_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef uint64_t (*hm_hash_fn)(const void*, size_t);
typedef int (*hm_eq_fn)(const void*, const void*, size_t);

typedef struct {
    void* key;
    size_t key_size;
    void* value;
    bool used;
} HashmapEntry;

typedef struct {
    HashmapEntry* data;
    size_t capacity;
    size_t next_capacity;
    size_t size;
    hm_eq_fn eq;
} Hashmap;

static inline int hm_default_eq(const void* a, const void* b, size_t n) {
    return memcmp(a, b, n) == 0;
}

static inline uint64_t hash_fnv1a(const void* data, size_t size) {
    const unsigned char* p = (const unsigned char*)data;
    uint64_t h = 1469598103934665603ULL;

    for (uint64_t i = 0; i < size; i++)
        h = (h ^ p[i]) * 1099511628211ULL;

    return h;
}

static inline void hm_init(Hashmap* hm, hm_eq_fn eq, uint32_t cap_factor) {
    if (!cap_factor) cap_factor = 1;

    hm->capacity = 0;
    hm->next_capacity = (size_t)1 << cap_factor;
    hm->size = 0;
    hm->eq = eq ? eq : hm_default_eq;
    hm->data = NULL;
}

static inline void hm_insert(Hashmap* hm, const void* key, size_t key_size, void* value) {
    if ((hm->size + 1) * 10 > hm->capacity * 7) {
        HashmapEntry* new_data = (HashmapEntry*)calloc(hm->next_capacity, sizeof(HashmapEntry));
        if (!new_data) return;

        hm->capacity = hm->next_capacity;
        hm->next_capacity = hm->capacity << 1;

        for (size_t i = 0; i < hm->capacity >> 1; i++)  {
            HashmapEntry* old_entry = &hm->data[i];
            if (!old_entry->used) continue;

            uint64_t new_key = hash_fnv1a(old_entry->key, old_entry->key_size) & (hm->capacity - 1);
            HashmapEntry* new_entry = &new_data[new_key];

            while (new_entry->used) {
                new_key = (new_key + 1) & (hm->capacity -1);
                new_entry = &new_data[new_key];
            };

            memcpy(new_entry, old_entry, sizeof(HashmapEntry));
        }
        free(hm->data);
        hm->data = new_data;
    }

    uint64_t hashed_key = hash_fnv1a(key, key_size) & (hm->capacity -1);
    while (hm->data[hashed_key].used) {

        if (hm->data[hashed_key].key_size == key_size &&
            hm->eq(hm->data[hashed_key].key, key, key_size)
        ) {
            hm->data[hashed_key].value = value;
            return;
        }
        
        hashed_key = (hashed_key + 1) & (hm->capacity - 1);
    }

    HashmapEntry* new_entry = &hm->data[hashed_key];

    new_entry->key = malloc(key_size);
    if (!new_entry->key) return; 

    memcpy(new_entry->key, key, key_size);
    new_entry->key_size = key_size;
    new_entry->value = value;
    new_entry->used = true;
    hm->size++;
}

static inline void* hm_get(Hashmap* hm, const void* key, size_t key_size) {
    if (!hm || !hm->data || hm->capacity == 0 || !key || key_size == 0) return NULL;

    uint64_t hashed_key = hash_fnv1a(key, key_size) & (hm->capacity -1);
    
    while (hm->data[hashed_key].used) {
        if (hm->data[hashed_key].key_size == key_size && hm->eq(key, hm->data[hashed_key].key, key_size)) {
            return hm->data[hashed_key].value;
        }

        hashed_key = (hashed_key + 1) & (hm->capacity -1);
    }

    return NULL;
}

static inline void hm_destroy(Hashmap* hm) {
    if (!hm || !hm->data) return;
    for (size_t i = 0; i < hm->capacity; i++) {
        if (hm->data[i].used) free(hm->data[i].key);
    }

    free(hm->data);
    hm->capacity = 0;
    hm->next_capacity = 0;
    hm->size = 0;
    hm->data = NULL;
    hm->eq = NULL;
}

#endif
