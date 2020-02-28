#include <c8.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MEM_SIZE 0x1000
#define WIDTH 64
#define HEIGHT 32
#define OPSTRLEN 15

#define BIT(n) (1 << (n))
#define FLAG_TRACE BIT (0)


/* helpers for extracting values from opcodes */
#define _NNN(opcode) ((opcode) & 0xFFF)
#define _X__(opcode) (((opcode) >> 8) & 0xF)
#define __Y_(opcode) (((opcode) >> 4) & 0xF)
#define __KK(opcode) ((opcode) & 0xFF)


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
    uint16_t addr = _NNN(opcode);

    ctx->reg.pc = addr;
    snprintf(ctx->last.opstr, OPSTRLEN, "JP\t0x%03X", addr);
    if (ctx->reg.pc == ctx->last.pc)
        return ERR_INFINIT_LOOP;
    else
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
    uint16_t addr = _NNN(opcode);

    ctx->stack[ctx->reg.sp] = ctx->reg.pc;
    ctx->reg.sp++;
    ctx->reg.pc = addr;
    snprintf(ctx->last.opstr, OPSTRLEN, "CALL\t0x%03X", addr);
    return ERR_OK;
}

/**
 * 3xkk - SE Vx, byte
 * Skip next instruction if Vx = kk.
 */
static int op_SE_Vx_byte(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg = _X__(opcode);
    uint8_t byte = __KK(opcode);

    if (ctx->reg.v[reg] == byte)
        ctx->reg.pc += 2;
    snprintf(ctx->last.opstr, OPSTRLEN, "SE\tV%X,\t0x%02X", reg, byte);
    return ERR_OK;
}

/**
 * 4xkk - SNE Vx, byte
 * Skip next instruction if Vx != kk.
 */
static int op_SNE_Vx_byte(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg = _X__(opcode);
    uint8_t byte = __KK(opcode);

    if (ctx->reg.v[reg] != byte)
        ctx->reg.pc += 2;
    snprintf(ctx->last.opstr, OPSTRLEN, "SNE\tV%X,\t0x%02X", reg, byte);
    return ERR_OK;
}

/**
 * 5xy0 - SE Vx, Vy
 * Skip next instruction if Vx = Vy.
 */
static int op_SE_Vx_Vy(c8_t *ctx, uint16_t opcode)
{
    uint8_t x = _X__(opcode);
    uint8_t y = __Y_(opcode);

    if (ctx->reg.v[x] == ctx->reg.v[y])
        ctx->reg.pc += 2;
    snprintf(ctx->last.opstr, OPSTRLEN, "SE\tV%X,\tV%X", x, y);
    return ERR_OK;
}

/**
 * 6xkk - LD Vx, byte
 * Set Vx = kk.
 */
static int op_LD_Vx_byte(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg = _X__(opcode);
    uint8_t byte = __KK(opcode);

    ctx->reg.v[reg] = byte;
    snprintf(ctx->last.opstr, OPSTRLEN, "LD\tV%X,\t0x%02X", reg, byte);
    return ERR_OK;
}

/**
 * 7xkk - ADD Vx, byte
 * Set Vx = Vx + kk.
 */
static int op_ADD_Vx_byte(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg = _X__(opcode);
    uint8_t byte = __KK(opcode);

    ctx->reg.v[reg] += byte;
    snprintf(ctx->last.opstr, OPSTRLEN, "ADD\tV%X,\t0x%02X", reg, byte);
    return ERR_OK;
}

/*
 * 8000 - OP Vx, Vy
 */
