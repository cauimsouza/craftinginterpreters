#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TOMBSTONE_VAL FromBoolean(true)

inline static bool isTombstone(Entry *entry) {
    return entry->key == NULL && IsBoolean(entry->value) && entry->value.as.boolean;
}

//  Assumes the table is not full and is non-empty.
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
        // GROW_ARRAY() might trigger a garbage collection, which will try (if
        // this is the 'strings' table) try to free unmarked objects in this
        // table. The garbage collector reads the 'capacity' field to traverse
        // the entries in this table. Therefore, we should only update the
        // 'capacity' field *after* the call to GROW_ARRAY().
        size_t new_cap = GROW_CAPACITY(0);
        table->entries = GROW_ARRAY(Entry, table->entries, 0, new_cap);
        table->capacity = new_cap;
        for (size_t i = 0; i < table->capacity; i++) {
            table->entries[i].key = NULL;     
            table->entries[i].value = FromNil();
        }
        return;
    }
    
    size_t cur_cap = table->capacity;
    size_t new_cap = GROW_CAPACITY(cur_cap);
    
    Entry *cur_entries = table->entries;
    table->entries = ALLOCATE(Entry, new_cap);
    for (size_t i = 0; i < new_cap; i++) {
        table->entries[i].key = NULL;     
        table->entries[i].value = FromNil();
    }
    
    table->capacity = new_cap;
    
    table->count = 0; // We recalculate count as part of the moves below
    for (size_t i = 0; i < cur_cap; i++) {
        Entry *entry = &cur_entries[i];
        if (entry->key == NULL) {
            continue;
        }
        
        Insert(table, entry->key, entry->value);
        
        // Insert increments the refcounts of the key and of the value, so we have to decrement them
        DecrementRefcountObject((Obj*) entry->key);
        DecrementRefcountValue(entry->value);
    }
    
    FREE_ARRAY(Entry, cur_entries, cur_cap);
}

void InitTable(Table *table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void FreeTable(Table *table) {
    for (size_t i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL) {
            continue;
        }
        
        DecrementRefcountObject((Obj*) entry->key);
        DecrementRefcountValue(entry->value);
    }
    
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
    bool new_entry = entry->key == NULL;
    if (new_entry) {
        IncrementRefcountObject((Obj*) key);
        if (!isTombstone(entry)) table->count++;
    } else {
        DecrementRefcountValue(entry->value);
    }
    entry->key = key;
    entry->value = value; IncrementRefcountValue(value);
    
    return new_entry;
}

bool Get(Table *table, ObjString *key, Value *value) {
    if (table->count == 0) {
        return false;     
    }
    
    Entry *entry = probe(table, key);
    if (entry->key == NULL) {
        return false;
    }
    *value = entry->value;
    return true;
}

void Remove(Table *table, ObjString *key) {
    if (table->count == 0) {
        return;
    }
    
    Entry *entry = probe(table, key);
    if (entry->key == NULL) {
        return;
    }
    
    Value v = entry->value;
    entry->key = NULL;
    entry->value = TOMBSTONE_VAL;
    
    DecrementRefcountObject((Obj*) key);
    DecrementRefcountValue(v);
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

void MarkTable(Table *table) {
    for (size_t i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL) {
            continue;
        }
        
        MarkObj((Obj*) entry->key);
        MarkValue(entry->value);
    }
}

void RemoveUnmarkedKeys(Table *table) {
    for (size_t i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        if (entry->key != NULL && !entry->key->obj.marked) {
            entry->key = NULL;
            entry->value = TOMBSTONE_VAL;
        }
    }
}