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


static void test_op_invalid()
{
    c8_t *ctx;
    uint8_t code[] = {
            0x00,
            0x00,
    };

    ctx = c8_create();
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(ERR_OK, c8_load(ctx, 0, code, sizeof(code)));

    TEST_ASSERT_EQUAL(ERR_INVALID_OP, c8_step(ctx));
}


static void test_op_JP_addr()
{
    c8_t *ctx;
    uint8_t code[] = {
            0x60, 0x36, // 000: LD V0, 0x36
            0x16, 0x78, // 002: JP 0x678
            0x10, 0x04, // 004: JP 0x004
    };

    ctx = c8_create();
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(ERR_OK, c8_load(ctx, 0, code, sizeof(code)));

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));

    TEST_ASSERT_EQUAL(0x678, ctx->reg.pc);
    TEST_ASSERT_EQUAL_STRING("JP\t0x678", ctx->last.opstr);

    c8_set_pc(ctx, 4);
    TEST_ASSERT_EQUAL(4, ctx->reg.pc);
    TEST_ASSERT_EQUAL(ERR_INFINIT_LOOP, c8_step(ctx));
}


void test_op_skip()
{
    c8_t *ctx;
    uint8_t code[] = {
            0x62, 0x10, //  000: LD V2, 0x10
            0x32, 0xab, //  002: SE V2, 0xab
            0x32, 0x10, //  004: SE V2, 0x10
            0x00, 0x00, //  006: should not be executed
            0x42, 0xab, //  008: SNE V2, 0xab
            0x00, 0x00, //  010: should not be executed
            0x42, 0x10, //  012: SNE V2, 0x10

            0x62, 0x10, //  014: LD V2, 0x10
            0x63, 0xab, //  016: LD V3, 0xab
            0x52, 0x30, //  018: SE V2, V3
            0x63, 0x10, //  020: LD V3, 0x10
            0x52, 0x30, //  022: SE V2, V3
            0x00, 0x00, //  024: should not be executed
            0x63, 0xab, //  026: LD V3, 0xab
            0x92, 0x30, //  028: SNE V2, V3
            0x00, 0x00, //  030: should not be executed
            0x63, 0x10, //  032: LD V3, 0x10
            0x92, 0x30, //  034: SNE V2, V3
    };

    ctx = c8_create();
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(ERR_OK, c8_load(ctx, 0, code, sizeof(code)));

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(4, ctx->reg.pc);
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(8, ctx->reg.pc);
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(12, ctx->reg.pc);
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(14, ctx->reg.pc);

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(20, ctx->reg.pc);
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(26, ctx->reg.pc);
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(32, ctx->reg.pc);
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(34, ctx->reg.pc);
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(36, ctx->reg.pc);

    TEST_ASSERT_EQUAL_STRING("SNE\tV2,\tV3", ctx->last.opstr);
}

static void test_op_LD_Vx_byte()
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
    TEST_ASSERT_EQUAL(ERR_OK, c8_load(ctx, 0, code, sizeof(code)));

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));

    TEST_ASSERT_EQUAL(8, ctx->reg.pc);
    TEST_ASSERT_EQUAL_HEX8(0xab, ctx->reg.v[4]);
    TEST_ASSERT_EQUAL_HEX8(0x51, ctx->reg.v[0]);
    TEST_ASSERT_EQUAL_HEX8(0x12, ctx->reg.v[0xb]);
    TEST_ASSERT_EQUAL_HEX8(0x36, ctx->reg.v[0xe]);

    TEST_ASSERT_EQUAL_STRING("LD\tVE,\t0x36", ctx->last.opstr);
}


void test_op_ADD_Vx_byte()
{
    c8_t *ctx;
    uint8_t code[] = {
            0x64, 0x19, // LD V4, 0x19
            0x62, 0xf0, // LD V2, 0xf0
            0x74, 0x13, // ADD V4, 0x13
            0x72, 0x99, // ADD V2, 0x99
    };

    ctx = c8_create();
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(ERR_OK, c8_load(ctx, 0, code, sizeof(code)));

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));

    TEST_ASSERT_EQUAL(8, ctx->reg.pc);
    TEST_ASSERT_EQUAL_HEX8(0x2c, ctx->reg.v[4]);
    TEST_ASSERT_EQUAL_HEX8(0x89, ctx->reg.v[2]);

    TEST_ASSERT_EQUAL_STRING("ADD\tV2,\t0x99", ctx->last.opstr);
}


