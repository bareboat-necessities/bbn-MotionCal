#include <wx/bitmap.h>

wxBitmap MyBitmap(const char *name)
{
	return wxBitmap(name, wxBITMAP_TYPE_PNG_RESOURCE);
}
