#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "native.h"
#include "value.h"
#include "object.h"

ValueOpt Rand(int argc, Value *argv) {
    return (ValueOpt) {
        .value = FromDouble(rand()),
        .error = false
    };
}

ValueOpt Clock(int argc, Value *argv) {
    return (ValueOpt) {
        .value = FromDouble((double) clock() / CLOCKS_PER_SEC),
        .error = false
    };
}

ValueOpt Sqrt(int argc, Value *argv) {
    Value arg = argv[0];
    if (!IsNumber(arg) || arg.as.number < 0) {
        return (ValueOpt) {
            .error = true
        };
    }
    
    return (ValueOpt) {
        .value = FromDouble(sqrt(arg.as.number)),
        .error = false
    };
}

ValueOpt Len(int argc, Value *argv) {
    Value arg = argv[0];
    if (!IsString(arg)) {
        return (ValueOpt) {
            .error = true
        };
    }
    return (ValueOpt) {
        .value = FromDouble(((ObjString*) arg.as.obj)->length),
        .error = false
    };
}