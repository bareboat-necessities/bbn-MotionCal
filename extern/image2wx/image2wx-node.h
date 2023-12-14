#pragma once

namespace image2wx
{
	struct node_t
	{
		node_t(
			const char * name,
			const void * bytes,
			unsigned long size)
		: m_name(name)
		, m_bytes(bytes)
		, m_size(size)
		, m_bitmap(nullptr)
		, m_next(nullptr)
		{
			m_next = s_head;
			s_head = this;
		}

		const char * m_name;
		const void * m_bytes;
		unsigned long m_size;
		void * m_bitmap; // void so we don't have to include wx goo into this simple class.
		node_t * m_next;

		static node_t * s_head;
	};
} // namespace image2wx
