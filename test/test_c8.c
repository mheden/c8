#include "unity/unity.h"

#include <string.h>
/* we want the access internal structures */
#include "../src/c8.c"

#define ARRAY_SIZE(x)       ((sizeof x) / (sizeof *x))


int main (int argc, char ** argv)
{
    (void)argc;
    (void)argv;

    UnityBegin("c8 test suite");
    UnityEnd();

    return 0;
}
