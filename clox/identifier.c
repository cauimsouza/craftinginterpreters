#include "identifier.h"
#include "memory.h"

static size_t addIdentifier(Identifiers *ids, ObjString *id) {
    if (ids->capacity == ids->count) {
        ids->capacity = GROW_CAPACITY(ids->capacity);
        ids->identifiers = GROW_ARRAY(ObjString*, ids->identifiers, ids->count, ids->capacity);
    }
    
    ids->identifiers[ids->count] = id;
    return ids->count++;
}

void InitIdentifiers(Identifiers *ids) {
    ids->count = 0;
    ids->capacity = 0;
    ids->identifiers = NULL;
}

void FreeIdentifiers(Identifiers *ids) {
    FREE_ARRAY(ObjString*, ids->identifiers, ids->capacity);
    InitIdentifiers(ids);
}

size_t AddIdentifier(Identifiers *ids, ObjString *id) {
    for (size_t i = 0; i < ids->capacity; i++) {
        if (ids->identifiers[i] == id) {
            return i;
        }
    }
    
    return addIdentifier(ids, id);
}
