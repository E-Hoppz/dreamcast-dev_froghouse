#ifndef FROG_MATH_H
#define FROG_MATH_H

#include <stdbool.h>

float frog_lerp(float a, float b, float t);
float frog_clamp(float val, float min, float max);
float frog_math_colani_lerp(float a, float b, float t);
float frog_math_map_signal(float val, float a, float b);
float frog_math_wrap(float val, float min, float max);

// [NEW] Catmull-Rom Cubic Spline Interpolation for smooth path tracking
float frog_math_catmull_rom(float p0, float p1, float p2, float p3, float t);

// [NEW] 2D Point-in-Box check for Point-and-Click menus
bool frog_math_point_in_box(float px, float py, float bx, float by, float bw, float bh);

#endif
