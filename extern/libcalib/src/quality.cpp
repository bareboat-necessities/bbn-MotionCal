#include "libcalib.h"

#include <math.h>
#include <string.h>

#include <algorithm>

namespace libcalib
{

constexpr float RadFromDeg(float deg)
{
	return (M_PI * deg) / 180.0f;
}

template<uint32_t N, class T>
constexpr uint32_t DIM(T(&)[N]) { return N; }

// a class which breaks a sphere into 100 partitions of roughly equal
//	size and radius. details here:
//		https://etna.mcs.kent.edu/vol.25.2006/pp309-327.dir/pp309-327.html
// sphere equations....
//  area of unit sphere = 4*pi
//  area of unit sphere cap = 2*pi*h  h = cap height
//  latitude of unit sphere cap = arcsin(1 - h)

struct SpherePartition
{
	SpherePartition();

	static const int s_regionMax = 100;

	Point_t	m_mpRegionAnchor[s_regionMax];
	
	int RegionFromXyz(float x, float y, float z);

private:
	struct Collar
	{
		int		m_cRegion;	// number of regions
		float	m_latMax;	// upper/lower latitude in radians
		float	m_latMin;
	};


	static constexpr int s_cRegionPole =		1;
	static constexpr int s_cRegionTemperate =	15;
	static constexpr int s_cRegionTropic =		34;

	static constexpr float s_degNorthPole =		90.0f;
	static constexpr float s_degTemperateMax =	78.52;
	static constexpr float s_degTropicMax =		42.84;
	static constexpr float s_degEquator =		0.0f;
	static constexpr float s_degTropicMin =		-s_degTropicMax;
	static constexpr float s_degTemperateMin =	-s_degTemperateMax;
	static constexpr float s_degSouthPole =		-s_degNorthPole;

	static constexpr float s_radNorthPole =		RadFromDeg(s_degNorthPole);
	static constexpr float s_radTemperateMax =	RadFromDeg(s_degTemperateMax);
	static constexpr float s_radTropicMax =		RadFromDeg(s_degTropicMax);
	static constexpr float s_radEquator =		RadFromDeg(s_degEquator);
	static constexpr float s_radTropicMin =		RadFromDeg(s_degTropicMin);
	static constexpr float s_radTemperateMin =	RadFromDeg(s_degTemperateMin);
	static constexpr float s_radSouthPole =		RadFromDeg(s_degSouthPole);

	static constexpr Collar s_aCollar[] =
	{
		// m_cRegion			m_latMax			m_latMin
		{ s_cRegionPole,		s_radNorthPole,		s_radTemperateMax },
		{ s_cRegionTemperate,	s_radTemperateMax,	s_radTropicMax },
		{ s_cRegionTropic,		s_radTropicMax,		s_radEquator },
		{ s_cRegionTropic,		s_radEquator,		s_radTropicMin },
		{ s_cRegionTemperate,	s_radTropicMin,		s_radTemperateMin },
		{ s_cRegionPole,		s_radTemperateMin,	s_radSouthPole },
	};
	static constexpr int s_cCollar = DIM(s_aCollar);