void test_op_LD_I_addr()
{
    c8_t *ctx;
    uint8_t code[] = {
            0xA1, 0x23, // LD I, 0x123
            0xA4, 0x56, // LD I, 0x456
    };

    ctx = c8_create();
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(ERR_OK, c8_load(ctx, 0, code, sizeof(code)));

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));

    TEST_ASSERT_EQUAL(2, ctx->reg.pc);
    TEST_ASSERT_EQUAL_HEX16(0x123, ctx->reg.i);

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));

    TEST_ASSERT_EQUAL(4, ctx->reg.pc);
    TEST_ASSERT_EQUAL_HEX16(0x456, ctx->reg.i);

    TEST_ASSERT_EQUAL_STRING("LD\tI,\t0x456", ctx->last.opstr);
}


void test_op_JP_V0_addr()
{
    c8_t *ctx;
    uint8_t code[] = {
            0x60, 0x36, // LD V0, 0x36
            0xB2, 0x34, // JP V0, 0x234
    };

    ctx = c8_create();
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(ERR_OK, c8_load(ctx, 0, code, sizeof(code)));

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(0x26a, ctx->reg.pc);

    TEST_ASSERT_EQUAL_STRING("JP\tV0,\t0x234", ctx->last.opstr);
}


static void test_op_subroutine()
{
    c8_t *ctx;
    uint8_t code[] = {
            0x61, 0x10, // 000: LD V1, 0x10
            0x20, 0x06, // 002: CALL 0x006
            0x00, 0x00, // 004: space
            0x20, 0x0a, // 006: CALL 0x00a
            0x00, 0xee, // 008: RET
            0x61, 0x77, // 010: LD V1, 0x77
            0x00, 0xee, // 012: RET
    };

    ctx = c8_create();
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(ERR_OK, c8_load(ctx, 0, code, sizeof(code)));

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(6, ctx->reg.pc);
    TEST_ASSERT_EQUAL(1, ctx->reg.sp);
    TEST_ASSERT_EQUAL(4, ctx->stack[0]);

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(10, ctx->reg.pc);
    TEST_ASSERT_EQUAL(2, ctx->reg.sp);
    TEST_ASSERT_EQUAL(4, ctx->stack[0]);
    TEST_ASSERT_EQUAL(8, ctx->stack[1]);

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(12, ctx->reg.pc);
    TEST_ASSERT_EQUAL_HEX8(0x77, ctx->reg.v[1]);

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(8, ctx->reg.pc);
    TEST_ASSERT_EQUAL(1, ctx->reg.sp);
    TEST_ASSERT_EQUAL(4, ctx->stack[0]);

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(4, ctx->reg.pc);
    TEST_ASSERT_EQUAL(0, ctx->reg.sp);

    TEST_ASSERT_EQUAL_STRING("RET", ctx->last.opstr);
}

