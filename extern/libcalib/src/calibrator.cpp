#include "libcalib.h"

#include <stdio.h>

void MagCalibration_t::apply_calibration(int16_t rawx, int16_t rawy, int16_t rawz, Point_t* out)
{
	float x, y, z;

	x = ((float)rawx * UT_PER_COUNT) - m_cal_V[0];
	y = ((float)rawy * UT_PER_COUNT) - m_cal_V[1];
	z = ((float)rawz * UT_PER_COUNT) - m_cal_V[2];
	out->x = x * m_cal_invW[0][0] + y * m_cal_invW[0][1] + z * m_cal_invW[0][2];
	out->y = x * m_cal_invW[1][0] + y * m_cal_invW[1][1] + z * m_cal_invW[1][2];
	out->z = x * m_cal_invW[2][0] + y * m_cal_invW[2][1] + z * m_cal_invW[2][2];
}
