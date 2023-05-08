#ifndef clox_identifier_h
#define clox_identifier_h

#include "object.h"

typedef struct {
    size_t count;
    size_t capacity;
    ObjString **identifiers;
} Identifiers;

void InitIdentifiers(Identifiers *ids);

void FreeIdentifiers(Identifiers *ids);

size_t AddIdentifier(Identifiers *ids, ObjString *id);

#endif // clox_identifier_h