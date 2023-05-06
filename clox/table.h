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
void Insert(Table *table, ObjString *key, Value value);
Value *Get(Table *table, ObjString *key);
void Delete(Table *table, ObjString *key);

// Returns the string interned in the table if it exists, NULL otherwise.
ObjString *Intern(Table *table, ObjString *key);

#endif