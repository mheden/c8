#include <SDL2/SDL.h>
// #define _XOPEN_SOURCE 700
#include <c8.h>
#include <time.h>
#include <unistd.h>

// http://gigi.nullneuron.net/gigilabs/sdl2-pixel-drawing/

// export SDL_VIDEO_X11_VISUALID=
/*
 * TODO:
 * - sound
 * - graphics scaling
 * - SuperChip8 support
 */

#define ARRAY_SIZE(x) ((sizeof x) / (sizeof *x))

uint64_t get_us()
{
    struct timespec spec;
    uint64_t us;

    clock_gettime(CLOCK_MONOTONIC, &spec);
    us = spec.tv_sec * 1e6 + spec.tv_nsec / 1e3;
    return us;
}

enum
{
    CLOCKSPEED_480kHz = 8,
    CLOCKSPEED_1020kHz = 17,
} clockspeed;

#define COLOR 0x00008000

#ifdef HIRES
#define TITLE "sc8emu: %s"
#define WIDTH 640
#define HEIGHT 480
#else
#define TITLE "c8emu: %s"
#define SCALE 1
#define WIDTH 64
#define HEIGHT 32
#endif


int main(int argc, char **argv)
{
    int done = 0;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    uint32_t *display;
    uint64_t vsync;
    c8_t *ctx;
    char window_title[256] = "\0";

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
    snprintf(window_title, 255, TITLE, argv[1]);

    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, WIDTH * SCALE,
                              HEIGHT * SCALE, 0);
    renderer = SDL_CreateRenderer(window, -1, 0);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STATIC, WIDTH, HEIGHT);

    display = malloc(SCALE * WIDTH * HEIGHT * sizeof(uint32_t));
    memset(display, 0, SCALE * WIDTH * HEIGHT * sizeof(uint32_t));

    vsync = get_us();
    while (!done)
    {
        int i;
        SDL_Event event;
        uint64_t currenttime;

        for (i = 0; i < CLOCKSPEED_480kHz; i++)
        {
            int res = c8_step(ctx);
            if (res != ERR_OK)
                break;
        }

        SDL_PollEvent(&event);
        switch (event.type)
        {
            case SDL_QUIT:
                done = 1;
                // return 0;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    // case SDLK_LEFT:
                    //     printf("left\n");
                    //     x--;
                    //     break;
                    // case SDLK_RIGHT:
                    //     x++;
                    //     break;
                    // case SDLK_UP:
                    //     y--;
                    //     break;
                    // case SDLK_DOWN:
                    //     y++;
                    //     break;
                    case SDLK_q:
                        done = 1;
                        return 0;
                        ;
                        break;
                }
                break;
        }

        /* wait for 60Hz */
        do
        {
            usleep(500);
            currenttime = get_us();
        } while (currenttime < vsync);

        int x, y, s;
        for (y = 0; y < HEIGHT; y++)
        {
            for (x = 0; x < WIDTH; x++)
            {
                if (c8_get_pixel(ctx, x, y))
                {
                    for (s = 0; s < SCALE; s++)
                    {
                        display[y * WIDTH * SCALE + x + s] = COLOR;
                    }
                }
            }
        }

        SDL_UpdateTexture(texture, NULL, display, WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        c8_tick_60hz(ctx);
        vsync += 16667;
        // printf("vsync:%ld\n", vsync);
    }

    free(display);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
