#pragma once
#include "iostream"
#include "windows.h"
#include "fstream"
#include "vector"
#include "chrono"
#include <time.h>
#include <cmath>
#include <algorithm>

#define PI 3.14159265358979323846 
void CoordTransform(long nPix, long nLine, double& fX, double& fY, double a);
void BilenearInterp(double fX, double fY, int width, int height, long newWidth, long newHeight, BYTE* buffer_ch, BYTE* buffer_rotation, int i, int j);

struct Points
{
	long x, y;
};