static int op_OP_Vx_Vy(c8_t *ctx, uint16_t opcode)
{
    int ret = ERR_INVALID_OP;
    uint8_t x = _X__(opcode);
    uint8_t y = __Y_(opcode);

    switch (opcode & 0xF)
    {
        case 0:
        {
            /*
             * 8xy0 - LD Vx, Vy
             * Set Vx = Vy.
             */
            ctx->reg.v[x] = ctx->reg.v[y];
            snprintf(ctx->last.opstr, OPSTRLEN, "LD\tV%X,\tV%X", x, y);
            ret = ERR_OK;
            break;
        }
        case 1:
        {
            /*
             * 8xy1 - OR Vx, Vy
             * Set Vx = Vx OR Vy.
             */
            ctx->reg.v[x] |= ctx->reg.v[y];
            snprintf(ctx->last.opstr, OPSTRLEN, "OR\tV%X,\tV%X", x, y);
            ret = ERR_OK;
            break;
        }
        case 2:
        {
            /*
             * 8xy2 - AND Vx, Vy
             * Set Vx = Vx AND Vy.
             */
            ctx->reg.v[x] &= ctx->reg.v[y];
            snprintf(ctx->last.opstr, OPSTRLEN, "AND\tV%X,\tV%X", x, y);
            ret = ERR_OK;
            break;
        }
        case 3:
        {
            /*
             * 8xy3 - XOR Vx, Vy
             * Set Vx = Vx XOR Vy.
             */
            ctx->reg.v[x] ^= ctx->reg.v[y];
            snprintf(ctx->last.opstr, OPSTRLEN, "XOR\tV%X,\tV%X", x, y);
            ret = ERR_OK;
            break;
        }
        case 4:
        {
            /*
             * 8xy4 - ADD Vx, Vy
             * Set Vx = Vx + Vy, set VF = carry.
             */
            ctx->reg.v[0xF] =
                    (uint16_t)ctx->reg.v[x] + (uint16_t)ctx->reg.v[y]
                    > 0xFF;
            ctx->reg.v[x] += ctx->reg.v[y];
            snprintf(ctx->last.opstr, OPSTRLEN, "ADD\tV%X,\tV%X", x, y);
            ret = ERR_OK;
            break;
        }
        case 5:
        {
            /*
             * 8xy5 - SUB Vx, Vy
             * Set Vx = Vx - Vy, set VF = NOT borrow.
             */
            ctx->reg.v[0xF] = ctx->reg.v[x] > ctx->reg.v[y];
            ctx->reg.v[x] -= ctx->reg.v[y];
            snprintf(ctx->last.opstr, OPSTRLEN, "SUB\tV%X,\tV%X", x, y);
            ret = ERR_OK;
            break;
        }
        case 6:
        {
            /*
             * 8xy6 - SHR Vx , Vy
             * Set Vx = Vy SHR 1.
             */
            ctx->reg.v[0xF] = ctx->reg.v[y] & 0x1;
            ctx->reg.v[x] = ctx->reg.v[y] >> 1;
            snprintf(ctx->last.opstr, OPSTRLEN, "SHR\tV%X,\tV%X", x, y);
            ret = ERR_OK;
            break;
        }
        case 7:
        {
            /*
             * 8xy7 - SUBN Vx, Vy
             * Set Vx = Vy - Vx, set VF = NOT borrow.
             */
            ctx->reg.v[0xF] = ctx->reg.v[y] > ctx->reg.v[x];
            ctx->reg.v[x] = ctx->reg.v[y] - ctx->reg.v[x];
            snprintf(ctx->last.opstr, OPSTRLEN, "SUBN\tV%X,\tV%X", x, y);
            ret = ERR_OK;
            break;
        }
        case 0xE:
        {
            /*
             * 8xyE - SHL Vx , Vy
             * Set Vx = Vy SHL 1.
             */
            ctx->reg.v[0xF] = ctx->reg.v[y] & 0x80 ? 1 : 0;
            ctx->reg.v[x] = ctx->reg.v[y] << 1;
            snprintf(ctx->last.opstr, OPSTRLEN, "SHL\tV%X,\tV%X", x, y);
            ret = ERR_OK;
            break;
        }
        default:
            ret = ERR_INVALID_OP;
    }

    return ret;
}

/**
 * 9xy0 - SNE Vx, Vy
 * Skip next instruction if Vx != Vy.
 */
static int op_SNE_Vx_Vy(c8_t *ctx, uint16_t opcode)
{
    uint8_t x = _X__(opcode);
    uint8_t y = __Y_(opcode);

    if (ctx->reg.v[x] != ctx->reg.v[y])
        ctx->reg.pc += 2;
    snprintf(ctx->last.opstr, OPSTRLEN, "SNE\tV%X,\tV%X", x, y);
    return ERR_OK;
}

/**
 * Annn - LD I, addr
 * Set I = nnn.
 */
static int op_LD_I_addr(c8_t *ctx, uint16_t opcode)
{
    uint16_t addr = _NNN(opcode);

    ctx->reg.i = addr;
    snprintf(ctx->last.opstr, OPSTRLEN, "LD\tI,\t0x%03X", addr);
    return ERR_OK;
}

/**
 * Bnnn - JP V0, addr
 * Jump to location nnn + V0.
 */
static int op_JP_V0_addr(c8_t *ctx, uint16_t opcode)
{
    uint16_t addr = _NNN(opcode);

    ctx->reg.pc = ctx->reg.v[0] + addr;
    snprintf(ctx->last.opstr, OPSTRLEN, "JP\tV0,\t0x%03X", addr);
    return ERR_OK;
}

/**
 * Misc instructions under 0xFnnn
 */
