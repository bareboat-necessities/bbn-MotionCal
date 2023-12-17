#include "libcalib.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#define random() rand()
#endif


MagCalibration_t::MagCalibration_t()
: m_cal_V()
, m_cal_invW()
, m_cal_B()
, m_errorFit()
, m_isValid()
, m_aBpFast()
, m_aBpIsValid()
, m_cBpIsValid()
, m_errorFitAged()
, m_calNext_V()
, m_calNext_invW()
, m_calNext_B()
, m_errorFitNext()
, m_A()
, m_invA()
, m_matA()
, m_matB()
, m_vecA()
, m_vecB()
, m_rawcount(OVERSAMPLE_RATIO)
, m_accel()
, m_mag()
, m_gyro()
, m_fusion()
{
	m_cal_V[2] = 80.0f;  // initial guess
	m_cal_invW[0][0] = 1.0f;
	m_cal_invW[1][1] = 1.0f;
	m_cal_invW[2][2] = 1.0f;
	m_errorFit = 100.0f;
	m_errorFitAged = 100.0f;
	m_cal_B = 50.0f;
}

void MagCalibration_t::reset()
{
	*this = MagCalibration_t();
}

int MagCalibration_t::choose_discard_magcal()
{
	int32_t rawx, rawy, rawz;
	int32_t dx, dy, dz;
	float x, y, z;
	uint64_t distsq, minsum=0xFFFFFFFFFFFFFFFFull;
	static int runcount=0;
	int i, j, minindex=0;
	Point_t point;
	float gaps, field, error, errormax;

	// When enough data is collected (gaps error is low), assume we
	// have a pretty good coverage and the field stregth is known.
	gaps = quality_surface_gap_error();
	if (gaps < 25.0f) {
		// occasionally look for points farthest from average field strength
		// always rate limit assumption-based data purging, but allow the
		// rate to increase as the angular coverage improves.
		if (gaps < 1.0f) gaps = 1.0f;
		if (++runcount > (int)(gaps * 10.0f)) {
			j = MAGBUFFSIZE;
			errormax = 0.0f;
			for (i=0; i < MAGBUFFSIZE; i++) {
				rawx = m_aBpFast[0][i];
				rawy = m_aBpFast[1][i];
				rawz = m_aBpFast[2][i];
				apply_calibration(rawx, rawy, rawz, &point);
				x = point.x;
				y = point.y;
				z = point.z;
				field = sqrtf(x * x + y * y + z * z);
				// if m_cal_B is bad, things could go horribly wrong
				error = fabsf(field - m_cal_B);
				if (error > errormax) {
					errormax = error;
					j = i;
				}
			}
			runcount = 0;
			if (j < MAGBUFFSIZE) {
				//printf("worst error at %d\n", j);
				return j;
			}
		}
	} else {
		runcount = 0;
	}
	// When solid info isn't availabe, find 2 points closest to each other,
	// and randomly discard one.  When we don't have good coverage, this
	// approach tends to add points into previously unmeasured areas while
	// discarding info from areas with highly redundant info.
	for (i=0; i < MAGBUFFSIZE; i++) {
		for (j=i+1; j < MAGBUFFSIZE; j++) {
			dx = m_aBpFast[0][i] - m_aBpFast[0][j];
			dy = m_aBpFast[1][i] - m_aBpFast[1][j];
			dz = m_aBpFast[2][i] - m_aBpFast[2][j];
			distsq = (int64_t)dx * (int64_t)dx;
			distsq += (int64_t)dy * (int64_t)dy;
			distsq += (int64_t)dz * (int64_t)dz;
			if (distsq < minsum) {
				minsum = distsq;
				minindex = (random() & 1) ? i : j;
			}
		}
	}
	return minindex;
}


void MagCalibration_t::add_magcal_data(const int16_t(&data)[9])
{
	int i;

	// first look for an unused caldata slot
	for (i=0; i < MAGBUFFSIZE; i++) {
		if (!m_aBpIsValid[i]) break;
	}
	// If the buffer is full, we must choose which old data to discard.
	// We must choose wisely!  Throwing away the wrong data could prevent
	// collecting enough data distributed across the entire 3D angular
	// range, preventing a decent cal from ever happening at all.  Making
	// any assumption about good vs bad data is particularly risky,
	// because being wrong could cause an unstable feedback loop where
	// bad data leads to wrong decisions which leads to even worse data.
	// But if done well, purging bad data has massive potential to
	// improve results.  The trick is telling the good from the bad while
	// still in the process of learning what's good...
	if (i >= MAGBUFFSIZE) {
		i = choose_discard_magcal();
		if (i < 0 || i >= MAGBUFFSIZE) {
			i = random() % MAGBUFFSIZE;
		}
	}
	// add it to the cal buffer
	m_aBpFast[0][i] = data[6];
	m_aBpFast[1][i] = data[7];
	m_aBpFast[2][i] = data[8];
	m_aBpIsValid[i] = 1;
}

void MagCalibration_t::add_raw_data(const int16_t(&data)[9], Quaternion_t * pResult)
{
	static int force_orientation_counter=0;
	float x, y, z, ratio, magdiff;
	Point_t point;

	add_magcal_data(data);
	x = m_cal_V[0];
	y = m_cal_V[1];
	z = m_cal_V[2];
	if (get_new_calibration()) {
		x -= m_cal_V[0];
		y -= m_cal_V[1];
		z -= m_cal_V[2];
		magdiff = sqrtf(x * x + y * y + z * z);
		//printf("magdiff = %.2f\n", magdiff);
		if (magdiff > 0.8f) {
			m_fusion.init();
			m_rawcount = OVERSAMPLE_RATIO;
			force_orientation_counter = 240;
		}
	}

	if (force_orientation_counter > 0) {
		if (--force_orientation_counter == 0) {
			//printf("delayed forcible orientation reset\n");
			m_fusion.init();
			m_rawcount = OVERSAMPLE_RATIO;
		}
	}

	if (m_rawcount >= OVERSAMPLE_RATIO) {
		memset(&m_accel, 0, sizeof(m_accel));
		memset(&m_mag, 0, sizeof(m_mag));
		memset(&m_gyro, 0, sizeof(m_gyro));
		m_rawcount = 0;
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
	m_gyro.YpFast[m_rawcount][0] = x;
	m_gyro.YpFast[m_rawcount][1] = y;
	m_gyro.YpFast[m_rawcount][2] = z;

	apply_calibration(data[6], data[7], data[8], &point);
	m_mag.BcFast[0] = point.x;
	m_mag.BcFast[1] = point.y;
	m_mag.BcFast[2] = point.z;
	m_mag.Bc[0] += point.x;
	m_mag.Bc[1] += point.y;
	m_mag.Bc[2] += point.z;

	m_rawcount++;
	if (m_rawcount >= OVERSAMPLE_RATIO) {
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
		m_fusion.update(&m_accel, &m_mag, &m_gyro, m_isValid, m_cal_B);
		m_fusion.read(pResult);
	}
}

