#include "lines.h"
#include "memory.h"

void InitLines(Lines* lines) {
    lines->capacity = 0;
    lines->count = 0;
    lines->lines = NULL;
    lines->lengths = NULL;
}

void WriteLines(Lines* lines, int line) {
    if (lines->count > 0 && lines->lines[lines->count - 1] == line) {
        lines->lengths[lines->count - 1]++;
        return;
    }
    
    if (lines->count == lines->capacity) {
        lines->capacity = GROW_CAPACITY(lines->capacity);
        lines->lines = GROW_ARRAY(int, lines->lines, lines->count, lines->capacity);
        lines->lengths = GROW_ARRAY(int, lines->lengths, lines->count, lines->capacity);
    }
    
    lines->lines[lines->count] = line;
    lines->lengths[lines->count] = 1;
    lines->count++;
}

void FreeLines(Lines* lines) {
    FREE_ARRAY(int, lines->lines, lines->capacity);
    FREE_ARRAY(int, lines->lengths, lines->capacity);
    InitLines(lines);
}

int GetLineAtOffset(Lines* lines, int offset) {
    if (offset < 0) {
        return -1;
    }
    
    int index = 0;
    int i = 0;
    for (; i < lines->count && index + lines->lengths[i] <= offset; i++) {
        index += lines->lengths[i];
    }
    
    if (i == lines->count) {
        return -1;
    }
    
    return lines->lines[i];
    
}