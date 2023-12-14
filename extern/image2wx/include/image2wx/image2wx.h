#pragma once

#include <wx/bitmap.h>

namespace image2wx
{
	wxBitmap BitmapFromName(const char *name);
	void FreeBitmaps();
} // namespace image2wx
