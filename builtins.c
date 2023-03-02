#include "builtins.h"
#include <stdio.h>

int _print(int val) {
    return printf("%d ", val);
}

int _println(int val) {
    return printf("%d\n", val);
}