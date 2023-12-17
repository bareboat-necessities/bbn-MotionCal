#pragma once

#include "libcalib_common.h"

namespace libcalib
{

struct quality
{
			quality();

	void	reset();
	void	update(const Point_t* point);

	float	surface_gap_error();
	float	magnitude_variance_error();
	float	wobble_error();

private:
	int		m_count;
	int		m_spheredist[100];
	Point_t	m_spheredata[100];
	Point_t	m_sphereideal[100];
	bool	m_is_sphereideal_initialized;
	float	m_magnitude[MAGBUFFSIZE];
	float	m_gaps_buffer;
	float	m_variance_buffer;
	float	m_wobble_buffer;
	bool	m_are_gaps_computed;
	bool	m_is_variance_computed;
	bool	m_is_wobble_computed;
};

} // namespace libcalib
