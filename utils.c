#include "utils.h"

int new_double2D(double ***array, Uint32 rows, Uint32 cols) {
    double **ptrs = (double**)malloc(sizeof(double*) * rows);
    double *data = (double*)malloc(sizeof(double) * rows * cols);

    if (!ptrs || !data) return 0;
    for (int i = 0; i < rows; i++) {
        ptrs[i] = &data[i * cols]; 
    }

    *array = ptrs;
    return 1;
}

int fluid_setup(Fluid *f)
{
    FILE* config = fopen("config.txt", "r");
    if (!config)
    {
        fprintf(stderr, "Error al abrir el archivo de configuracion\n");
        return 1;
    }

    fscanf(config, "%lf", &f->gravity);
    fscanf(config, "%lf", &f->intake_speed);
    fclose(config);
    printf("Gravity = %f\nIntake Velocity = %f\n", f->gravity, f->intake_speed);

    if (__DEBUG)
        printf("Inicializando simulacion\n");

    if (!new_double2D(&f->u, f->height, f->width))
    {
        fprintf(stderr, "Error al asignar memoria para los componentes horizontales\n");
        return 1;
    }

    if (!new_double2D(&f->v, f->height, f->width))
    {
        fprintf(stderr, "Error al asignar memoria para los componentes verticales\n");
        return 1;
    }

    if (!new_double2D(&f->newU, f->height, f->width))
    {
        fprintf(stderr, "Error al asignar memoria para el buffer de componentes horizontales\n");
        return 1;
    }

    if (!new_double2D(&f->newV, f->height, f->width))
    {
        fprintf(stderr, "Error al asignar memoria para el buffer de componentes verticales\n");
        return 1;
    }

    if (!new_double2D(&f->m, f->height, f->width))
    {
        fprintf(stderr, "Error al asignar memoria para la intensidad de los colores\n");
        return 1;
    }

    if (!new_double2D(&f->newM, f->height, f->width))
    {
        fprintf(stderr, "Error al asignar memoria para el buffer de intensidad del color\n");
        return 1;
    }

    if (!new_double2D(&f->scalar, f->height, f->width))
    {
        fprintf(stderr, "Error al asignar memoria para los coeficientes escalares\n");
        return 1;
    }

    if (__DEBUG)
        printf("Memoria asignada con exito\n");

    // En utils.c -> fluid_setup
    double inTakeVelocity = 1.0f;
    for (int i = 0; i < f->height; i++)
    {
        for (int j = 0; j < f->width; j++)
        {
            f->u[i][j] = 0.0f;
            f->v[i][j] = 0.0f;
            f->m[i][j] = 0.0f; // <--- CAMBIO 1: Fondo negro (vacío) en lugar de 1.0

            // CAMBIO 2: Agregamos la pared DERECHA (j == f->width - 1) que faltaba
            if (j == 0 || j == f->width - 1 || i == 0 || i == f->height - 1)
                f->scalar[i][j] = 0.0f;
            else
                f->scalar[i][j] = 1.0f;
            
            // Inyectamos velocidad inicial
            if (j == 1)
                f->u[i][j] = inTakeVelocity;
        }
    }

    if (__DEBUG)
        printf("Inicializacion completada\n");

    return 0;
}

void fluid_integrate(Fluid *f, double deltaTime)
{
    for (int i = 1; i < f->height - 1; i++)
    {
        for (int j = 1; j < f->width - 1; j++)
        {
            if (f->scalar[i][j] != 0.0f && f->scalar[i - 1][j] != 0.0f)
            {
                f->v[i][j] += 0.001 * f->gravity * deltaTime;
            }
        }
    }
}

void fluid_solveIncompressibility(Fluid* f, double deltaTime)
{
    double divergence, s;

    #pragma omp parallel for
    for (int i = 1; i < f->height - 1; i++)
    {
        for (int j = 1; j < f->width - 1; j++)
        {            
            // account for obstacles
            s = f->scalar[i + 1][j] + f->scalar[i - 1][j] + f->scalar[i][j + 1] + f->scalar[i][j - 1];
            if (s == 0.0f) continue;
            
            // calculate divergence
            divergence = f->v[i + 1][j] - f->v[i][j] + f->u[i][j + 1] - f->u[i][j];
            divergence = (-divergence) / s;

            // overrelaxation-divergence
            divergence *= OVERRELAXATION_VAL;

            // sum vectors
            f->v[i][j] -= divergence * (f->scalar[i - 1][j]);
            f->v[i + 1][j] += divergence * (f->scalar[i + 1][j]);

            f->u[i][j] -= divergence * (f->scalar[i][j - 1]);
            f->u[i][j + 1] += divergence * (f->scalar[i][j + 1]);
        }
    }
}

