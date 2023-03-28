#ifndef clox_lines_h
#define clox_lines_h

typedef struct {
  int count;
  int capacity;
  int* lines;
  int* lengths;
} Lines;

void initLines(Lines* lines);
void writeLines(Lines* lines, int line);
void freeLines(Lines* lines);
int getLineAtOffset(Lines* lines, int offset);

#endif