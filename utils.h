#ifndef UTILS_H
#define UTILS_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <stdio.h>
#include <omp.h>
#include <string.h>

#define __DEBUG 0
#define __SHOW_FPS 0

#define WINDOW_W (1000)
#define WINDOW_H (450)
#define __RENDER_SCALE 1.0f
#define FRAMETIME 66

#define CELL_T_BORDER 0
#define CELL_T_FLUID 1
#define CELL_SIZE 5

#define X_OFFSET 1
#define Y_OFFSET 0

#define GRID_WIDTH 200
#define GRID_HEIGHT 90

#define GRAVITY 10.0f
#define OVERRELAXATION_VAL 1.9f

//No modificar, estas macros permiten elegir el tipo de muestrado usado en la funcion sample_field()
#define U_FIELD 1
#define V_FIELD 2
#define S_FIELD 3

#define max(a,b) \
   ({   __typeof__ (a) _a = (a); \
        __typeof__ (b) _b = (b); \
        _a > _b ? _a : _b; })
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

typedef struct COLOR
{
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
} Color;

typedef struct FLUID
{
    float intake_speed;
    float gravity;
    
    float** u;
    float** v;
    float** newU;
    float** newV;
    float** scalar;
    float** m;
    float** newM;

    // Grid height
    Uint32 height;
    // Grid width
    Uint32 width;

} Fluid;

int new_float2D(float*** array, Uint32 rows, Uint32 cols);

int fluid_setup(Fluid* f);

void fluid_integrate(Fluid* f, float deltaTime);

void fluid_extrapolate(Fluid* f);

int simulate(Fluid* f, float delta);

int update(SDL_Renderer* renderer, float delta, Fluid* f, SDL_Texture *target, Uint32 *pixels);

int draw(SDL_Renderer* renderer, Fluid* f, SDL_Texture *target, Uint32 *pixels);

float sampleField(float x, float y, int field, Fluid *f);

void fluid_advect_velocity(Fluid* f, float delta);

void fluid_advect_smoke(Fluid* f, float delta);
#endif // UTILS_H