void fluid_extrapolate(Fluid *f)
{
    // Límite Izquierdo y Derecho
    for (int i = 0; i < f->height; i++)
    {
        // En lugar de copiar ( f->v[i][1] ), forzamos 0
        f->v[i][0] = 0.0f; 
        f->v[i][f->width - 1] = 0.0f;
        
        f->u[i][0] = 0.0f;
        f->u[i][f->width - 1] = 0.0f;
    }

    // Límite Superior e Inferior
    for (int j = 0; j < f->width; j++)
    {
        f->u[0][j] = 0.0f;
        f->u[f->height - 1][j] = 0.0f;

        f->v[0][j] = 0.0f;
        f->v[f->height - 1][j] = 0.0f;
    }
}

int simulate(Fluid* f, double delta)
{
    // --- NUEVO CÓDIGO: REFORZAR LA ENTRADA CONSTANTE ---
    double inTakeVelocity = f->intake_speed; // Velocidad deseada
    int pipeHeight = 0.2f * f->height;
    int pipeLowerBound = 0.5f * (f->height - pipeHeight);
    int pipeHigherBound = 0.5f * (f->height + pipeHeight);

    for (int col = 1; col <= 4; col++) 
    {
        for (int i = pipeLowerBound; i < pipeHigherBound; i++)
        {
            f->u[i][col] = inTakeVelocity; 
            f->m[i][col] = 1.0f; 
        }
    }

    fluid_integrate(f, delta);
    if (__DEBUG) 
        printf("Integracion completada\n");

    for (int i = 0; i < 100; i++)
        fluid_solveIncompressibility(f, delta);
    if (__DEBUG) 
        printf("Proyeccion completada\n");

    fluid_extrapolate(f);
    if (__DEBUG) 
        printf("Extrapolacion completada\n");

    fluid_advect_velocity(f, delta);
    if (__DEBUG) 
        printf("Adveccion de la velocidad completada\n");

    fluid_advect_smoke(f, delta);
    if (__DEBUG) 
        printf("Adveccion del humo completada\n");

    return 0;
}

int update(SDL_Renderer *renderer, double delta, Fluid *f)
{
    if (__DEBUG)
        printf("Dibujando malla!\n");
    draw(renderer, f);

    if (__DEBUG)
        printf("Iniciando simulacion!\n");
    simulate(f, delta);

    if (__DEBUG)
        printf("Actualizacion completa!\n");

    return 1;
}

int draw(SDL_Renderer *renderer, Fluid *f)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
    SDL_RenderClear(renderer);

    int cell_size = CELL_SIZE;
    SDL_Rect rect;
    rect.w = cell_size;
    rect.h = cell_size;

    Uint8 intensity;

    // Calculate starting position to center the grid on screen
    int start_x = (__WINDOW_W / 2) - ((f->width * cell_size) / 2);
    int start_y = (__WINDOW_H / 2) - ((f->height * cell_size) / 2);

    for (int i = 1; i < f->height - 1; i++)
    {
        for (int j = 1; j < f->width - 1; j++)
        {
            // FIXED: Standard grid coordinates
            rect.x = start_x + (j * cell_size);
            rect.y = start_y + (i * cell_size);

            // Clamp values to prevent weird colors
            double val = f->m[i][j];
            if(val > 1.0) val = 1.0;
            if(val < 0.0) val = 0.0;

            intensity = (Uint8)(255.0f * val);

            // Draw white smoke on black bg, or blue fluid
            SDL_SetRenderDrawColor(renderer, 0, intensity, 0, 255);
            SDL_RenderFillRect(renderer, &rect);
            
            // Optional: Draw Walls (Scalar = 0) as Red
            if(f->scalar[i][j] == 0.0f) {
                 SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                 SDL_RenderFillRect(renderer, &rect);
            }
        }
    }

    return 0;
}

