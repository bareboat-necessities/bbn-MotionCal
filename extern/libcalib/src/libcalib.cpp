#include "libcalib.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C"{
#endif

MagCalibration_t magcal;

Quaternion_t current_orientation;

void apply_calibration(int16_t rawx, int16_t rawy, int16_t rawz, Point_t* out)
{
	float x, y, z;

	x = ((float)rawx * UT_PER_COUNT) - magcal.m_cal_V[0];
	y = ((float)rawy * UT_PER_COUNT) - magcal.m_cal_V[1];
	z = ((float)rawz * UT_PER_COUNT) - magcal.m_cal_V[2];
	out->x = x * magcal.m_cal_invW[0][0] + y * magcal.m_cal_invW[0][1] + z * magcal.m_cal_invW[0][2];
	out->y = x * magcal.m_cal_invW[1][0] + y * magcal.m_cal_invW[1][1] + z * magcal.m_cal_invW[1][2];
	out->z = x * magcal.m_cal_invW[2][0] + y * magcal.m_cal_invW[2][1] + z * magcal.m_cal_invW[2][2];
}

#ifdef __cplusplus
} // extern "C"
#endif
