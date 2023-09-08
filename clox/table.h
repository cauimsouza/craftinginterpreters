#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

typedef struct {
  ObjString* key;
  Value value;
} Entry;

typedef struct {
  size_t count;
  size_t capacity;
  Entry* entries;
} Table;

void InitTable(Table *table);
void FreeTable(Table *table);

// Insert inserts an entry into the table.
//
// Returns true iff a new entry was added to the table.
bool Insert(Table *table, ObjString *key, Value value);

// Get retrieves an element from the table.
//
// If key is in the table, the associated value is stored in value and true is returned.
// If key is not in the table, value is unchanged and false is returned.
bool Get(Table *table, ObjString *key, Value *value);

// Returns the string interned in the table if it exists, NULL otherwise.
ObjString *Intern(Table *table, ObjString *key);

// GC related functions
void MarkTable(Table *table);
void RemoveUnmarkedKeys(Table *table);

#endif