double sampleField(double x, double y, int field, Fluid *f)
{
    double result;

    double samples = 100.0f;
    double samples_rec = 1.0f / samples;
    double samples_half = 0.5f * samples;

    x = max(min(x, f->width * samples), 0.0f);
    y = max(min(y, f->height * samples), 0.0f);

    double dx = 0.0f;
    double dy = 0.0f;

    int x0, x1, y0, y1;
    double tx, ty, sx, sy;

    double** d;

    switch (field)
    {
    case U_FIELD:
        dy = samples_half;
        d = f->u;
        break;
        
    case V_FIELD:
        dx = samples_half;
        d = f->v;
        break;

    case S_FIELD:
        dx = samples_half;
        dy = samples_half;
        d = f->m;
        break;

    default:
        break;
    }

    //x0 = min(floor(x - dx) * samples_rec, f->width - 1);
    x0 = max(0, min(floor(x - dx) * samples_rec, f->width - 1));
    tx = ((x - dx) - x0 * samples) * samples_rec;
    x1 = min(x0 + 1, f->width - 1);

    //y0 = min(floor(y - dy) * samples_rec, f->height - 1);
    y0 = max(0, min(floor(y - dy) * samples_rec, f->height - 1));
    ty = ((y - dy) - y0 * samples) * samples_rec;
    y1 = min(y0 + 1, f->height - 1);

    sx = 1.0f - tx;
    sy = 1.0f - ty;

    result = sx * sy * d[y0][x0] + 
             tx * sy * d[y0][x1] + 
             tx * ty * d[y1][x1] + 
             sx * ty * d[y1][x0];
                 
    return result;
}


void fluid_advect_velocity(Fluid *f, double delta)
{
    delta *= .001; // Scale time for stability
    
    // Note: 'samples' logic is kept to match your original scaling style
    double samples = 100.0f;
    double samples_half = 0.5f * samples;

    double avg, x, y;

    for (int i = 1; i < f->height - 1; i++) // Bounds check -1 to stay in grid
    {
        for (int j = 1; j < f->width - 1; j++)
        {
            f->newU[i][j] = f->u[i][j];
            f->newV[i][j] = f->v[i][j];

            // Update U component
            // Check neighbors for walls
            if (f->scalar[i][j] != 0.0f && f->scalar[i][j - 1] != 0.0f)
            {
                // Average V to the center of U face
                avg = (f->v[i][j - 1] + f->v[i][j + 1] + f->v[i - 1][j] + f->v[i + 1][j]) * 0.25f;
                
                // FIXED: x uses j, y uses i
                x = (double)j * samples - delta * f->u[i][j] * samples;
                y = (double)i * samples - delta * avg * samples;
                
                f->newU[i][j] = sampleField(x, y, U_FIELD, f);
            }

            // Update V component
            if (f->scalar[i][j] != 0.0f && f->scalar[i - 1][j] != 0.0f)
            {
                // Average U to the center of V face
                avg = (f->u[i][j - 1] + f->u[i][j + 1] + f->u[i - 1][j] + f->u[i + 1][j]) * 0.25f;
                
                // FIXED: x uses j, y uses i
                x = (double)j * samples - delta * avg * samples;
                y = (double)i * samples - delta * f->v[i][j] * samples;
                
                f->newV[i][j] = sampleField(x, y, V_FIELD, f);
            }
        }
    }

    // Swap pointers
    double** tmp;
    tmp = f->v; f->v = f->newV; f->newV = tmp;
    tmp = f->u; f->u = f->newU; f->newU = tmp;
}

void fluid_advect_smoke(Fluid *f, double delta)
{
    delta *= .1; // Scale for smoke speed
    double samples = 100.0f;
    double samples_half = 0.5f * samples;

    double x, y, u, v;

    for (int i = 1; i < f->height - 1; i++)
    {
        for (int j = 1; j < f->width - 1; j++)
        {
            // Default to fading old value slightly or keeping it
            f->newM[i][j] = f->m[i][j];
            
            if (f->scalar[i][j] != 0.0)
            {
                u = (f->u[i][j] + f->u[i][j + 1]) * 0.5f;
                v = (f->v[i][j] + f->v[i + 1][j]) * 0.5f;

                // FIXED: x comes from j (width), y comes from i (height)
                // We backtrace: position - velocity * time
                x = j * samples + samples_half - delta * u * samples;
                y = i * samples + samples_half - delta * v * samples;

                f->newM[i][j] = sampleField(x, y, S_FIELD, f);
            }
        }
    }
    double** tmp = f->m;
    f->m = f->newM;
    f->newM = tmp;
}