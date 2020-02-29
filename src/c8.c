#include <c8.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MEM_SIZE 0x1000
#define LOAD_ADDR 0x200
#define WIDTH 64
#define HEIGHT 32
#define OPSTRLEN 15

#define BIT(n) (1 << (n))
#define FLAG_TRACE BIT(0)


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


typedef int (*opfn)(c8_t *ctx, uint16_t opcode);

typedef struct {
    uint16_t opcode;
    uint16_t mask;
    opfn fn;
} op_t;

#define ARRAY_SIZE(x) ((sizeof x) / (sizeof *x))


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

/**
 * 8xy0 - LD Vx, Vy
 * Set Vx = Vy.
 */
static int op_LD_Vx_Vy(c8_t *ctx, uint16_t opcode)
{
    uint8_t x = _X__(opcode);
    uint8_t y = __Y_(opcode);

    ctx->reg.v[x] = ctx->reg.v[y];
    snprintf(ctx->last.opstr, OPSTRLEN, "LD\tV%X,\tV%X", x, y);
    return ERR_OK;
}

/**
 * 8xy1 - OR Vx, Vy
* Set Vx = Vx OR Vy.
*/
static int op_OR_Vx_Vy(c8_t *ctx, uint16_t opcode)
{
    uint8_t x = _X__(opcode);
    uint8_t y = __Y_(opcode);

    ctx->reg.v[x] |= ctx->reg.v[y];
    snprintf(ctx->last.opstr, OPSTRLEN, "OR\tV%X,\tV%X", x, y);
    return ERR_OK;
}


/**
 * 8xy2 - AND Vx, Vy
 * Set Vx = Vx AND Vy.
 */
static int op_AND_Vx_Vy(c8_t *ctx, uint16_t opcode)
{
    uint8_t x = _X__(opcode);
    uint8_t y = __Y_(opcode);

    ctx->reg.v[x] &= ctx->reg.v[y];
    snprintf(ctx->last.opstr, OPSTRLEN, "AND\tV%X,\tV%X", x, y);
    return ERR_OK;
}

/**
 * 8xy3 - XOR Vx, Vy
 * Set Vx = Vx XOR Vy.
 */
static int op_XOR_Vx_Vy(c8_t *ctx, uint16_t opcode)
{
    uint8_t x = _X__(opcode);
    uint8_t y = __Y_(opcode);

    ctx->reg.v[x] ^= ctx->reg.v[y];
    snprintf(ctx->last.opstr, OPSTRLEN, "XOR\tV%X,\tV%X", x, y);
    return ERR_OK;
}

/**
 * 8xy4 - ADD Vx, Vy
 * Set Vx = Vx + Vy, set VF = carry.
 */
static int op_ADD_Vx_Vy(c8_t *ctx, uint16_t opcode)
{
    uint8_t x = _X__(opcode);
    uint8_t y = __Y_(opcode);

    ctx->reg.v[0xF] = (uint16_t)ctx->reg.v[x] + (uint16_t)ctx->reg.v[y] > 0xFF;
    ctx->reg.v[x] += ctx->reg.v[y];
    snprintf(ctx->last.opstr, OPSTRLEN, "ADD\tV%X,\tV%X", x, y);
    return ERR_OK;
}
 
/**
 * 8xy5 - SUB Vx, Vy
 * Set Vx = Vx - Vy, set VF = NOT borrow.
 */
static int op_SUB_Vx_Vy(c8_t *ctx, uint16_t opcode)
{
    uint8_t x = _X__(opcode);
    uint8_t y = __Y_(opcode);

    ctx->reg.v[0xF] = ctx->reg.v[x] > ctx->reg.v[y];
    ctx->reg.v[x] -= ctx->reg.v[y];
    snprintf(ctx->last.opstr, OPSTRLEN, "SUB\tV%X,\tV%X", x, y);
    return ERR_OK;
}

/**
 * 8xy6 - SHR Vx, Vy
 * Set Vx = Vy SHR 1.
 */