void test_op_OP_Vx_Vy()
{
    c8_t *ctx;
    uint8_t code[] = {
            0x60, 0x56, // LD V0, 0x56
            0x61, 0x78, // LD V1, 0x78
            0x80, 0x10, // LD V0, V1

            0x60, 0x56, // LD V0, 0x56
            0x61, 0x78, // LD V1, 0x78
            0x80, 0x11, // OR V0, V1

            0x60, 0x56, // LD V0, 0x56
            0x61, 0x78, // LD V1, 0x78
            0x80, 0x12, // AND V0, V1

            0x60, 0x56, // LD V0, 0x56
            0x61, 0x78, // LD V1, 0x78
            0x80, 0x13, // XOR V0, V1

            // ADD
            0x60, 0x56, // LD V0, 0x56
            0x61, 0x78, // LD V1, 0x78
            0x80, 0x14, // ADD V0, V1

            0x60, 0x9a, // LD V0, 0x9a
            0x61, 0x89, // LD V1, 0x89
            0x80, 0x14, // ADD V0, V1

            // SUB
            0x60, 0x78, // LD V0, 0x78
            0x61, 0x56, // LD V1, 0x56
            0x80, 0x15, // SUB V0, V1

            0x60, 0x56, // LD V0, 0x56
            0x61, 0x78, // LD V1, 0x78
            0x80, 0x15, // SUB V0, V1

            // SHR
            0x61, 0x56, // LD V1, 0x56
            0x80, 0x16, // SHR V0, V1
            0x61, 0x55, // LD V1, 0x55
            0x80, 0x16, // SHR V0, V1

            // SUBN
            0x60, 0x78, // LD V0, 0x78
            0x61, 0x56, // LD V1, 0x56
            0x80, 0x17, // SUBN V0, V1

            0x60, 0x56, // LD V0, 0x56
            0x61, 0x78, // LD V1, 0x78
            0x80, 0x17, // SUBN V0, V1

            // SHL
            0x61, 0x56, // LD V1, 0x56
            0x80, 0x1e, // SHL V0, V1
            0x61, 0x87, // LD V1, 0x87
            0x80, 0x1e, // SHL V0, V1
    };

    ctx = c8_create();
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(ERR_OK, c8_load(ctx, 0, code, sizeof(code)));

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL_HEX8(0x78, ctx->reg.v[0]);

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL_HEX8(0x7e, ctx->reg.v[0]);

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL_HEX8(0x50, ctx->reg.v[0]);

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL_HEX8(0x2e, ctx->reg.v[0]);

    // ADD
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL_HEX8(0xce, ctx->reg.v[0]);
    TEST_ASSERT_EQUAL_HEX8(0, ctx->reg.v[0xf]);

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL_HEX8(0x23, ctx->reg.v[0]);
    TEST_ASSERT_EQUAL_HEX8(1, ctx->reg.v[0xf]);

    // SUB
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL_HEX8(0x22, ctx->reg.v[0]);
    TEST_ASSERT_EQUAL_HEX8(1, ctx->reg.v[0xf]);

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL_HEX8(0xde, ctx->reg.v[0]);
    TEST_ASSERT_EQUAL_HEX8(0, ctx->reg.v[0xf]);

    // SHR
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL_HEX8(0x2b, ctx->reg.v[0]);
    TEST_ASSERT_EQUAL_HEX8(0, ctx->reg.v[0xf]);

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL_HEX8(0x2a, ctx->reg.v[0]);
    TEST_ASSERT_EQUAL_HEX8(1, ctx->reg.v[0xf]);

    // SUBN
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL_HEX8(0xde, ctx->reg.v[0]);
    TEST_ASSERT_EQUAL_HEX8(0, ctx->reg.v[0xf]);

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL_HEX8(0x22, ctx->reg.v[0]);
    TEST_ASSERT_EQUAL_HEX8(1, ctx->reg.v[0xf]);

    // SHL
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL_HEX8(0xac, ctx->reg.v[0]);
    TEST_ASSERT_EQUAL_HEX8(0, ctx->reg.v[0xf]);

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL_HEX8(0x0e, ctx->reg.v[0]);
    TEST_ASSERT_EQUAL_HEX8(1, ctx->reg.v[0xf]);

    TEST_ASSERT_EQUAL_STRING("SHL\tV0,\tV1", ctx->last.opstr);
}