static int op_Fxxx(c8_t *ctx, uint16_t opcode)
{
    int ret = ERR_OK;
    uint8_t reg = _X__(opcode);

    switch (opcode & 0xFF)
    {
        case 0x07:
        {
            /* LD Vx, DT */
            ctx->reg.v[reg] = ctx->reg.delay_timer;
            snprintf(ctx->last.opstr, OPSTRLEN, "LD\tV%X,\tDT", reg);
            ret = ERR_OK;
            break;
        }
        // case 0x0A:
        // {
        /* LD Vx, K */
        /* TODO: */
        // break;
        // }
        case 0x15:
        {
            /* LD DT, Vx */
            ctx->reg.delay_timer = ctx->reg.v[reg];
            snprintf(ctx->last.opstr, OPSTRLEN, "LD\tDT,\tV%X", reg);
            ret = ERR_OK;
            break;
        }
        case 0x18:
        {
            /* LD ST, Vx */
            ctx->reg.sound_timer = ctx->reg.v[reg];
            snprintf(ctx->last.opstr, OPSTRLEN, "LD\tST,\tV%X", reg);
            ret = ERR_OK;
            break;
        }
        case 0x1E:
        {
            /* ADD I, Vx */
            ctx->reg.v[0xF] =
                    ((uint32_t)ctx->reg.i + (uint32_t)ctx->reg.v[reg]) > 0xFFF;
            ctx->reg.i = ctx->reg.i + ctx->reg.v[reg];
            snprintf(ctx->last.opstr, OPSTRLEN, "ADD\tI,\tV%X", reg);
            ret = ERR_OK;
            break;
        }
        case 0x29:
        {
            /* LD I, FONT(Vx) */
            ctx->reg.i = ctx->reg.v[reg] * 5;
            snprintf(ctx->last.opstr, OPSTRLEN, "LD\tI,\tFONT(V%X)", reg);
            ret = ERR_OK;
            break;
        }
        case 0x33:
        {
            /* LD B, Vx */
            ctx->mem[ctx->reg.i + 0] = (ctx->reg.v[reg] / 100) % 10;
            ctx->mem[ctx->reg.i + 1] = (ctx->reg.v[reg] / 10) % 10;
            ctx->mem[ctx->reg.i + 2] = ctx->reg.v[reg] % 10;
            snprintf(ctx->last.opstr, OPSTRLEN, "LD\tB,\tV%X", reg);
            ret = ERR_OK;
            break;
        }
        case 0x55:
        {
            /* LD [I], Vx */
            uint8_t i;
            for (i = 0; i <= reg; i++)
                ctx->mem[ctx->reg.i++] = ctx->reg.v[i];
            snprintf(ctx->last.opstr, OPSTRLEN, "LD\t[I],\tV%X", reg);
            ret = ERR_OK;
            break;
        }
        case 0x65:
        {
            /* LD Vx, [I] */
            uint8_t i;
            for (i = 0; i <= reg; i++)
                ctx->reg.v[i] = ctx->mem[ctx->reg.i++];
            snprintf(ctx->last.opstr, OPSTRLEN, "LD\tV%X,\t[I]", reg);
            ret = ERR_OK;
            break;
        }
        default:
            ret = ERR_INVALID_OP;
    }
    return ret;
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

    switch (opcode & 0xF000)
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
        case 0x8000:
        {
            ret = op_OP_Vx_Vy(ctx, opcode);
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
        case 0xC000:
        {
            /*
             * Cxkk - RND Vx, byte
             * Set Vx = random byte AND kk.
             */
            uint8_t reg = _X__(opcode);
            uint8_t byte = __KK(opcode);
            ctx->reg.v[reg] = rand() & byte;
            snprintf(ctx->last.opstr, OPSTRLEN, "RND\tV%x,\t0x%X", reg, byte);
            ret = ERR_OK;
            break;
        }
        case 0xF000:
        {
            ret = op_Fxxx(ctx, opcode);
            break;
        }
    }

    // ctx->flags = FLAG_TRACE;
    if (ctx->flags & FLAG_TRACE)
        fprintf(stderr, "%03x:\t%04x\t;\t%s\n", ctx->last.pc, ctx->last.op,
                ctx->last.opstr);

    return ret;
}

int c8_tick_60hz(c8_t *ctx)
{
    int ret = ERR_OK;

    if (ctx->reg.delay_timer > 0)
        ctx->reg.delay_timer--;
    if (ctx->reg.sound_timer > 0)
        ctx->reg.sound_timer--;
    if (ctx->reg.sound_timer > 0)
        ret = ERR_SOUND_ON;

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

void c8_debug_dump_memory(c8_t *ctx, uint16_t address, uint16_t length)
{
    int i = 0;
    uint8_t *memptr = &ctx->mem[address];

    for (i = 0; i < length; i++)
    {
        if (i % 16 == 0)
        {
            if (i != 0)
                fprintf(stderr, "\n%03x:", address + i);
            else
                fprintf(stderr, "%03x:", address + i);
        }
        fprintf(stderr, " %02x", *memptr++);
    }
    fprintf(stderr, "\n");
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

    fprintf(stderr, "\n      ST=%02X DT=%02X", ctx->reg.sound_timer,
            ctx->reg.delay_timer);
    fprintf(stderr, "\n      I=%03X PC=%03X SP=%02x\nstack:", ctx->reg.i,
            ctx->reg.pc, ctx->reg.sp);
    for (i = 0; i < ctx->reg.sp; i++)
    {
        fprintf(stderr, " %03X", ctx->stack[i]);
    }
    fprintf(stderr, "\n");
}
