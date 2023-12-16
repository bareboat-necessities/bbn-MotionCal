#pragma once

#include <stdint.h>

#ifndef M_PI
// Source: http://www.geom.uiuc.edu/~huberty/math5337/groupe/digits.html
constexpr float M_PI = 3.141592653589793238462643383279502884197169399375105820974944592307816406;
#endif

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

// magnetic calibration & buffer structure

constexpr int MAGBUFFSIZE = 650; // Freescale's lib needs at least 392

class MagCalibration_t
{
public:

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
};


constexpr int SENSORFS = 100;
constexpr int OVERSAMPLE_RATIO = 4;


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


#define USE_NXP_FUSION
//#define USE_MAHONY_FUSION

void fusion_init(void);
void fusion_update(const AccelSensor_t* Accel, const MagSensor_t* Mag, const GyroSensor_t* Gyro,
    const MagCalibration_t* MagCal);
void fusion_read(Quaternion_t* q);

void quality_reset(void);
void quality_update(const Point_t* point);
float quality_surface_gap_error(void);
float quality_magnitude_variance_error(void);
float quality_wobble_error(void);

void raw_data(const int16_t* data);


