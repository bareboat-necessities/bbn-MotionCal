#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/mstream.h>

#include "image2wx-node.h"

// NOTE bruceo: it's insane that C, in the year of our lord 2023, still does not have
//	a standard, cross platform, case insensitive string compare function.

#ifdef _MSC_VER 
//not #if defined(_WIN32) || defined(_WIN64) because we have strncasecmp in mingw
inline int strncasecmp(char const* _String1, char const* _String2, size_t _MaxCount)
{
	return _strnicmp(_String1, _String2, _MaxCount);
}

inline int strcasecmp(char const* _String1, char const* _String2)
{
	return _stricmp(_String1, _String2);
}
#endif

namespace image2wx
{
	node_t * node_t::s_head = nullptr;

	wxBitmapType BitmapTypeFromName(const char * name)
	{
		const char * extension = strrchr(name, '.');

		if (!extension)
			return wxBITMAP_TYPE_ANY;

		struct SExtType
		{
			const char * extension;
			wxBitmapType type;
		};

		static const SExtType s_aEt[] =
		{
			// NOTE bruceo: arbitrarily reordered to put more common types first

			{ ".png",	wxBITMAP_TYPE_PNG },
			{ ".jpg",	wxBITMAP_TYPE_JPEG },
			{ ".jpeg",	wxBITMAP_TYPE_JPEG },
			{ ".tif",	wxBITMAP_TYPE_TIFF },
			{ ".tiff",	wxBITMAP_TYPE_TIFF },
			{ ".gif",	wxBITMAP_TYPE_GIF },
			{ ".bmp",	wxBITMAP_TYPE_BMP },
			{ ".ico",	wxBITMAP_TYPE_ICO },
			{ ".cur",	wxBITMAP_TYPE_CUR },
			{ ".xbm",	wxBITMAP_TYPE_XBM },
			{ ".xpm",	wxBITMAP_TYPE_XPM },
			{ ".pnm",	wxBITMAP_TYPE_PNM },
			{ ".pcx",	wxBITMAP_TYPE_PCX },
			{ ".pict",	wxBITMAP_TYPE_PICT },
			{ ".icon",	wxBITMAP_TYPE_ICON },
			{ ".ani",	wxBITMAP_TYPE_ANI },
			{ ".iff",	wxBITMAP_TYPE_IFF },
			{ ".tga",	wxBITMAP_TYPE_TGA },
			{ ".targa",	wxBITMAP_TYPE_TGA },
		};

		for (const auto et : s_aEt)
		{
			if (strcasecmp(extension, et.extension) == 0)
				return et.type;
		}

		return wxBITMAP_TYPE_ANY;
	}

	wxBitmap BitmapFromNode(node_t * node)
	{
		if (node == nullptr)
			return wxNullBitmap;
		
		if (wxBitmap * p = static_cast<wxBitmap *>(node->m_bitmap))
			return *p;

		wxMemoryInputStream istream(node->m_bytes, node->m_size);
		wxBitmapType type = BitmapTypeFromName(node->m_name);
		wxImage img(istream, type);
		
		node->m_bitmap = new wxBitmap(img);

		return * static_cast<wxBitmap *>(node->m_bitmap);
	}

	wxBitmap BitmapFromName(const char *name)
	{
		for (auto node = node_t::s_head; node; node = node->m_next)
		{
			if (strcasecmp(name, node->m_name) == 0)
				return BitmapFromNode(node);
		}

		return wxNullBitmap;
	}

	void FreeBitmaps()
	{
		for (auto node = node_t::s_head; node; node = node->m_next)
		{
			delete static_cast<wxBitmap *>(node->m_bitmap);
			node->m_bitmap = nullptr;
		}
	}
} // namespace image2wx
