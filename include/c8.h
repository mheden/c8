#ifndef C8_H
#define C8_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ERR_OK               0
#define ERR_INVALID_OP      -1
#define ERR_INFINIT_LOOP    -2

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


#ifdef __cplusplus
}
#endif

#endif /* C8_H */
