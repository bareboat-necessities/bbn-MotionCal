#pragma once

#include "libcalib_common.h"

// 9DOF Kalman filter accelerometer, magnetometer and gyroscope state vector structure
namespace libcalib
{

struct Nxp
{
			Nxp()
				{ init(); }

	void	init();
	void	update(
				const AccelSensor_t* Accel,
				const MagSensor_t* Mag,
				const GyroSensor_t* Gyro,
				bool isBCurValid,
				float BCur);
	void	read(Quaternion_t* q);

private:
	// start: elements common to all motion state vectors
	// Euler angles
	float m_PhiPl;			// roll (deg)
	float m_ThePl;			// pitch (deg)
	float m_PsiPl;			// yaw (deg)
	float m_RhoPl;			// compass (deg)
	float m_ChiPl;			// tilt from vertical (deg)
	// orientation matrix, quaternion and rotation vector
	float m_RPl[3][3];		// a posteriori orientation matrix
	Quaternion_t m_qPl;		// a posteriori orientation quaternion
	float m_RVecPl[3];		// rotation vector
	// angular velocity
	float m_Omega[3];		// angular velocity (deg/s)
	// systick timer for benchmarking
	int32_t m_systick;		// systick timer;
	// end: elements common to all motion state vectors

	// elements transmitted over bluetooth in kalman packet
	float m_bPl[3];			// gyro offset (deg/s)
	float m_ThErrPl[3];		// orientation error (deg)
	float m_bErrPl[3];		// gyro offset error (deg/s)
	// end elements transmitted in kalman packet

	float m_dErrGlPl[3];	// magnetic disturbance error (uT, global frame)
	float m_dErrSePl[3];	// magnetic disturbance error (uT, sensor frame)
	float m_aErrSePl[3];	// linear acceleration error (g, sensor frame)
	float m_aSeMi[3];		// linear acceleration (g, sensor frame)
	float m_DeltaPl;		// inclination angle (deg)
	float m_aSePl[3];		// linear acceleration (g, sensor frame)
	float m_aGlPl[3];		// linear acceleration (g, global frame)
	float m_gErrSeMi[3];	// difference (g, sensor frame) of gravity vector (accel) and gravity vector (gyro)
	float m_mErrSeMi[3];	// difference (uT, sensor frame) of geomagnetic vector (magnetometer) and geomagnetic vector (gyro)
	float m_gSeGyMi[3];		// gravity vector (g, sensor frame) measurement from gyro
	float m_mSeGyMi[3];		// geomagnetic vector (uT, sensor frame) measurement from gyro
	float m_mGl[3];			// geomagnetic vector (uT, global frame)
	float m_QvAA;			// accelerometer terms of Qv
	float m_QvMM;			// magnetometer terms of Qv
	float m_PPlus12x12[12][12];	// covariance matrix P+
	float m_K12x6[12][6];	// kalman filter gain matrix K
	float m_Qw12x12[12][12];// covariance matrix Qw
	float m_C6x12[6][12];	// measurement matrix C
	float m_RMi[3][3];		// a priori orientation matrix
	Quaternion_t m_Deltaq;	// delta quaternion
	Quaternion_t m_qMi;		// a priori orientation quaternion
	float m_casq;			// FCA * FCA;
	float m_cdsq;			// FCD * FCD;
	float m_Fastdeltat;		// sensor sampling interval (s) = 1 / SENSORFS
	float m_deltat;			// kalman filter sampling interval (s) = OVERSAMPLE_RATIO / SENSORFS
	float m_deltatsq;		// fdeltat * fdeltat
	float m_QwbplusQvG;		// FQWB + FQVG
	int16_t m_FirstOrientationLock;	// denotes that 9DOF orientation has locked to 6DOF
	int8_t m_resetflag;		// flag to request re-initialization on next pass
};

} // namespace libcalib
