#pragma once

#include "libcalib_common.h"
#include "libcalib_nxp.h"
#include "libcalib_mahony.h"

// magnetic calibration & buffer structure

constexpr int MAGBUFFSIZE = 650; // Freescale's lib needs at least 392

class MagCalibration_t
{
public:

    MagCalibration_t();

	void reset();
	void add_raw_data(const int16_t (& data)[9], Quaternion_t* pResult);
	void apply_calibration(int16_t rawx, int16_t rawy, int16_t rawz, Point_t* out);
	bool get_new_calibration();

    float m_cal_V[3];                  // current hard iron offset x, y, z, (uT)
    float m_cal_invW[3][3];            // current inverse soft iron matrix
    float m_cal_B;                     // current geomagnetic field magnitude (uT)
    float m_errorFit;              // current fit error %
    int8_t m_isValid;          // integer value 0, 4, 7, 10 denoting both valid calibration and solver used
    int16_t m_aBpFast[3][MAGBUFFSIZE];   // uncalibrated magnetometer readings
    int8_t  m_aBpIsValid[MAGBUFFSIZE];        // 1=has data, 0=empty slot
    int16_t m_cBpIsValid;           // number of magnetometer readings
private:

	int choose_discard_magcal();
	void add_magcal_data(const int16_t(&data)[9]);

	void UpdateCalibration4INV();
	void UpdateCalibration7EIG();
	void UpdateCalibration10EIG();

    float m_errorFitAged;           // current fit error % (grows automatically with age)
    float m_calNext_V[3];                // trial value of hard iron offset z, y, z (uT)
    float m_calNext_invW[3][3];          // trial inverse soft iron matrix size
    float m_calNext_B;                   // trial value of geomagnetic field magnitude in uT
    float m_errorFitNext;          // trial value of fit error %
    float m_A[3][3];               // ellipsoid matrix A
    float m_invA[3][3];            // inverse of ellipsoid matrix A
    float m_matA[10][10];          // scratch 10x10 matrix used by calibration algorithms
    float m_matB[10][10];          // scratch 10x10 matrix used by calibration algorithms
    float m_vecA[10];              // scratch 10x1 vector used by calibration algorithms
    float m_vecB[4];               // scratch 4x1 vector used by calibration algorithms

	int m_rawcount;
	AccelSensor_t m_accel;
	MagSensor_t   m_mag;
	GyroSensor_t  m_gyro;

    libcalib::nxp m_fusion;
};


#define USE_NXP_FUSION
//#define USE_MAHONY_FUSION

void quality_reset(void);
void quality_update(const Point_t* point);
float quality_surface_gap_error(void);
float quality_magnitude_variance_error(void);
float quality_wobble_error(void);
