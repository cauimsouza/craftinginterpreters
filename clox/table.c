#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

inline static bool isTombstone(Entry *entry) {
    return entry->key == NULL && IsBoolean(entry->value) && entry->value.as.boolean;
}

// probe assumes the table is not full.
// key may be in the table.
static Entry *probe(Table *table, ObjString *key) {
    Entry *tombstone = NULL;
    size_t i = key->hash % table->capacity;
    for (;;) {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL) {
            if (!isTombstone(entry)) {
                return tombstone ? tombstone : entry;
            }
            tombstone = tombstone ? tombstone : entry; 
        } else if (entry->key == key) {
            // This is possible because we're using string interning,
            // otherwise we would have to compare byte by byte.
            return entry;
        }
        
        i = (i + 1) % table->capacity;
    }
}

static void grow(Table *table) {
    if (table->capacity == 0) {
        table->capacity = GROW_CAPACITY(table->capacity);
        table->entries = ALLOCATE(Entry, table->capacity);
        for (size_t i = 0; i < table->capacity; i++) {
            table->entries[i].key = NULL;     
            table->entries[i].value = FromNil();
        }
        return;
    }
    
    size_t capacity = table->capacity;
    Entry *old_entries = table->entries;
    
    table->count = 0; // We recalculate count as part of the move below
    table->capacity = GROW_CAPACITY(table->capacity);
    table->entries = ALLOCATE(Entry, table->capacity);
    for (size_t i = 0; i < table->capacity; i++) {
        table->entries[i].key = NULL;     
        table->entries[i].value = FromNil();
    }
    
    for (size_t i = 0; i < capacity; i++) {
        Entry *entry = &old_entries[i];
        if (entry->key == NULL) {
            continue;
        }
        
        Insert(table, entry->key, entry->value);
    }
    
    FREE_ARRAY(Entry, old_entries, capacity);
}

void InitTable(Table *table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void FreeTable(Table *table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);    
    InitTable(table);
}

bool Insert(Table *table, ObjString *key, Value value) {
    // Load factor alpha = 75%
    // At all times either capacity = 0 or count / capacity < 0.75.
    if (4 * (table->count + 1) >= 3 * table->capacity) {
        grow(table);
    }
    
    Entry *entry = probe(table, key);
    bool res = entry->key == NULL;
    if (entry->key == NULL && !isTombstone(entry)) {
        table->count++;
    }
    entry->key = key;
    entry->value = value;
    return res;
}

bool Get(Table *table, ObjString *key, Value *value) {
    if (table->count == 0) {
       return NULL; 
    }
    
    Entry *entry = probe(table, key);
    if (entry->key == NULL) {
        return false;
    }
    *value = entry->value;
    return true;
}

void Delete(Table *table, ObjString *key) {
    if (table->count == 0) {
        return;
    }
    
    Entry *entry = probe(table, key);
    if (entry->key == NULL) {
        return;
    }
    entry->key = NULL;
    entry->value = FromBoolean(true);
}

ObjString *Intern(Table *table, ObjString *key) {
    if (table->capacity == 0) {
        return NULL;
    }
    
    size_t i = key->hash % table->capacity;    
    for (;;) {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL && !isTombstone(entry)) {
            return NULL;
        }
        if (entry->key != NULL &&
            entry->key->hash == key->hash &&
            entry->key->length == key->length &&
            memcmp(entry->key->chars, key->chars, key->length) == 0) {
            return entry->key; 
        }
        i = (i + 1) % table->capacity;
    }
}