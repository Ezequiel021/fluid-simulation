#include "utils.h"

int new_float2D(float ***array, Uint32 rows, Uint32 cols) {
    float **ptrs = (float**)malloc(sizeof(float*) * rows);
    float *data = (float*)malloc(sizeof(float) * rows * cols);

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

    // Read configurations from file
    char buffer[128];
    char *endptr;
    fscanf(config, "%s", buffer);
    f->gravity = strtof(buffer, &endptr);
    fscanf(config, "%s", buffer);
    f->intake_speed = strtof(buffer, &endptr);
    fclose(config);

    printf("Gravity = %f\nIntake Velocity = %f\n", f->gravity, f->intake_speed);

    if (!new_float2D(&f->u, f->height, f->width))
    {
        fprintf(stderr, "Error al asignar memoria para los componentes horizontales\n");
        return 1;
    }

    if (!new_float2D(&f->v, f->height, f->width))
    {
        fprintf(stderr, "Error al asignar memoria para los componentes verticales\n");
        return 1;
    }

    if (!new_float2D(&f->newU, f->height, f->width))
    {
        fprintf(stderr, "Error al asignar memoria para el buffer de componentes horizontales\n");
        return 1;
    }

    if (!new_float2D(&f->newV, f->height, f->width))
    {
        fprintf(stderr, "Error al asignar memoria para el buffer de componentes verticales\n");
        return 1;
    }

    if (!new_float2D(&f->m, f->height, f->width))
    {
        fprintf(stderr, "Error al asignar memoria para la intensidad de los colores\n");
        return 1;
    }

    if (!new_float2D(&f->newM, f->height, f->width))
    {
        fprintf(stderr, "Error al asignar memoria para el buffer de intensidad del color\n");
        return 1;
    }

    if (!new_float2D(&f->scalar, f->height, f->width))
    {
        fprintf(stderr, "Error al asignar memoria para los coeficientes escalares\n");
        return 1;
    }

    for (int i = 0; i < f->height; i++)
    {
        for (int j = 0; j < f->width; j++)
        {
            f->u[i][j] = 0.0f;
            f->v[i][j] = 0.0f;
            f->m[i][j] = 0.0f; // <--- CAMBIO 1: Fondo negro (vacío) en lugar de 1.0

            // Change the boundary condition to keep the Right Wall (j == f->width - 1) open
            if (j == 0 || i == 0 || i == f->height - 1) // Removed "j == f->width - 1"
                f->scalar[i][j] = 0.0f;

            else if ((sqrtf(powf(0.25f * f->width - j, 2) + powf(0.5f * f->height - i , 2)) <= 10.0f) && 1) {
                f->scalar[i][j] = 0.0f;
            }
            else
                f->scalar[i][j] = 1.0f;

            // Inyectamos velocidad inicial
            if (j == 1)
                f->u[i][j] = f->intake_speed;
        }
    }
    return 0;
}

void fluid_integrate(Fluid *f, float deltaTime)
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