static void test_op_Fxxx_timers()
{
    c8_t *ctx;
    uint8_t code[] = {
            0x60, 0x78, // LD V0, 0x78
            0x61, 0x04, // LD V1, 0x04
            0xF0, 0x15, // LD DT, V0
            0xF1, 0x18, // LD ST, V1
    };

    ctx = c8_create();
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(ERR_OK, c8_load(ctx, 0, code, sizeof(code)));

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));

    TEST_ASSERT_EQUAL(0, ctx->reg.sound_timer);
    TEST_ASSERT_EQUAL(0, ctx->reg.delay_timer);

    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));

    TEST_ASSERT_EQUAL(0x78, ctx->reg.delay_timer);
    TEST_ASSERT_EQUAL(0x04, ctx->reg.sound_timer);

    TEST_ASSERT_EQUAL(ERR_SOUND_ON, c8_tick_60hz(ctx));
    TEST_ASSERT_EQUAL(ERR_SOUND_ON, c8_tick_60hz(ctx));
    TEST_ASSERT_EQUAL(ERR_SOUND_ON, c8_tick_60hz(ctx));

    TEST_ASSERT_EQUAL(0x75, ctx->reg.delay_timer);
    TEST_ASSERT_EQUAL(0x01, ctx->reg.sound_timer);

    TEST_ASSERT_EQUAL(ERR_OK, c8_tick_60hz(ctx));

    TEST_ASSERT_EQUAL_STRING("LD\tST,\tV1", ctx->last.opstr);
}

static void test_op_Fxxx_push_pop()
{
    int i;
    c8_t *ctx;
    uint8_t code[] = {
            0x60, 0x01, // LD V0, 0x01
            0x61, 0x02, // LD V1, 0x02
            0x62, 0x03, // LD V2, 0x03
            0x63, 0x04, // LD V3, 0x04
            0x64, 0x05, // LD V4, 0x05
            0x65, 0x06, // LD V5, 0x06
            0x66, 0x07, // LD V6, 0x07
            0x67, 0x08, // LD V7, 0x08
            0x68, 0x09, // LD V8, 0x09
            0x69, 0x0a, // LD V9, 0X0A
            0x6a, 0x0b, // LD VA, 0X0B
            0x6b, 0x0c, // LD VB, 0X0C
            0x6c, 0x0d, // LD VC, 0X0D
            0x6d, 0x0e, // LD VD, 0X0E
            0x6e, 0x0f, // LD VE, 0X0F
            0x6f, 0x10, // LD VF, 0x10
            0xa1, 0x00, // LD I, 0x100
            0xff, 0x55, // LD [I], VF

            0x60, 0x00, // LD V0, 0x00
            0x61, 0x00, // LD V1, 0x00
            0x62, 0x00, // LD V2, 0x00
            0x63, 0x00, // LD V3, 0x00
            0x64, 0x00, // LD V4, 0x00
            0x65, 0x00, // LD V5, 0x00
            0x66, 0x00, // LD V6, 0x00
            0x67, 0x00, // LD V7, 0x00
            0xa1, 0x00, // LD I, 0x100
            0xf7, 0x65, // LD V7, [I]
    };

    ctx = c8_create();
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(ERR_OK, c8_load(ctx, 0, code, sizeof(code)));

    for (i = 0; i < 18; i++)
    {
        TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    }

    for (i = 0; i < 16; i++)
    {
        TEST_ASSERT_EQUAL_HEX8(i + 1, ctx->mem[0x100 + i]);
    }

    for (i = 0; i < 10; i++)
    {
        TEST_ASSERT_EQUAL(ERR_OK, c8_step(ctx));
    }

    for (i = 0; i < 16; i++)
    {
        TEST_ASSERT_EQUAL_HEX8(i + 1, ctx->reg.v[i]);
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    UnityBegin("c8 test suite");
    RUN_TEST(test_load);
    RUN_TEST(test_op_invalid);
    RUN_TEST(test_op_JP_addr);
    RUN_TEST(test_op_skip);
    RUN_TEST(test_op_LD_Vx_byte);
    RUN_TEST(test_op_ADD_Vx_byte);
    RUN_TEST(test_op_LD_I_addr);
    RUN_TEST(test_op_JP_V0_addr);
    RUN_TEST(test_op_subroutine);
    RUN_TEST(test_op_OP_Vx_Vy);
    RUN_TEST(test_op_Fxxx_timers);
    RUN_TEST(test_op_Fxxx_push_pop);
    UnityEnd();

    return 0;
}
