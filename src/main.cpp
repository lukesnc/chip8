#include <SDL2/SDL.h>
#include <chrono>
#include <iostream>
#include <thread>

#include "chip8.h"

#define CYCLE 2000
#define SPEEDUP_MOD 20
#define WIDTH 1024
#define HEIGHT 512

/* Keypad
 * |1|2|3|4|
 * |q|w|e|r|
 * |a|s|d|f|
 * |z|x|c|v|
 */
unsigned char keymap[16] = {
    SDLK_x,
    SDLK_1,
    SDLK_2,
    SDLK_3,
    SDLK_q,
    SDLK_w,
    SDLK_e,
    SDLK_a,
    SDLK_s,
    SDLK_d,
    SDLK_z,
    SDLK_c,
    SDLK_4,
    SDLK_r,
    SDLK_f,
    SDLK_v
};

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cout << "Usage: ./chip8 [ROM]\n";
        return 1;
    }

    // Create window
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT);

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);

    unsigned int frame_buffer[64 * 32];

    // Load rom
    Chip8 myChip8 = Chip8();
    bool speedup = false;

load:
    if (!myChip8.load(argv[1]))
        return 1;

    // Emulation cycle
    for (;;) {
        myChip8.exec();

        // Check keyboard
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
                SDL_DestroyTexture(texture);
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 0;
            }

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE)
                speedup = !speedup;

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F1) {
                std::cout << "RESET\n";
                goto load;
            }

            if (e.type == SDL_KEYDOWN) {
                for (int i = 0; i < 16; i++) {
                    if (e.key.keysym.sym == keymap[i])
                        myChip8.key[i] = 1;
                }
            }

            if (e.type == SDL_KEYUP) {
                for (int i = 0; i < 16; i++) {
                    if (e.key.keysym.sym == keymap[i])
                        myChip8.key[i] = 0;
                }
            }
        }

        // Update screen
        if (myChip8.draw_flag) {
            myChip8.draw_flag = false;

            // Copy screen into frame buffer
            for (int i = 0; i < 2048; i++)
                frame_buffer[i] = (0x00FFFFFF * myChip8.gfx[i]) | 0xFF000000;

            SDL_UpdateTexture(texture, NULL, frame_buffer, 64 * sizeof(unsigned int));

            // Render
            SDL_RenderClear(renderer); // Clear screen
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }

        std::this_thread::sleep_for(std::chrono::microseconds(speedup ? (CYCLE / SPEEDUP_MOD) : CYCLE));
    }

    return 0;
}