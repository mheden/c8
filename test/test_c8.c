#include "unity/unity.h"

#include <string.h>
/* we want the access internal structures */
#include "../src/c8.c"

#define ARRAY_SIZE(x) ((sizeof x) / (sizeof *x))

static void test_load()
{
    c8_t *ctx;
    uint8_t code[] = {
            0x64, 0xab, // LD V4, 0xab
            0x60, 0x51, // LD V0, 0x51
            0x6b, 0x12, // LD VB, 0x12
            0x6e, 0x36, // LD VE, 0x36
    };

    ctx = c8_create();
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(ERR_OK, c8_load(ctx, 32, code, sizeof(code)));
    TEST_ASSERT_EQUAL(ERR_OK, c8_load(ctx, 99, code, sizeof(code)));
    TEST_ASSERT_EQUAL(ERR_OK, c8_load(ctx, 0x234, code, sizeof(code)));
    TEST_ASSERT_EQUAL_MEMORY(code, &ctx->mem[32], sizeof(code));
    TEST_ASSERT_EQUAL_MEMORY(code, &ctx->mem[99], sizeof(code));
    TEST_ASSERT_EQUAL_MEMORY(code, &ctx->mem[0x234], sizeof(code));
    TEST_ASSERT_EQUAL(ERR_OUT_OF_MEM,
                      c8_load(ctx, MEM_SIZE - 2, code, sizeof(code)));
}


int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    UnityBegin("c8 test suite");
    RUN_TEST(test_load);
    UnityEnd();

    return 0;
}
