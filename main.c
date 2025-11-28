#include "utils.h"

int main(int argc, char **argv)
{
    int show_fps = 0, debug = 0;
    if (argc > 1 && !strcmp(argv[1], "--fps"))
    {
        show_fps = 1;
    }
    if (argc > 1 && !strcmp(argv[1], "--debug"))
    {
        debug = 1;
    }

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(__WINDOW_W, __WINDOW_H, 0, &window, &renderer);
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
    if (__DEBUG)
        printf("Generacion de malla basica, Completada!\n");

    int isRunning = 1;
    int isPaused = 0;
    int step = 1;

    unsigned long NOW = SDL_GetPerformanceCounter();
    Uint64 LAST = 0;
    double deltaTime = 0;

    if (__DEBUG)
        printf("Deltatime iniciado con exito!\n");

    while (isRunning)
    {
        LAST = NOW;
        NOW = SDL_GetPerformanceCounter();
        deltaTime = (double)((NOW - LAST) * 1000 / (double)SDL_GetPerformanceFrequency());
        if (show_fps)
            printf("%f - %.1f fps   \r", deltaTime, 1000.0f / deltaTime);

        SDL_PollEvent(&eventHandler);
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
            case SDL_SCANCODE_M:
                step = 1;
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

        if (!isPaused)
        {
            update(renderer, deltaTime, &fluid);
        }
        else if (step)
        {
            update(renderer, deltaTime, &fluid);
            if (!show_fps)printf("Avanzando un paso\n");
            isPaused = 1;
            step = 0;
        }
    }

    printf("\n");
    SDL_Quit();

    return 0;
}
