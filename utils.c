#include "utils.h"

int new_double2D(double ***array, Uint32 rows, Uint32 cols)
{
    double **res = (double**)malloc(sizeof(double*) * rows);
    if (!res)
    {
        fprintf(stderr, "Error al alojar memoria para la simulacion\n");
        return 0;
    }

    for (int i = 0; i < rows; i++)
    {
        res[i] = (double*)malloc(sizeof(double) * cols);
        if (!res[i])
        {
            fprintf(stderr, "Error al alojar memoria para la simulacion\n");
            for (int j = 0; j < i; j++)
            {
                free(res[j]);
            }
            free(res);
            return 0;
        }
    }

    *array = res;

    return 1;
}

int fluid_setup(Fluid *f)
{
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

    double inTakeVelocity = 2.0f;
    for (int i = 0; i < f->height; i++)
    {
        for (int j = 0; j < f->width; j++)
        {
            f->u[i][j] = 0.0f;
            f->v[i][j] = 0.0f;
            f->m[i][j] = 1.0f;

            if (j == 0 || i == 0 || i == f->height - 1)
                f->scalar[i][j] = 0.0f;
            else
                f->scalar[i][j] = 1.0f;
            if (j == 1)
                f->u[i][j] = inTakeVelocity;
        }
    }

    int pipeHeight = 0.1f * f->height;
    int pipeLowerBound = 0.5f * (f->height - pipeHeight);
    int pipeHigherBound = 0.5f * (f->height + pipeHeight);

    for (int i = pipeLowerBound; i < pipeHigherBound; i++)
    {
        f->m[i][1] = 0.0f;
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
                f->v[i][j] += 0.001 * GRAVITY * deltaTime;
            }
        }
    }
}

void fluid_solveIncompressibility(Fluid* f, double deltaTime)
{
    double divergence, s;

    for (int i = 1; i < f->height - 1; i++)
    {
        for (int j = 1; j < f->width - 1; j++)
        {            
            if (s == 0.0f)
                continue;
            
            // account for obstacles
            s = f->scalar[i + 1][j] + f->scalar[i - 1][j] + f->scalar[i][j + 1] + f->scalar[i][j - 1];

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
    for (int i = 0; i < f->height; i++)
    {
        f->v[i][0] = f->v[i][1];
        f->v[i][f->width - 1] = f->v[i][f->width - 2];
    }

    for (int j = 0; j < f->width; j++)
    {
        f->u[0][j] = f->u[1][j];
        f->u[f->height - 1][j] = f->u[f->height - 2][j];
    }
}

int simulate(Fluid* f, double delta)
{
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
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    unsigned int cell_size = CELL_SIZE;
    SDL_Rect* rect = (SDL_Rect*)malloc(sizeof(SDL_Rect));
    if (!rect)
    {
        fprintf(stderr, "Error al dibujar :c\n");
    }
    rect->w = cell_size;
    rect->h = cell_size;

    Uint8 r, g, b;

    for (int i = 1; i < f->height - 1; i++)
    {
        for (int j = 1; j < f->width - 1; j++)
        {
            rect->x = (__WINDOW_W * .5) - (f->width * .5 - j + X_OFFSET) * cell_size;
            rect->y = (__WINDOW_H * .5) - (f->height * .5 - i + Y_OFFSET) * cell_size;

            r = (Uint8)(255.0f * f->m[i][j]);
            g = (Uint8)(255.0f * f->m[i][j]);
            b = (Uint8)(255.0f * f->m[i][j]);

            SDL_SetRenderDrawColor(renderer, r, g, b, 255);

            if (__DEBUG)
                printf("%.1f, ", f->m[i][j]);
            
            SDL_RenderFillRect(renderer, rect);
        }
        if (__DEBUG)
            printf("\n");
    }

    SDL_RenderPresent(renderer);
    return 0;
}

double sampleField(double x, double y, int field, Fluid *f)
{
    double result;

    double samples = 100.0f;
    double samples_rec = 1.0f / samples;
    double samples_half = 0.5f * samples;

    x = max(min(x, f->width * samples), samples);
    y = max(min(y, f->height * samples), samples);

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

    x0 = min(floor(x - dx) * samples_rec, f->width - 1);
    tx = ((x - dx) - x0 * samples) * samples_rec;
    x1 = min(x0 + 1, f->width - 1);

    y0 = min(floor(y - dy) * samples_rec, f->height - 1);
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
    delta *= .001;
    double samples = 100.0f;
    double samples_rec = 1.0f / samples;
    double samples_half = 0.5f * samples;

    double avg, x, y;

    for (int i = 1; i < f->height; i++)
    {
        for (int j = 1; j < f->width; j++)
        {
            f->newU[i][j] = f->u[i][j];
            f->newV[i][j] = f->v[i][j];

            //update u component
            if (f->scalar[i][j] != 0.0f && f->scalar[i][j - 1] && i < f->height - 1)
            {
                avg = (f->v[i][j - 1] + f->v[i][j + 1] + f->v[i - 1][j] + f->v[i + 1][j]) * .25;
                f->newU[i][j] = sampleField((double)j - delta * f->u[i][j], (double)i - delta * avg, U_FIELD, f);
            }
            //update v component
            if (f->scalar[i][j] != 0.0f && f->scalar[i][j - 1] && i < f->height - 1)
            {
                avg = (f->u[i][j - 1] + f->u[i][j + 1] + f->u[i - 1][j] + f->u[i + 1][j]) * .25;
                f->newV[i][j] = sampleField((double)j - delta * avg, (double)i - delta * f->v[i][j], V_FIELD, f);
            }
        }
    }

    double** tmp;

    tmp = f->v;
    f->v = f->newV;
    f->newV = tmp;

    tmp = f->u;
    f->u = f->newU;
    f->newU = tmp;
}

void fluid_advect_smoke(Fluid *f, double delta)
{
    delta *= .1;
    double samples = 100.0f;
    double samples_rec = 1.0f / samples;
    double samples_half = 0.5f * samples;

    double x, y, u, v;

    for (int i = 1; i < f->height - 1; i++)
    {
        for (int j = 1; j < f->width - 1; j++)
        {
            f->newM[i][j] = f->m[i][j];
            
            if (f->scalar[i][j] != 0.0)
            {
                u = (f->u[i][j] + f->u[i][j + 1]) * 0.5f;
                v = (f->v[i][j] + f->v[i + 1][j]) * 0.5f;

                x = i * samples + samples_half - delta * u;
                y = j * samples + samples_half - delta * v;

                f->newM[i][j] = sampleField(x, y, S_FIELD, f);
            }
        }
    }
    double** tmp = f->m;
    f->m = f->newM;
    f->newM = tmp;
}