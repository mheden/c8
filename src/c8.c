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
 * 00EE - RET
 * Return from a subroutine.
 *
 * The interpreter sets the program counter to the address at the top of the
 * stack, then subtracts 1 from the stack pointer.
 */
static int op_RET(c8_t *ctx, uint16_t opcode)
{
    (void)opcode;

    ctx->reg.sp--;
    ctx->reg.pc = ctx->stack[ctx->reg.sp];
    snprintf(ctx->last.opstr, OPSTRLEN, "RET");

    return ERR_OK;
}

/**
 * 1nnn - JP addr
 * Jump to location nnn.
 */
static int op_JP_addr(c8_t *ctx, uint16_t opcode)
{
    uint16_t value = opcode & 0xfff;

    ctx->reg.pc = value;
    snprintf(ctx->last.opstr, OPSTRLEN, "JP\t0x%03X", value);

    return ERR_OK;
}

/**
 * 2nnn - CALL addr
 * Call subroutine at nnn.
 *
 * The interpreter increments the stack pointer, then puts the current PC on
 * the top of the stack. The PC is then set to nnn.
 */
static int op_CALL_addr(c8_t *ctx, uint16_t opcode)
{
    uint16_t value = opcode & 0xfff;

    ctx->stack[ctx->reg.sp] = ctx->reg.pc;
    ctx->reg.sp++;
    ctx->reg.pc = value;
    snprintf(ctx->last.opstr, OPSTRLEN, "CALL\t0x%03X", value);

    return ERR_OK;
}

/**
 * 3xkk - SE Vx, byte
 * Skip next instruction if Vx = kk.
 */
static int op_SE_Vx_byte(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg, value;

    reg = (opcode >> 8) & 0xf;
    value = opcode & 0xff;

    if (ctx->reg.v[reg] == value)
        ctx->reg.pc += 2;
    snprintf(ctx->last.opstr, OPSTRLEN, "SE\tV%X,\t0x%02X", reg, value);

    return ERR_OK;
}

/**
 * 4xkk - SNE Vx, byte
 * Skip next instruction if Vx != kk.
 */
static int op_SNE_Vx_byte(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg, value;

    reg = (opcode >> 8) & 0xf;
    value = opcode & 0xff;

    if (ctx->reg.v[reg] != value)
        ctx->reg.pc += 2;
    snprintf(ctx->last.opstr, OPSTRLEN, "SNE\tV%X,\t0x%02X", reg, value);

    return ERR_OK;
}

/**
 * 5xy0 - SE Vx, Vy
 * Skip next instruction if Vx = Vy.
 */
static int op_SE_Vx_Vy(c8_t *ctx, uint16_t opcode)
{
    uint8_t regX, regY;

    regX = (opcode >> 8) & 0xf;
    regY = (opcode >> 4) & 0xf;

    if (ctx->reg.v[regX] == ctx->reg.v[regY])
        ctx->reg.pc += 2;
    snprintf(ctx->last.opstr, OPSTRLEN, "SE\tV%X,\tV%X", regX, regY);

    return ERR_OK;
}

/**
 * 6xkk - LD Vx, byte
 * Set Vx = kk.
 */
static int op_LD_Vx_byte(c8_t *ctx, uint16_t opcode)
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
static int op_ADD_Vx_byte(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg, value;

    reg = (opcode >> 8) & 0xf;
    value = opcode & 0xff;

    ctx->reg.v[reg] += value;
    snprintf(ctx->last.opstr, OPSTRLEN, "ADD\tV%X,\t0x%02X", reg, value);

    return ERR_OK;
}

/**
 * 9xy0 - SNE Vx, Vy
 * Skip next instruction if Vx != Vy.
 */
static int op_SNE_Vx_Vy(c8_t *ctx, uint16_t opcode)
{
    uint8_t regX, regY;

    regX = (opcode >> 8) & 0xf;
    regY = (opcode >> 4) & 0xf;

    if (ctx->reg.v[regX] != ctx->reg.v[regY])
        ctx->reg.pc += 2;
    snprintf(ctx->last.opstr, OPSTRLEN, "SNE\tV%X,\tV%X", regX, regY);

    return ERR_OK;
}

/**
 * Annn - LD I, addr
 * Set I = nnn.
 */
static int op_LD_I_addr(c8_t *ctx, uint16_t opcode)
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
static int op_JP_V0_addr(c8_t *ctx, uint16_t opcode)
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
        case 0x0000:
        {
            if (opcode == 0x00EE)
            {
                ret = op_RET(ctx, opcode);
            }
            break;
        }
        case 0x1000:
        {
            ret = op_JP_addr(ctx, opcode);
            if (ctx->reg.pc == ctx->last.pc)
                ret = ERR_INFINIT_LOOP;
            break;
        }
        case 0x2000:
        {
            ret = op_CALL_addr(ctx, opcode);
            break;
        }
        case 0x3000:
        {
            ret = op_SE_Vx_byte(ctx, opcode);
            break;
        }
        case 0x4000:
        {
            ret = op_SNE_Vx_byte(ctx, opcode);
            break;
        }
        case 0x5000:
        {
            if ((opcode & 0xF) == 0)
                ret = op_SE_Vx_Vy(ctx, opcode);
            break;
        }
        case 0x6000:
        {
            ret = op_LD_Vx_byte(ctx, opcode);
            break;
        }
        case 0x7000:
        {
            ret = op_ADD_Vx_byte(ctx, opcode);
            break;
        }
        case 0x9000:
        {
            if ((opcode & 0xF) == 0)
                ret = op_SNE_Vx_Vy(ctx, opcode);
            break;
        }
        case 0xA000:
        {
            ret = op_LD_I_addr(ctx, opcode);
            break;
        }
        case 0xB000:
        {
            ret = op_JP_V0_addr(ctx, opcode);
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

void c8_set_pc(c8_t *ctx, uint16_t pc)
{
    ctx->reg.pc = pc;
}

void c8_debug_dump_memory(c8_t *ctx)
{
    int row, col;
    uint8_t *memptr = ctx->mem;

    for (row = 0; row < 128; row++)
    {
        fprintf(stderr, "%03x:", row * col);
        for (col = 0; col < 32; col++)
        {
            fprintf(stderr, " %02x", *memptr++);
        }
        fprintf(stderr, "\n");
    }
}

void c8_debug_dump_state(c8_t *ctx)
{
    int i;

    fprintf(stderr, "regs: ");
    for (i = 0; i < 8; i++)
    {
        fprintf(stderr, "V%X=%02X ", i, ctx->reg.v[i]);
    }
    fprintf(stderr, "\n      ");
    for (i = 8; i < 16; i++)
    {
        fprintf(stderr, "V%X=%02X ", i, ctx->reg.v[i]);
    }

    fprintf(stderr, "\n      I=%03X PC=%03X SP=%02x\nstack:", ctx->reg.i,
            ctx->reg.pc, ctx->reg.sp);
    for (i = 0; i < ctx->reg.sp; i++)
    {
        fprintf(stderr, " %03X", ctx->stack[i]);
    }
    fprintf(stderr, "\n");
}
