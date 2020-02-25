#include <c8.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MEM_SIZE 0x1000
#define WIDTH 64
#define HEIGHT 32
#define OPSTRLEN 15


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
    uint8_t mem[MEM_SIZE];
    uint8_t disp[WIDTH][HEIGHT];
    uint16_t keys;
};


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
    (void)ctx;
    return ERR_OK;
}

int c8_load(c8_t *ctx, uint16_t address, uint8_t *data, uint16_t size)
{
    uint32_t end = (uint32_t)address + (uint32_t)size;
    if (end >= MEM_SIZE)
        return ERR_OUT_OF_MEM;
    memcpy(&ctx->mem[address], data, size);
    return ERR_OK;
}
