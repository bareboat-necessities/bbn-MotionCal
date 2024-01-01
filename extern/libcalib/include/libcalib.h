#pragma once

#include "libcalib_common.h"
#include "libcalib_magcal.h"

namespace libcalib
{

class Calibrator
{
public:
	Calibrator()
		{ reset(); }

	void reset();
	void add_raw_data(const int16_t(&data)[9]);

	MagCalibrator m_magcal;
	Quaternion_t m_current_orientation;

private:
	int m_oversample_countdown;         // countdown of oversampling in add_raw_data()
	int m_force_orientation_countdown;  // countdown to reseting fusion in add_raw_data()

	AccelSensor_t m_accel;
	MagSensor_t   m_mag;
	GyroSensor_t  m_gyro;

	libcalib::Nxp m_fusion;

	static const int s_force_orientation_countdown_max = 240;
};

} // namespace libcalib
