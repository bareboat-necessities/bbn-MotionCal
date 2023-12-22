#pragma once

#include "libcalib_common.h"

namespace libcalib
{

struct mahony
{
			mahony()
			: m_q0(1.0f)
			, m_q1(0.0f)
			, m_q2(0.0f)
			, m_q3(0.0f)
				{ init(); }

	void	init();
	void	update(
				const AccelSensor_t *Accel,
				const MagSensor_t *Mag,
				const GyroSensor_t *Gyro,
				bool isBCurValid,
				float BCur);
	void	read(Quaternion_t* q);

private:
	void	update(float gx, float gy, float gz, float ax, float ay, float az, float mx, float my, float mz);
	void	updateIMU(float gx, float gy, float gz, float ax, float ay, float az);

	float	m_twoKp;		// 2 * proportional gain (Kp)
	float	m_twoKi;		// 2 * integral gain (Ki)
	float	m_q0;			// quaternion of sensor frame relative to auxiliary frame
	float	m_q1;
	float	m_q2;
	float	m_q3; 
	float	m_integralFBx;	// integral error terms scaled by Ki
	float	m_integralFBy;
	float	m_integralFBz;

	bool	m_reset_next_update;
};

} // namespace libcalib
