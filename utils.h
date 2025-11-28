#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>

#define __DEBUG 0
#define __SHOW_FPS 0

#define __WINDOW_W 640 * 2
#define __WINDOW_H 480 * 2
#define __RENDER_SCALE 1.0f
#define __FRAMETIME 66

#define CELL_T_BORDER 0
#define CELL_T_FLUID 1
#define CELL_SIZE 5

#define X_OFFSET 1
#define Y_OFFSET 0

#define GRID_WIDTH 160
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
    double intake_speed;
    double gravity;
    
    double** u;
    double** v;
    double** newU;
    double** newV;
    double** scalar;
    double** m;
    double** newM;

    Uint32 height;
    Uint32 width;

} Fluid;

int new_double2D(double*** array, Uint32 rows, Uint32 cols);

int fluid_setup(Fluid* f);

void fluid_integrate(Fluid* f, double deltaTime);

void fluid_extrapolate(Fluid* f);

int simulate(Fluid* f, double delta);

int update(SDL_Renderer* renderer, double delta, Fluid* f);

int draw(SDL_Renderer* renderer, Fluid* f);

double sampleField(double x, double y, int field, Fluid *f);

void fluid_advect_velocity(Fluid* f, double delta);

void fluid_advect_smoke(Fluid* f, double delta);