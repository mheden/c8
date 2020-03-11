#ifndef C8_H
#define C8_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ERR_SOUND_ON 1
#define ERR_OK 0
#define ERR_INVALID_OP -1
#define ERR_INFINIT_LOOP -2
#define ERR_OUT_OF_MEM -3
#define ERR_FILE_NOT_FOUND -4


typedef struct c8 c8_t;


/**
 *
 *
 */
c8_t *c8_create(void);

/**
 *
 *
 */
int c8_step(c8_t *ctx);

/**
 *
 *
 */
int c8_tick_60hz(c8_t *ctx);

/**
 *
 *
 */
int c8_load(c8_t *ctx, uint16_t address, uint8_t *data, uint16_t size);

/**
 *
 *
 */
int c8_load_file(c8_t *ctx, const char *filename);

/**
 *
 *
 */
void c8_set_pc(c8_t *ctx, uint16_t pc);

/**
 *
 *
 */
void c8_set_keys(c8_t *ctx, uint16_t keys);

/**
 *
 *
 */
int c8_redraw_needed(c8_t *ctx);

/**
 *
 *
 */
void c8_redraw_ack(c8_t *ctx);

/**
 *
 *
 */
void c8_debug_dump_memory(c8_t *ctx, uint16_t address, uint16_t length);

/**
 *
 *
 */
void c8_debug_dump_state(c8_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* C8_H */
