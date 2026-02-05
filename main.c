#include "utils.h"

int draw_fps(SDL_Renderer* renderer, TTF_Font* font, float fps)
{
    char str[] = "FPS: 0000.0";
    sprintf(str, "FPS: %.1f", fps);

    SDL_Color color = {127, 127, 127, 255};
    SDL_Surface *surface = TTF_RenderText_Blended(font, str, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect label = {0, 0, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &label);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    return 1;
}

int main(int argc, char **argv)
{
    int show_fps = 0;
    if (argc > 1 && !strcmp(argv[1], "--fps"))
    {
        show_fps = 1;
    }

    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_CreateWindowAndRenderer(WINDOW_W, WINDOW_H, 0, &window, &renderer);
    SDL_RenderSetScale(renderer, __RENDER_SCALE, __RENDER_SCALE);

    SDL_SetWindowTitle(window, "Fluidos - Elementos de CC");

    SDL_RenderClear(renderer);

    SDL_Event eventHandler;

    Fluid fluid;
    fluid.height = GRID_HEIGHT + 2;
    fluid.width = GRID_WIDTH + 2;

    if (fluid_setup(&fluid))
    {
        fprintf(stderr, "Error al crear la malla. (L24)\n");
        SDL_Quit();
    }

    int isRunning = 1;
    int isPaused = 0;
    unsigned long NOW = SDL_GetPerformanceCounter();
    Uint64 LAST = 0;
    float deltaTime = 0;

    TTF_Font *font = TTF_OpenFont("roboto.ttf", 24);

    SDL_Texture *target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, fluid.width, fluid.height);

    Uint32 *pixels = (Uint32 *) malloc(sizeof(Uint32) * fluid.width * fluid.height);

    while (isRunning)
    {
        LAST = NOW;
        NOW = SDL_GetPerformanceCounter();
        deltaTime = (float)((NOW - LAST) * 1000 / (float)SDL_GetPerformanceFrequency());
        if (show_fps)
            printf("%f - %.1f fps   \r", deltaTime, 1000.0f / deltaTime);


        while (SDL_PollEvent(&eventHandler)) {
            switch (eventHandler.type)
            {
                case (SDL_KEYUP):
                    switch (eventHandler.key.keysym.scancode)
                    {
                    case SDL_SCANCODE_P:
                            isPaused = !isPaused;
                            if (!show_fps)
                                printf("%s\n", isPaused ? "Pausa" : "Reanudando...");
                            break;
                    case SDL_SCANCODE_Q:
                            isRunning = 0;
                    default:
                            break;
                    }
                    break;

                case (SDL_QUIT):
                    isRunning = 0;
                    break;
                default:
                    break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        update(renderer, deltaTime, &fluid, target, pixels);
        draw_fps(renderer, font, 1000.0 / deltaTime);
        SDL_RenderPresent(renderer);
    }

    // Free U
    free(fluid.u[0]); // Free the data block
    free(fluid.u);    // Free the row pointers

    // Free V
    free(fluid.v[0]);
    free(fluid.v);

    // ... You must also free these: ...
    free(fluid.newU[0]); free(fluid.newU);
    free(fluid.newV[0]); free(fluid.newV);
    free(fluid.m[0]);    free(fluid.m);
    free(fluid.newM[0]); free(fluid.newM);
    free(fluid.scalar[0]); free(fluid.scalar);

    SDL_DestroyTexture(target);

    printf("\n");
    TTF_Quit();
    SDL_Quit();

    return 0;
}