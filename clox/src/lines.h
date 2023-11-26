#ifndef clox_lines_h
#define clox_lines_h

typedef struct {
  int count;
  int capacity;
  int* lines;
  int* lengths;
} Lines;

void InitLines(Lines* lines);
void WriteLines(Lines* lines, int line);
void FreeLines(Lines* lines);
int GetLineAtOffset(Lines* lines, int offset);

#endif