	static_assert(s_regionMax == (
					s_aCollar[0].m_cRegion +
					s_aCollar[1].m_cRegion +
					s_aCollar[2].m_cRegion +
					s_aCollar[3].m_cRegion +
					s_aCollar[4].m_cRegion +
					s_aCollar[5].m_cRegion));
	static_assert(s_cCollar == 6);
};


SpherePartition::SpherePartition()
{
	static_assert(s_aCollar[0].m_cRegion == 1);

	m_mpRegionAnchor[0].x = 0.0f;
	m_mpRegionAnchor[0].y = 0.0f;
	m_mpRegionAnchor[0].z = 1.0f;

	Point_t * pAnchor = &m_mpRegionAnchor[1];

	// NOTE: skipping first and last collars (the poles);

	for (int iCollar = 1; iCollar < s_cCollar - 1; ++iCollar)
	{
		auto & collar = s_aCollar[iCollar];
		float latMiddle = (collar.m_latMin + collar.m_latMax) / 2.0f;
		float radiusMiddle = cosf(latMiddle);
		float zMiddle = sinf(latMiddle);
		float dLongitude = (M_PI * 2.0 / float(collar.m_cRegion));

		for (int subregion = 0; subregion < collar.m_cRegion; ++subregion)
		{
			float longitude = (float(subregion) + 0.5f) * dLongitude;

			// negation here is to flip regions into a winding order that matches
			// the subregion from longitude calculation in RegionFromXyz().

			pAnchor->x = -cosf(longitude) * radiusMiddle;
			pAnchor->y = -sinf(longitude) * radiusMiddle;
			pAnchor->z = zMiddle;

			++pAnchor;
		}
	}

	static_assert(s_aCollar[s_cCollar - 1].m_cRegion == 1);

	m_mpRegionAnchor[s_regionMax - 1].x = 0.0f;
	m_mpRegionAnchor[s_regionMax - 1].y = 0.0f;
	m_mpRegionAnchor[s_regionMax - 1].z = -1.0f;

	//for (int region = 0; region < s_regionMax; ++region)
	//{
	//	const auto & anchor = m_mpRegionAnchor[region];
	//	assert(region == RegionFromXyz(anchor.x, anchor.y, anchor.z));
	//}
}

int SpherePartition::RegionFromXyz(float x, float y, float z)
{
	float latitude = (M_PI / 2.0) - atan2f(sqrtf(x * x + y * y), z);

	int regionCur = 0;

	for (auto collar : s_aCollar)
	{
		if (latitude >= collar.m_latMin)
		{
			if (collar.m_cRegion <= 1)
				return regionCur;

			float longitude = atan2f(y, x) + M_PI;
			int subregion = floorf(float(collar.m_cRegion) * longitude / (M_PI * 2.0));
			subregion = std::clamp(subregion, 0, collar.m_cRegion - 1);

			return regionCur + subregion;
		}

		regionCur += collar.m_cRegion;
	}

	return 0;
}

static SpherePartition s_sphere_partition;

MagQuality::MagQuality()
: m_errGaps(s_errMax)
, m_errVariance(s_errMax)
, m_errWobble(s_errMax)
, m_errFit(s_errMax)
, m_isValid(false)
, m_mpRegionCount()
, m_mpRegionSum()
{
}

void MagQuality::ensure_valid(const MagCalibrator & magcal)
{
	if (m_isValid)
		return;

	memset(&m_mpRegionCount, 0, sizeof(m_mpRegionCount));
	memset(&m_mpRegionSum, 0, sizeof(m_mpRegionSum));

	for (int i = 0; i < magcal.m_cSamp; i++)
	{
		const MagSample & samp = magcal.m_aSamp[i];
		const Point_t & pntCal = samp.m_pntCal;
		float x = pntCal.x;
		float y = pntCal.y;
		float z = pntCal.z;

		int region = s_sphere_partition.RegionFromXyz(x, y, z);
		m_mpRegionCount[region]++;
		m_mpRegionSum[region].x += x;
		m_mpRegionSum[region].y += y;
		m_mpRegionSum[region].z += z;
	}

	m_errGaps = ErrGaps();
	m_errVariance = ErrVariance(magcal);
	m_errWobble = ErrWobble(magcal);
	m_errFit = magcal.m_errFit;

	m_isValid = true;
}

bool MagQuality::AreErrorsOk() const
{
	if (m_errGaps >= s_errGapsOkMin)
		return false;
	if (m_errVariance >= s_errVarianceOkMin)
		return false;
	if (m_errWobble >= s_errWobbleOkMin)
		return false;
	if (m_errFit >= s_errFitOkMin)
		return false;

	return true;
}

bool MagQuality::AreErrorsBad() const
{
	if (m_errGaps <= s_errGapsBadMax)
		return false;
	if (m_errVariance <= s_errVarianceBadMax)
		return false;
	if (m_errWobble <= s_errWobbleBadMax)
		return false;
	if (m_errFit <= s_errFitBadMax)
		return false;

	return true;
}

// How many surface gaps

float MagQuality::ErrGaps()
{
	float error=0.0f;

	for (int i=0; i < DIM(m_mpRegionCount); i++) {
		int count = m_mpRegionCount[i];

		if (count == 0) {
			error += 1.0f;
		} else if (count == 1) {
			error += 0.2f;
		} else if (count == 2) {
			error += 0.01f;
		}
	}

	return error;
}

// Variance in magnitude

float MagQuality::ErrVariance(const MagCalibrator & magcal)
{
	if (magcal.m_cSamp == 0)
		return s_errMax;

	float sum = 0.0f;
	for (int i=0; i < magcal.m_cSamp; i++) {
		const MagSample& samp = magcal.m_aSamp[i];
		sum += samp.m_field;
	}

	float mean = sum / (float)magcal.m_cSamp;
	
	float variance = 0.0f;
	for (int i=0; i < magcal.m_cSamp; i++) {
		const MagSample& samp = magcal.m_aSamp[i];
		float diff = samp.m_field - mean;
		variance += diff * diff;
	}
	
	variance /= (float)magcal.m_cSamp;

	return sqrtf(variance) / mean * s_errMax;
}

// Offset of piecewise average data from ideal sphere surface

float MagQuality::ErrWobble(const MagCalibrator & magcal)
{
	if (magcal.m_cSamp == 0)
		return s_errMax;

	float sum = 0.0f;
	for (int i=0; i < magcal.m_cSamp; i++) {
		const MagSample& samp = magcal.m_aSamp[i];
		sum += samp.m_field;
	}

	float radius = sum / (float)magcal.m_cSamp;

	float xoff = 0.0f;
	float yoff = 0.0f;
	float zoff = 0.0f;
	int cRegionHit = 0;

	for (int i=0; i < DIM(m_mpRegionCount); i++)
	{
		if (m_mpRegionCount[i] > 0)
		{
			float x = m_mpRegionSum[i].x / (float)m_mpRegionCount[i];
			float y = m_mpRegionSum[i].y / (float)m_mpRegionCount[i];
			float z = m_mpRegionSum[i].z / (float)m_mpRegionCount[i];

			float xi = s_sphere_partition.m_mpRegionAnchor[i].x * radius;
			float yi = s_sphere_partition.m_mpRegionAnchor[i].y * radius;
			float zi = s_sphere_partition.m_mpRegionAnchor[i].z * radius;

			xoff += x - xi;
			yoff += y - yi;
			zoff += z - zi;

			cRegionHit++;
		}
	}

	if (cRegionHit == 0)
		return s_errMax;
	
	xoff /= (float)cRegionHit;
	yoff /= (float)cRegionHit;
	zoff /= (float)cRegionHit;

	return sqrtf(xoff * xoff + yoff * yoff + zoff * zoff) / radius * s_errMax;;
}

} // namespace libcalib
