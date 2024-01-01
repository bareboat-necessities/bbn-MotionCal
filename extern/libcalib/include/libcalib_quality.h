#pragma once

#include "libcalib_common.h"

namespace libcalib
{

class MagCalibrator;

struct MagQuality
{
	MagQuality();
	
	void set_invalid()
			{ m_isValid = false; }
	void ensure_valid(const MagCalibrator & magcal);

	bool AreErrorsOk() const;
	bool AreErrorsBad() const;

	// Discussion of what these 4 quality metrics really do
	// https://forum.pjrc.com/threads/59277-Motion-Sensor-Calibration-Tool-Parameter-Understanding
	// All are 0..100, 0 being perfection and 100 being "all error".

	float	m_errGaps;		// how much of the sphere's surface is missing data points.
	float	m_errVariance;	// how much of the data is not located on the (imagined) surface of the sphere.
	float	m_errWobble;	// how far an estimated the "center of mass" is from the ideal center.
	float	m_errFit;		// how closely the data matches a perfect sphere.

	static constexpr float s_errMax = 100.0f;

private:
	float	ErrGaps();
	float	ErrVariance(const MagCalibrator & magcal);
	float	ErrWobble(const MagCalibrator & magcal);

	bool	m_isValid;
	int		m_mpRegionCount[100];
	Point_t	m_mpRegionSum[100];

	// these taken from MyFrame::OnTimer() in the original MotionCal app.
	//	(https://github.com/PaulStoffregen/MotionCal/blob/master/gui.cpp)

	// errors must all be below these values for calibration to be considered ok

	static constexpr float s_errGapsOkMin = 15.0f;
	static constexpr float s_errVarianceOkMin = 4.5f;
	static constexpr float s_errWobbleOkMin = 4.0f;
	static constexpr float s_errFitOkMin = 5.0f;

	// if errors all go above these values, the calibration is not ok anymore.

	static constexpr float s_errGapsBadMax = 20.0f;
	static constexpr float s_errVarianceBadMax = 5.0f;
	static constexpr float s_errWobbleBadMax = 5.0f;
	static constexpr float s_errFitBadMax = 6.0f;

};

} // namespace libcalib
