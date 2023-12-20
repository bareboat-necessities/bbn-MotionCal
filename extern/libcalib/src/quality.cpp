#include "libcalib_quality.h"

#include <string.h>
#include <math.h>

namespace libcalib
{

// Discussion of what these 4 quality metrics really do
// https://forum.pjrc.com/threads/59277-Motion-Sensor-Calibration-Tool-Parameter-Understanding

// return 0 to 99 - which region on the sphere (100 of equal surface area)
int sphere_region(float x, float y, float z)
{
	float latitude, longitude;
	int region;

	//if (pr) printf("  region %.1f,%.1f,%.1f  ", x, y, z);

	// longitude = 0 to 2pi  (meaning 0 to 360 degrees)
	longitude = atan2f(y, x) + (float)M_PI;
	// latitude = -pi/2 to +pi/2  (meaning -90 to +90 degrees)
	latitude = (float)(M_PI / 2.0) - atan2f(sqrtf(x * x + y * y), z);

	//if (pr) printf("   lat=%.1f", latitude * (float)(180.0 / M_PI));
	//if (pr) printf(",lon=%.1f  ", longitude * (float)(180.0 / M_PI));

	// https://etna.mcs.kent.edu/vol.25.2006/pp309-327.dir/pp309-327.html
	// sphere equations....
	//  area of unit sphere = 4*pi
	//  area of unit sphere cap = 2*pi*h  h = cap height
	//  lattitude of unit sphere cap = arcsin(1 - h)
	if (latitude > 1.37046f /* 78.52 deg */) {
		// arctic cap, 1 region
		region = 0;
	} else if (latitude < -1.37046f /* -78.52 deg */) {
		// antarctic cap, 1 region
		region = 99;
	} else if (latitude > 0.74776f /* 42.84 deg */ || latitude < -0.74776f ) {
		// temperate zones, 15 regions each
		region = floorf(longitude * (float)(15.0 / (M_PI * 2.0)));
		if (region < 0) region = 0;
		else if (region > 14) region = 14;
		if (latitude > 0.0) {
			region += 1; // 1 to 15
		} else {
			region += 84; // 84 to 98
		}
	} else {
		// tropic zones, 34 regions each
		region = floorf(longitude * (float)(34.0 / (M_PI * 2.0)));
		if (region < 0) region = 0;
		else if (region > 33) region = 33;
		if (latitude >= 0.0) {
			region += 16; // 16 to 49
		} else {
			region += 50; // 50 to 83
		}
	}
	//if (pr) printf("  %d\n", region);
	return region;
}

quality::quality()
: m_count(0)
, m_spheredist()
, m_spheredata()
, m_sphereideal()
, m_magnitude()
, m_gaps_buffer(0.0f)
, m_variance_buffer(0.0f)
, m_wobble_buffer(0.0f)
, m_are_gaps_computed(false)
, m_is_variance_computed(false)
, m_is_wobble_computed(false)
{
	float ring_a_angle = 1.05911;
	float ring_a_radius = cosf(1.05911) * -1.0f;
	float ring_a_height = sinf(1.05911);

	float ring_b_angle = 0.37388;
	float ring_b_radius = cosf(0.37388) * -1.0f;
	float ring_b_height = sinf(0.37388);

	m_sphereideal[0].x = 0.0f;
	m_sphereideal[0].y = 0.0f;
	m_sphereideal[0].z = 1.0f;

	for (int i=1; i <= 15; i++) {
		float longitude = ((float)(i - 1) + 0.5f) * (M_PI * 2.0 / 15.0);
		m_sphereideal[i].x = cosf(longitude) * ring_a_radius;
		m_sphereideal[i].y = sinf(longitude) * ring_a_radius;
		m_sphereideal[i].z = ring_a_height;
	}
	for (int i=16; i <= 49; i++) {
		float longitude = ((float)(i - 16) + 0.5f) * (M_PI * 2.0 / 34.0);
		m_sphereideal[i].x = cosf(longitude) * ring_b_radius;
		m_sphereideal[i].y = sinf(longitude) * ring_b_radius;
		m_sphereideal[i].z = ring_b_height;
	}
	for (int i=50; i <= 83; i++) {
		float longitude = ((float)(i - 50) + 0.5f) * (M_PI * 2.0 / 34.0);
		m_sphereideal[i].x = cosf(longitude) * ring_b_radius;
		m_sphereideal[i].y = sinf(longitude) * ring_b_radius;
		m_sphereideal[i].z = -ring_b_height;
	}
	for (int i=84; i <= 98; i++) {
		float longitude = ((float)(i - 1) + 0.5f) * (M_PI * 2.0 / 15.0);
		m_sphereideal[i].x = cosf(longitude) * ring_a_radius;
		m_sphereideal[i].y = sinf(longitude) * ring_a_radius;
		m_sphereideal[i].z = -ring_a_height;
	}
	m_sphereideal[99].x = 0.0f;
	m_sphereideal[99].y = 0.0f;
	m_sphereideal[99].z = -1.0f;
}

void quality::reset()
{
	m_count = 0;
	memset(&m_spheredist, 0, sizeof(m_spheredist));
	memset(&m_spheredata, 0, sizeof(m_spheredata));
	
	m_are_gaps_computed = false;
	m_is_variance_computed = false;
	m_is_wobble_computed = false;
}

void quality::update(const Point_t *point)
{
	float x, y, z;
	int region;

	x = point->x;
	y = point->y;
	z = point->z;
	// NOTE bruceo: uh, what if count is >= MAGBUFFSIZE?
	//	seems like we could be walking off the end of the array here.
	// ah, ok. it appears that display_callback() calls quality::reset()
	//	before feeding all the MagCalibrator::m_aBpFast calibrated points to this
	//	routine. so we are guaranteed to never get more than MAGBUFFSIZE
	//	points.
	m_magnitude[m_count] = sqrtf(x * x + y * y + z * z);
	region = sphere_region(x, y, z);
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
float quality::surface_gap_error()
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
float quality::magnitude_variance_error()
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
float quality::wobble_error()
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
			xi = m_sphereideal[i].x * radius;
			yi = m_sphereideal[i].y * radius;
			zi = m_sphereideal[i].z * radius;
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
