#include "libcalib.h"

#include <math.h>

namespace libcalib
{

void Calibrator::reset()
{
	m_magcal.reset();

	m_current_orientation.q0 = 1.0f;
	m_current_orientation.q1 = 0.0f;
	m_current_orientation.q2 = 0.0f;
	m_current_orientation.q3 = 0.0f;

	m_oversample_countdown = OVERSAMPLE_RATIO;
	m_force_orientation_countdown = s_force_orientation_countdown_max;

	m_accel = { 0 };
	m_mag = { 0 };
	m_gyro = { 0 };

	m_fusion.init();
}

void Calibrator::add_raw_data(const int16_t(&data)[9])
{
	float x, y, z, ratio, magdiff;
	Point_t point;

	m_magcal.add_magcal_data(data);
	x = m_magcal.m_cal_V[0];
	y = m_magcal.m_cal_V[1];
	z = m_magcal.m_cal_V[2];
	if (m_magcal.get_new_calibration()) {
		x -= m_magcal.m_cal_V[0];
		y -= m_magcal.m_cal_V[1];
		z -= m_magcal.m_cal_V[2];
		magdiff = sqrtf(x * x + y * y + z * z);
		//printf("magdiff = %.2f\n", magdiff);
		if (magdiff > 0.8f) {
			m_fusion.init();
			m_oversample_countdown = OVERSAMPLE_RATIO;
			m_force_orientation_countdown = s_force_orientation_countdown_max;
		}
	}

	if (m_force_orientation_countdown > 0) {
		if (--m_force_orientation_countdown == 0) {
			//printf("delayed forcible orientation reset\n");
			m_fusion.init();
			m_oversample_countdown = OVERSAMPLE_RATIO;
		}
	}

	if (m_oversample_countdown >= OVERSAMPLE_RATIO) {
		m_accel = { 0 };
		m_mag = { 0 };
		m_gyro = { 0 };
		m_oversample_countdown = 0;
	}
	x = (float)data[0] * G_PER_COUNT;
	y = (float)data[1] * G_PER_COUNT;
	z = (float)data[2] * G_PER_COUNT;
	m_accel.GpFast[0] = x;
	m_accel.GpFast[1] = y;
	m_accel.GpFast[2] = z;
	m_accel.Gp[0] += x;
	m_accel.Gp[1] += y;
	m_accel.Gp[2] += z;

	x = (float)data[3] * DEG_PER_SEC_PER_COUNT;
	y = (float)data[4] * DEG_PER_SEC_PER_COUNT;
	z = (float)data[5] * DEG_PER_SEC_PER_COUNT;
	m_gyro.Yp[0] += x;
	m_gyro.Yp[1] += y;
	m_gyro.Yp[2] += z;
	m_gyro.YpFast[m_oversample_countdown][0] = x;
	m_gyro.YpFast[m_oversample_countdown][1] = y;
	m_gyro.YpFast[m_oversample_countdown][2] = z;

	m_magcal.apply_calibration(data[6], data[7], data[8], &point);
	m_mag.BcFast[0] = point.x;
	m_mag.BcFast[1] = point.y;
	m_mag.BcFast[2] = point.z;
	m_mag.Bc[0] += point.x;
	m_mag.Bc[1] += point.y;
	m_mag.Bc[2] += point.z;

	m_oversample_countdown++;
	if (m_oversample_countdown >= OVERSAMPLE_RATIO) {
		ratio = 1.0f / (float)OVERSAMPLE_RATIO;
		m_accel.Gp[0] *= ratio;
		m_accel.Gp[1] *= ratio;
		m_accel.Gp[2] *= ratio;
		m_gyro.Yp[0] *= ratio;
		m_gyro.Yp[1] *= ratio;
		m_gyro.Yp[2] *= ratio;
		m_mag.Bc[0] *= ratio;
		m_mag.Bc[1] *= ratio;
		m_mag.Bc[2] *= ratio;
		m_fusion.update(&m_accel, &m_mag, &m_gyro, m_magcal.m_isValid, m_magcal.m_cal_B);
		m_fusion.read(&m_current_orientation);
	}
}

} // namespace libcalib