static int op_SHR_Vx_Vy(c8_t *ctx, uint16_t opcode)
{
    uint8_t x = _X__(opcode);
    uint8_t y = __Y_(opcode);

    ctx->reg.v[0xF] = ctx->reg.v[y] & 0x1;
    ctx->reg.v[x] = ctx->reg.v[y] >> 1;
    snprintf(ctx->last.opstr, OPSTRLEN, "SHR\tV%X,\tV%X", x, y);
    return ERR_OK;
}

/**
 * 8xy7 - SUBN Vx, Vy
 * Set Vx = Vy - Vx, set VF = NOT borrow.
 */
static int op_SUBN_Vx_Vy(c8_t *ctx, uint16_t opcode)
{
    uint8_t x = _X__(opcode);
    uint8_t y = __Y_(opcode);

    ctx->reg.v[0xF] = ctx->reg.v[y] > ctx->reg.v[x];
    ctx->reg.v[x] = ctx->reg.v[y] - ctx->reg.v[x];
    snprintf(ctx->last.opstr, OPSTRLEN, "SUBN\tV%X,\tV%X", x, y);
    return ERR_OK;
}

/**
 * 8xyE - SHL Vx , Vy
 * Set Vx = Vy SHL 1.
 */
static int op_SHL_Vx_Vy(c8_t *ctx, uint16_t opcode)
{
    uint8_t x = _X__(opcode);
    uint8_t y = __Y_(opcode);

    ctx->reg.v[0xF] = ctx->reg.v[y] & 0x80 ? 1 : 0;
    ctx->reg.v[x] = ctx->reg.v[y] << 1;
    snprintf(ctx->last.opstr, OPSTRLEN, "SHL\tV%X,\tV%X", x, y);
    return ERR_OK;
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
 * Cxkk - RND Vx, byte
 * Set Vx = random byte AND kk.
 */
static int op_RND_Vx_byte(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg = _X__(opcode);
    uint8_t byte = __KK(opcode);

    ctx->reg.v[reg] = rand() & byte;
    snprintf(ctx->last.opstr, OPSTRLEN, "RND\tV%x,\t0x%X", reg, byte);
    return ERR_OK;
}

static int op_LD_Vx_DT(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg = _X__(opcode);

    ctx->reg.v[reg] = ctx->reg.delay_timer;
    snprintf(ctx->last.opstr, OPSTRLEN, "LD\tV%X,\tDT", reg);
    return ERR_OK;
}

// case 0x0A:
// {
/* LD Vx, K */
/* TODO: */
// break;
// }

static int op_LD_DT_Vx(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg = _X__(opcode);

    ctx->reg.delay_timer = ctx->reg.v[reg];
    snprintf(ctx->last.opstr, OPSTRLEN, "LD\tDT,\tV%X", reg);
    return ERR_OK;
}

static int op_LD_ST_Vx(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg = _X__(opcode);

    ctx->reg.sound_timer = ctx->reg.v[reg];
    snprintf(ctx->last.opstr, OPSTRLEN, "LD\tST,\tV%X", reg);
    return ERR_OK;
}

static int op_ADD_I_Vx(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg = _X__(opcode);

    ctx->reg.v[0xF] =
            ((uint32_t)ctx->reg.i + (uint32_t)ctx->reg.v[reg]) > 0xFFF;
    ctx->reg.i = ctx->reg.i + ctx->reg.v[reg];
    snprintf(ctx->last.opstr, OPSTRLEN, "ADD\tI,\tV%X", reg);
    return ERR_OK;
}

static int op_LD_I_FONT_Vx(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg = _X__(opcode);

    ctx->reg.i = ctx->reg.v[reg] * 5;
    snprintf(ctx->last.opstr, OPSTRLEN, "LD\tI,\tFONT(V%X)", reg);
    return ERR_OK;
}

static int op_LD_B_Vx(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg = _X__(opcode);

    ctx->mem[ctx->reg.i + 0] = (ctx->reg.v[reg] / 100) % 10;
    ctx->mem[ctx->reg.i + 1] = (ctx->reg.v[reg] / 10) % 10;
    ctx->mem[ctx->reg.i + 2] = ctx->reg.v[reg] % 10;
    snprintf(ctx->last.opstr, OPSTRLEN, "LD\tB,\tV%X", reg);
    return ERR_OK;
}

static int op_LD_addrI_Vx(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg = _X__(opcode);
    uint8_t i;

    for (i = 0; i <= reg; i++)
        ctx->mem[ctx->reg.i++] = ctx->reg.v[i];
    snprintf(ctx->last.opstr, OPSTRLEN, "LD\t[I],\tV%X", reg);
    return ERR_OK;
}

static int op_LD_Vx_addrI(c8_t *ctx, uint16_t opcode)
{
    uint8_t reg = _X__(opcode);
    uint8_t i;

    for (i = 0; i <= reg; i++)
        ctx->reg.v[i] = ctx->mem[ctx->reg.i++];
    snprintf(ctx->last.opstr, OPSTRLEN, "LD\tV%X,\t[I]", reg);
    return ERR_OK;
}

op_t ops[] = {{0x00EE, 0xFFFF, op_RET},
              {0x1000, 0xF000, op_JP_addr},
              {0x2000, 0xF000, op_CALL_addr},
              {0x3000, 0xF000, op_SE_Vx_byte},
              {0x4000, 0xF000, op_SNE_Vx_byte},
              {0x5000, 0xF00F, op_SE_Vx_Vy},
              {0x6000, 0xF000, op_LD_Vx_byte},
              {0x7000, 0xF000, op_ADD_Vx_byte},
              {0x8000, 0xF00F, op_LD_Vx_Vy},
              {0x8001, 0xF00F, op_OR_Vx_Vy},
              {0x8002, 0xF00F, op_AND_Vx_Vy},
              {0x8003, 0xF00F, op_XOR_Vx_Vy},
              {0x8004, 0xF00F, op_ADD_Vx_Vy},
              {0x8005, 0xF00F, op_SUB_Vx_Vy},
              {0x8006, 0xF00F, op_SHR_Vx_Vy},
              {0x8007, 0xF00F, op_SUBN_Vx_Vy},
              {0x800E, 0xF00F, op_SHL_Vx_Vy},
              {0x9000, 0xF00F, op_SNE_Vx_Vy},
              {0xA000, 0xF000, op_LD_I_addr},
              {0xB000, 0xF000, op_JP_V0_addr},
              {0xC000, 0xF000, op_RND_Vx_byte},

              {0xF007, 0xF0FF, op_LD_Vx_DT},
              //  { 0xF00A, 0xF0FF, op_LD_Vx_K},
              {0xF015, 0xF0FF, op_LD_DT_Vx},
              {0xF018, 0xF0FF, op_LD_ST_Vx},
              {0xF01E, 0xF0FF, op_ADD_I_Vx},
              {0xF029, 0xF0FF, op_LD_I_FONT_Vx},
              {0xF033, 0xF0FF, op_LD_B_Vx},
              {0xF055, 0xF0FF, op_LD_addrI_Vx},
              {0xF065, 0xF0FF, op_LD_Vx_addrI}};


c8_t *c8_create(void)
{
    c8_t *ctx = calloc(sizeof(c8_t), 1);
    if (ctx)
        return ctx;
    return NULL;
}

int c8_step(c8_t *ctx)
{
    int ret = ERR_INVALID_OP;
    unsigned int i;
    uint16_t opcode;
    uint16_t pc;

    /* fetch, decode and execute OP code */
    pc = ctx->reg.pc;
    opcode = (ctx->mem[pc] << 8) | (ctx->mem[pc + 1]);
    ctx->reg.pc += 2;

    ctx->last.op = opcode;
    ctx->last.pc = pc;
    ctx->last.opstr[0] = '\0';

    for (i = 0; i < ARRAY_SIZE(ops); i++)
    {
        if ((opcode & ops[i].mask) == ops[i].opcode)
        {
            ret = ops[i].fn(ctx, opcode);
            break;
        }
    }

    ctx->flags = FLAG_TRACE;
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

int c8_load_file(c8_t *ctx, const char *filename)
{
    FILE *file;
    size_t nbytes;

    file = fopen(filename, "rb");
    if (!file)
        return ERR_FILE_NOT_FOUND;

    nbytes = fread(&ctx->mem[LOAD_ADDR], 1, MEM_SIZE -LOAD_ADDR, file);
    fclose(file);
    c8_set_pc(ctx, LOAD_ADDR);
    return nbytes;
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
