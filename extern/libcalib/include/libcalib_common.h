#pragma once

#include <stdint.h>

#ifndef M_PI
// Source: http://www.geom.uiuc.edu/~huberty/math5337/groupe/digits.html
constexpr float M_PI = 3.141592653589793238462643383279502884197169399375105820974944592307816406;
#endif

constexpr int SENSORFS = 100;
constexpr int OVERSAMPLE_RATIO = 4;

struct Point_t
{
	float x;
	float y;
	float z;
	//int valid;
};

struct Quaternion_t
{
	float q0; // w
	float q1; // x
	float q2; // y
	float q3; // z
};

// accelerometer sensor structure definition
constexpr float G_PER_COUNT = 0.0001220703125F;  // = 1/8192
struct AccelSensor_t
{
	float Gp[3];           // slow (typically 25Hz) averaged readings (g)
	float GpFast[3];       // fast (typically 200Hz) readings (g)
};

// magnetometer sensor structure definition
constexpr float UT_PER_COUNT = 0.1F;
struct MagSensor_t
{
	float Bc[3];           // slow (typically 25Hz) averaged calibrated readings (uT)
	float BcFast[3];       // fast (typically 200Hz) calibrated readings (uT)
};

// gyro sensor structure definition
constexpr float DEG_PER_SEC_PER_COUNT = 0.0625F;  // = 1/16
struct GyroSensor_t
{
	float Yp[3];                           // raw gyro sensor output (deg/s)
	float YpFast[OVERSAMPLE_RATIO][3];     // fast (typically 200Hz) readings
};
