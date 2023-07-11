#include <stdlib.h>

#include "native.h"
#include "time.h"
#include "value.h"

Value Rand(int argc, Value *argv) {
    int result = rand();
    return FromDouble(result);
}

Value Clock(int argc, Value *argv) {
    return FromDouble((double) clock() / CLOCKS_PER_SEC);
}