#include <SDL2/SDL.h>
#include <time.h>
#include <unistd.h>
#include <c8.h>

// http://gigi.nullneuron.net/gigilabs/sdl2-pixel-drawing/

// export SDL_VIDEO_X11_VISUALID=
/*
 * TODO:
 * - sound
 * - graphics scaling
 * - SuperChip8 support
 */

enum
{
    CLOCKSPEED_60Hz = 1,
    CLOCKSPEED_480Hz = 8,
    CLOCKSPEED_1020Hz = 17,
} clockspeed;

#define BIT(n) (1 << (n))

#define COLOR 0x00008000

#ifdef HIRES
#define TITLE "%s: %s"
#define WIDTH 128
#define HEIGHT 64
#else
#define TITLE "%s: %s"
#define SCALE 1
#define WIDTH 64
#define HEIGHT 32
#endif


static uint64_t get_us()
{
    struct timespec spec;
    uint64_t us;

    clock_gettime(CLOCK_MONOTONIC, &spec);
    us = spec.tv_sec * 1e6 + spec.tv_nsec / 1e3;
    return us;
}


int main(int argc, char **argv)
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;
    uint32_t *display = NULL;
    c8_t *ctx = NULL;
    uint64_t nextvsync;
    char window_title[256] = "\0";
    int trace = 0;

    if (argc != 2)
    {
        printf("usage:\n\t%s <rom.ch8>\n", argv[0]);
        return -1;
    }

    ctx = c8_create();
    if (c8_load_file(ctx, argv[1]) == ERR_FILE_NOT_FOUND)
    {
        printf("File '%s' not found\n", argv[1]);
        return ERR_FILE_NOT_FOUND;
    }
    snprintf(window_title, 255, TITLE, argv[0], argv[1]);

    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, WIDTH * SCALE,
                              HEIGHT * SCALE, 0);
    renderer = SDL_CreateRenderer(window, -1, 0);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STATIC, WIDTH, HEIGHT);

    display = malloc(SCALE * WIDTH * HEIGHT * sizeof(uint32_t));
    memset(display, 0, SCALE * WIDTH * HEIGHT * sizeof(uint32_t));

    nextvsync = get_us();
    while (6 != 9)
    {
        int i;
        SDL_Event event;
        uint16_t keys = 0;

        for (i = 0; i < CLOCKSPEED_480Hz; i++)
        {
            int res = c8_step(ctx);
            if (res == ERR_INVALID_OP)
            {
                uint16_t op, pc;
                (void)c8_debug_get_last(ctx, &op, &pc);
                printf("Illegal instruction %04x at %04x\n", op, pc);
                goto end;
            }
        }

        SDL_PollEvent(&event);
        switch (event.type)
        {
            case SDL_QUIT:
                goto end;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_1:    keys |= BIT(0);     break;
                    case SDLK_2:    keys |= BIT(1);     break;
                    case SDLK_3:    keys |= BIT(2);     break;
                    case SDLK_4:    keys |= BIT(3);     break;
                    case SDLK_q:    keys |= BIT(4);     break;
                    case SDLK_w:    keys |= BIT(5);     break;
                    case SDLK_e:    keys |= BIT(6);     break;
                    case SDLK_r:    keys |= BIT(7);     break;
                    case SDLK_a:    keys |= BIT(8);     break;
                    case SDLK_s:    keys |= BIT(9);     break;
                    case SDLK_d:    keys |= BIT(10);    break;
                    case SDLK_f:    keys |= BIT(11);    break;
                    case SDLK_z:    keys |= BIT(12);    break;
                    case SDLK_x:    keys |= BIT(13);    break;
                    case SDLK_c:    keys |= BIT(14);    break;
                    case SDLK_v:    keys |= BIT(15);    break;
                    case SDLK_PERIOD:
                        trace = (trace + 1) & 1;
                        c8_debug_set_trace(ctx, trace);
                        break;
                    case SDLK_ESCAPE:
                        goto end;
                        break;
                }
                c8_set_keys(ctx, keys);
                break;
        }

        /* wait for vertical sync (60 Hz) */
        do
        {
            usleep(500);
        } while (get_us() < nextvsync);

        c8_tick_60hz(ctx);
        nextvsync += 16667;

        int x, y, s;
        for (y = 0; y < HEIGHT; y++)
        {
            for (x = 0; x < WIDTH; x++)
            {
                // printf("--- %d, %d\n", x, y);
                if (c8_get_pixel(ctx, x, y))
                {
                    // set_pixel(display, x, y, COLOR);
                    for (s = 0; s < SCALE; s++)
                    {
                        display[y * WIDTH * SCALE + x + s] = COLOR;
                    }
                }
                else
                {
                    // set_pixel(display, x, y, 0);
                    for (s = 0; s < SCALE; s++)
                    {
                        display[y * WIDTH * SCALE + x + s] = 0x00000000;
                    }
                }
            }
        }

        SDL_UpdateTexture(texture, NULL, display, WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

end:
    if (display)
        free(display);
    if (texture)
        SDL_DestroyTexture(texture);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    if (ctx)
        free(ctx);
    SDL_Quit();

    return EXIT_SUCCESS;
}
