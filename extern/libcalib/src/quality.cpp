#include "libcalib_quality.h"

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
//  lattitude of unit sphere cap = arcsin(1 - h)

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

// Discussion of what these 4 quality metrics really do
// https://forum.pjrc.com/threads/59277-Motion-Sensor-Calibration-Tool-Parameter-Understanding

Quality::Quality()
: m_count(0)
, m_spheredist()
, m_spheredata()
, m_magnitude()
, m_gaps_buffer(0.0f)
, m_variance_buffer(0.0f)
, m_wobble_buffer(0.0f)
, m_are_gaps_computed(false)
, m_is_variance_computed(false)
, m_is_wobble_computed(false)
{
}

void Quality::reset()
{
	m_count = 0;
	memset(&m_spheredist, 0, sizeof(m_spheredist));
	memset(&m_spheredata, 0, sizeof(m_spheredata));
	
	m_are_gaps_computed = false;
	m_is_variance_computed = false;
	m_is_wobble_computed = false;
}

void Quality::update(const Point_t *point)
{
	float x = point->x;
	float y = point->y;
	float z = point->z;
	// NOTE bruceo: uh, what if count is >= MAGBUFFSIZE?
	//	seems like we could be walking off the end of the array here.
	// ah, ok. it appears that display_callback() calls Quality::reset()
	//	before feeding all the MagCalibrator::m_aBpFast calibrated points to this
	//	routine. so we are guaranteed to never get more than MAGBUFFSIZE
	//	points.
	m_magnitude[m_count] = sqrtf(x * x + y * y + z * z);
	int region = s_sphere_partition.RegionFromXyz(x, y, z);
	m_spheredist[region]++;
	m_spheredata[region].x += x;
	m_spheredata[region].y += y;
	m_spheredata[region].z += z;
	m_count++;
	m_are_gaps_computed = 0;
	m_is_variance_computed = 0;
	m_is_wobble_computed = 0;
}

// How many surface gaps
float Quality::surface_gap_error()
{
	float error=0.0f;
	int i, num;

	if (m_are_gaps_computed) return m_gaps_buffer;
	for (i=0; i < 100; i++) {
		num = m_spheredist[i];
		if (num == 0) {
			error += 1.0f;
		} else if (num == 1) {
			error += 0.2f;
		} else if (num == 2) {
			error += 0.01f;
		}
	}
	m_gaps_buffer = error;
	m_are_gaps_computed = 1;
	return m_gaps_buffer;
}

// Variance in magnitude
float Quality::magnitude_variance_error()
{
	float sum, mean, diff, variance;
	int i;

	if (m_is_variance_computed) return m_variance_buffer;
	sum = 0.0f;
	for (i=0; i < m_count; i++) {
		sum += m_magnitude[i];
	}
	mean = sum / (float)m_count;
	variance = 0.0f;
	for (i=0; i < m_count; i++) {
		diff = m_magnitude[i] - mean;
		variance += diff * diff;
	}
	variance /= (float)m_count;
	m_variance_buffer = sqrtf(variance) / mean * 100.0f;
	m_is_variance_computed = 1;
	return m_variance_buffer;
}

// Offset of piecewise average data from ideal sphere surface
float Quality::wobble_error()
{
	float sum, radius, x, y, z, xi, yi, zi;
	float xoff=0.0f, yoff=0.0f, zoff=0.0f;
	int i, n=0;

	if (m_is_wobble_computed) return m_wobble_buffer;
	sum = 0.0f;
	for (i=0; i < m_count; i++) {
		sum += m_magnitude[i];
	}
	radius = sum / (float)m_count;
	//if (pr) printf("  radius = %.2f\n", radius);
	for (i=0; i < 100; i++) {
		if (m_spheredist[i] > 0) {
			//if (pr) printf("  i=%3d", i);
			x = m_spheredata[i].x / (float)m_spheredist[i];
			y = m_spheredata[i].y / (float)m_spheredist[i];
			z = m_spheredata[i].z / (float)m_spheredist[i];
			//if (pr) printf("  at: %5.1f %5.1f %5.1f :", x, y, z);
			xi = s_sphere_partition.m_mpRegionAnchor[i].x * radius;
			yi = s_sphere_partition.m_mpRegionAnchor[i].y * radius;
			zi = s_sphere_partition.m_mpRegionAnchor[i].z * radius;
			//if (pr) printf("   ideal: %5.1f %5.1f %5.1f :", xi, yi, zi);
			xoff += x - xi;
			yoff += y - yi;
			zoff += z - zi;
			//if (pr) printf("\n");
			n++;
		}
	}
	if (n == 0) return 100.0f;
	//if (pr) printf("  off = %.2f, %.2f, %.2f\n", xoff, yoff, zoff);
	xoff /= (float)n;
	yoff /= (float)n;
	zoff /= (float)n;
	//if (pr) printf("  off = %.2f, %.2f, %.2f\n", xoff, yoff, zoff);
	m_wobble_buffer = sqrtf(xoff * xoff + yoff * yoff + zoff * zoff) / radius * 100.0f;
	m_is_wobble_computed = 1;
	return m_wobble_buffer;
}

} // namespace libcalib
