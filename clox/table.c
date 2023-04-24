#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

// probe assumes the table is not full.
// key may be in the table.
static Entry *probe(Table *table, ObjString *key) {
    size_t i = key->hash % table->capacity;
    for (;;) {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL || ObjsEqual((Obj*) entry->key, (Obj*) key)) {
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

void Insert(Table *table, ObjString *key, Value value) {
    // Load factor alpha = 75%
    // At all times either capacity = 0 or count / capacity < 0.75.
    if (4 * (table->count + 1) >= 3 * table->capacity) {
        grow(table);
    }
    
    Entry *entry = probe(table, key);
    if (entry->key == NULL) {
        table->count++;
    }
    entry->key = key;
    entry->value = value;
}

Value *Get(Table *table, ObjString *key) {
    if (table->count == 0) {
       return NULL; 
    }
    
    Entry *entry = probe(table, key);
    if (entry->key == NULL) {
        return NULL;
    }
    return &entry->value;
}

void Delete(Table *table, ObjString *key) {
    if (table->count == 0) {
        return;
    }
    
    Entry *entry = probe(table, key);
    if (entry->key == NULL) {
        return;
    }
    
    // Index i is the index of the entry that will be erased or replaced.
    size_t i = (size_t) (entry - table->entries);
    size_t j = (i + 1) % table->capacity;
    for (;;) {
        Entry *entry = &table->entries[j];
        if (entry->key == NULL) {
            break;
        }
        
        if (i >= (entry->key->hash % table->capacity)) {
            table->entries[i].key = entry->key;
            table->entries[i].value = entry->value; 
            i = j;
        }
        
        j++;
    }
    table->entries[i].key = NULL; 
    table->entries[i].value = FromNil();
    table->count--;
}
