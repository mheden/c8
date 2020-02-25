#include <c8.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MEM_SIZE 0x1000
#define WIDTH 64
#define HEIGHT 32
#define OPSTRLEN 15

#define BIT(n) (1 << (n))
#define FLAG_TRACE BIT(0)


struct c8
{
    struct
    {
        uint16_t pc;
        uint16_t i;
        uint8_t v[16];
        uint8_t sp;
        uint8_t sound_timer;
        uint8_t delay_timer;
    } reg;
    struct
    {
        uint16_t op;
        uint16_t pc;
        char opstr[OPSTRLEN + 1];
    } last;
    uint16_t stack[16];
    uint16_t keys;
    uint8_t mem[MEM_SIZE];
    uint8_t disp[WIDTH][HEIGHT];
    uint8_t flags;
};

/**
 * 6xkk - LD Vx, byte
 * Set Vx = kk.
 */
static int handle_6xxx(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg, value;

    reg = (opcode >> 8) & 0xf;
    value = opcode & 0xff;

    ctx->reg.v[reg] = value;
    snprintf(ctx->last.opstr, OPSTRLEN, "LD\tV%X,\t0x%02X", reg, value);

    return ERR_OK;
}

/**
 * 7xkk - ADD Vx, byte
 * Set Vx = Vx + kk.
 */
static int handle_7xxx(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg, value;

    reg = (opcode >> 8) & 0xf;
    value = opcode & 0xff;

    ctx->reg.v[reg] += value;
    snprintf(ctx->last.opstr, OPSTRLEN, "ADD\tV%X,\t0x%02X", reg, value);

    return ERR_OK;
}

/**
 * Annn - LD I, addr
 * Set I = nnn.
 */
static int handle_Axxx(c8_t *ctx, uint16_t opcode)
{
    uint16_t value = opcode & 0xfff;

    ctx->reg.i = value;
    snprintf(ctx->last.opstr, OPSTRLEN, "LD\tI,\t0x%03X", value);

    return ERR_OK;
}

/**
 * Bnnn - JP V0, addr
 * Jump to location nnn + V0.
 */
static int handle_Bxxx(c8_t *ctx, uint16_t opcode)
{
    uint16_t value = opcode & 0xfff;

    ctx->reg.pc = ctx->reg.v[0] + value;
    snprintf(ctx->last.opstr, OPSTRLEN, "JP\tV0,\t0x%03X", value);

    return ERR_OK;
}

c8_t *c8_create(void)
{
    c8_t *ctx;

    ctx = calloc(sizeof(c8_t), 1);
    if (ctx)
        return ctx;
    return NULL;
}

int c8_step(c8_t *ctx)
{
    int ret = ERR_INVALID_OP;
    uint16_t opcode;
    uint16_t pc;

    /* fetch, decode and execute OP code */
    pc = ctx->reg.pc;
    opcode = (ctx->mem[pc] << 8) | (ctx->mem[pc + 1]);
    ctx->reg.pc += 2;

    ctx->last.op = opcode;
    ctx->last.pc = pc;
    ctx->last.opstr[0] = '\0';

    switch (opcode & 0xf000)
    {
        case 0x6000:
        {
            ret = handle_6xxx(ctx, opcode);
            break;
        }
        case 0x7000:
        {
            ret = handle_7xxx(ctx, opcode);
            break;
        }
        case 0xA000:
        {
            ret = handle_Axxx(ctx, opcode);
            break;
        }
        case 0xB000:
        {
            ret = handle_Bxxx(ctx, opcode);
            break;
        }
    }

    if (ctx->flags & FLAG_TRACE)
        fprintf(stderr, "%03x:\t%04x\t;\t%s\n", ctx->last.pc, ctx->last.op,
                ctx->last.opstr);

    return ret;
}

int c8_load(c8_t *ctx, uint16_t address, uint8_t *data, uint16_t size)
{
    uint32_t end = (uint32_t)address + (uint32_t)size;
    if (end >= MEM_SIZE)
        return ERR_OUT_OF_MEM;
    memcpy(&ctx->mem[address], data, size);
    return ERR_OK;
}