void fluid_solveIncompressibility(Fluid* f, float deltaTime)
{
    float s;

    for (int i = 1; i < f->height - 1; i++)
    {
        for (int j = 1; j < f->width - 1; j++)
        {            
            // account for obstacles
            s = f->scalar[i + 1][j] + f->scalar[i - 1][j] + f->scalar[i][j + 1] + f->scalar[i][j - 1];
            if (s == 0.0f) continue;
            
            // calculate divergence
            float divergence = f->v[i + 1][j] - f->v[i][j] + f->u[i][j + 1] - f->u[i][j];
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
        f->v[i][0] = f->v[i][1];
        f->u[i][0] = 0.0f;
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

int simulate(Fluid* f, float delta)
{
    // --- NUEVO CÓDIGO: REFORZAR LA ENTRADA CONSTANTE ---
    float inTakeVelocity = f->intake_speed; // Velocidad deseada
    int pipeHeight = 0.2f * f->height;
    int pipeLowerBound = 0.5f * (f->height - pipeHeight);
    int pipeHigherBound = 0.5f * (f->height + pipeHeight);

    for (int col = 0; col <= 4; col++)
    {
        for (int i = pipeLowerBound; i < pipeHigherBound; i++)
        {
            f->u[i][col] = inTakeVelocity; 
            f->m[i][col] = 1.0f; 
        }
    }

    fluid_integrate(f, delta);
    for (int i = 0; i < 30; i++)
        fluid_solveIncompressibility(f, delta);

    fluid_extrapolate(f);
    fluid_advect_velocity(f, delta);

    fluid_advect_smoke(f, delta);

    return 0;
}

int update(SDL_Renderer *renderer, float delta, Fluid *f, SDL_Texture *target, Uint32 *pixels)
{
    draw(renderer, f, target, pixels);

    simulate(f, delta);

    return 1;
}

int draw(SDL_Renderer *renderer, Fluid *f, SDL_Texture *target, Uint32 *pixels)
{
    SDL_Rect rect = {0, 0, WINDOW_W, WINDOW_H};

    Uint8 r, g, b, a;
    #pragma omp parallel for private(r, g, b, a) collapse(2)
    for (int i = 0; i < f->height; i++)
    {
        for (int j = 0; j < f->width; j++)
        {
            if (fabsf(f->scalar[i][j]) <= 0.01f)
            {
                r = 255;
                g = 0;
                b = 0;
                a = 255;
            }
            else
            {
                float val = f->m[i][j];
                if(val > 1.0) val = 1.0f;
                if(val < 0.0) val = 0.0f;

                r = (Uint8)(255.0f * val);
                //g = (i + j) % 2 ? 255 : 0;
                g = r;
                b = r;
                a = 255;
            }
            pixels[i * f->width + j] = (r << 24) | (g << 16) | (b << 8) | a;
        }
    }

    SDL_UpdateTexture(target, nullptr, pixels, f->width * sizeof(Uint32));
    SDL_RenderCopy(renderer, target, nullptr, &rect);

    return 0;
}

float sampleField(float x, float y, int field, Fluid *f)
{
    float samples = 100.0f;
    float samples_rec = 1.0f / samples;
    float samples_half = 0.5f * samples;

    x = max(min(x, f->width * samples), 0.0f);
    y = max(min(y, f->height * samples), 0.0f);

    float dx = 0.0f;
    float dy = 0.0f;

    int x0, x1, y0, y1;

    float** d;

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
        d = f->m;
        break;
    }

    //x0 = min(floor(x - dx) * samples_rec, f->width - 1);
    x0 = max(0, min(floor(x - dx) * samples_rec, f->width - 1));
    float tx = ((x - dx) - x0 * samples) * samples_rec;
    x1 = min(x0 + 1, f->width - 1);

    //y0 = min(floor(y - dy) * samples_rec, f->height - 1);
    y0 = max(0, min(floor(y - dy) * samples_rec, f->height - 1));
    float ty = ((y - dy) - y0 * samples) * samples_rec;
    y1 = min(y0 + 1, f->height - 1);

    float sx = 1.0f - tx;
    float sy = 1.0f - ty;

    float result = sx * sy * d[y0][x0] +
                    tx * sy * d[y0][x1] +
                    tx * ty * d[y1][x1] +
                    sx * ty * d[y1][x0];
                 
    return result;
}


void fluid_advect_velocity(Fluid *f, float delta)
{
    delta *= 0.001f; // Scale time for stability
    
    // Note: 'samples' logic is kept to match your original scaling style
    constexpr float samples = 100.0f;
    constexpr float samples_half = 0.5f * samples;

    float avg, x, y;

    #pragma omp parallel for private(x, y, avg) collapse(2)
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
                x = (float)j * samples - delta * f->u[i][j] * samples;
                y = (float)i * samples + samples_half - delta * avg * samples; // <--- ADDED + samples_half

                f->newU[i][j] = sampleField(x, y, U_FIELD, f);
            }

            // Update V component
            if (f->scalar[i][j] != 0.0f && f->scalar[i - 1][j] != 0.0f)
            {
                // Average U to the center of V face
                avg = (f->u[i][j - 1] + f->u[i][j + 1] + f->u[i - 1][j] + f->u[i + 1][j]) * 0.25f;
                
                // FIXED: x uses j, y uses i
                x = (float)j * samples + samples_half - delta * avg * samples; // <--- ADDED + samples_half
                y = (float)i * samples - delta * f->v[i][j] * samples;

                f->newV[i][j] = sampleField(x, y, V_FIELD, f);
            }
        }
    }

    // Swap pointers
    float** tmp;
    tmp = f->v; f->v = f->newV; f->newV = tmp;
    tmp = f->u; f->u = f->newU; f->newU = tmp;
}

void fluid_advect_smoke(Fluid *f, float delta)
{
    delta *= 0.1f; // Scale for smoke speed
    float samples = 100.0f;
    float samples_half = 0.5f * samples;

    float x, y, u, v;

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
    float** tmp = f->m;
    f->m = f->newM;
    f->newM = tmp;
}