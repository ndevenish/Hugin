// -*- c-basic-offset: 4 -*-

/** @file UniversalCursor.cpp
 *
 *  @brief implementation of a platform independant cursor class
 *
 *  Another example, where wxWindows failed to abstract
 *  between the platforms properly.
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: UniversalCursor.cpp 843 2004-10-27 18:12:56Z dangelo $
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

// standard wx include
#include <config.h>
#include "panoinc_WX.h"

#include "common/utils.h"

#include "hugin/UniversalCursor.h"

#if defined(__WXMSW__)
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/list.h"
    #include "wx/utils.h"
    #include "wx/app.h"
    #include "wx/bitmap.h"
    #include "wx/icon.h"
    #include "wx/cursor.h"
#endif

#include "wx/module.h"
#include "wx/image.h"
#include "wx/msw/private.h"
#ifndef __WXMICROWIN__
#include "wx/msw/dib.h"
#endif

#if wxUSE_RESOURCE_LOADING_IN_MSW
    #include "wx/msw/curico.h"
    #include "wx/msw/curicop.h"
#endif

#elif defined(__WXGTK__)
#include "wx/utils.h"
#include "wx/app.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#endif

using namespace std;

#if defined(__WXMSW__)

#if 0
/** @BUG this is a bad hack! take from the private msw/cursor.c file
 *
 */
class WXDLLEXPORT wxCursorRefData : public wxGDIImageRefData
{
public:
    // the second parameter is used to tell us to delete the cursor when we're
    // done with it (normally we shouldn't call DestroyCursor() this is why it
    // doesn't happen by default)
    wxCursorRefData(HCURSOR hcursor = 0, bool takeOwnership = false);

    virtual ~wxCursorRefData() { Free(); }

    virtual void Free();


    // return the size of the standard cursor: notice that the system only
    // supports the cursors of this size
    static wxCoord GetStandardWidth();
    static wxCoord GetStandardHeight();

private:
    bool m_destroyCursor;

    // standard cursor size, computed on first use
    static wxSize ms_sizeStd;
};

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

//IMPLEMENT_DYNAMIC_CLASS(UniversalCursor, wxCursor)

UniversalCursor::UniversalCursor(const wxImage & image)
{
    //image has to be 32x32
    wxImage image32 = image.Scale(32,32);
    unsigned char * rgbBits = image32.GetData();
    int w = image32.GetWidth()  ;
    int h = image32.GetHeight() ;
//    bool bHasMask = image32.HasMask() ;
    int imagebitcount = (w*h)/8;

    int hotSpotX = 0;
    int hotSpotY = 0;

//    unsigned char r, g, b ;
    unsigned char * bits = new unsigned char [imagebitcount];
    unsigned char * maskBits = new unsigned char [imagebitcount];

    int i,j, i8;
//    unsigned char c;
    unsigned char cMask;
    for (i=0; i<imagebitcount; i++)
    {
        bits[i] = 0;
        maskBits[i] = 0;
        i8 = i * 8;
//unlike gtk, the pixels go in the opposite order in the bytes
        cMask = 128;
        for (j=0; j<8; j++)
        {
            // pixels
            if (rgbBits[0] > 127) {
                bits[i] = bits[i] | cMask ;
            }
            // mask
            if (rgbBits[1] > 0) {
                maskBits[i] = maskBits[i] | cMask ;
            }
            // hotspot
            if (rgbBits[2] > 0) {
                hotSpotX = (i*8 + j) % w;
                hotSpotY = (i*8 + j) / w;
            }
            rgbBits += 3;
        }
    }

    wxCursorRefData *refData = new wxCursorRefData;
    m_refData = refData;
    refData->m_hCursor = (WXHCURSOR) CreateCursor ( wxGetInstance(), hotSpotX, hotSpotY, w, h, /*AND*/ maskBits, /*XOR*/ bits   );

    delete [] bits ;
    delete [] maskBits;
}
#endif

#elif defined(__WXGTK__)


class wxCursorRefData: public wxObjectRefData
{
  public:

    wxCursorRefData();
    ~wxCursorRefData();

    GdkCursor *m_cursor;
};


#define M_CURSORDATA ((wxCursorRefData *)m_refData)
extern GtkWidget *wxGetRootWindow();

UniversalCursor::UniversalCursor(const wxImage & image)
{
    unsigned char * rgbBits = image.GetData();
    int w = image.GetWidth() ;
    int h = image.GetHeight();
    int imagebitcount = (w*h)/8;
    int hotSpotX = 0;
    int hotSpotY = 0;

    unsigned char * bits = new unsigned char [imagebitcount];
    unsigned char * maskBits = new unsigned char [imagebitcount];

    int i, j, i8; unsigned char cMask;
    for (i=0; i<imagebitcount; i++)
    {
        bits[i] = 0;
        maskBits[i] = 0;
        i8 = i * 8;

        cMask = 1;
        for (j=0; j<8; j++)
        {
            // pixels
            if (rgbBits[0] > 127) {
                bits[i] = bits[i] | cMask ;
            }
            // mask
            if (rgbBits[1] > 0) {
                maskBits[i] = maskBits[i] | cMask ;
            }
            // hotspot
            if (rgbBits[2] > 0) {
                hotSpotX = (i*8 + j) % w;
                hotSpotY = (i*8 + j) / w;
            }
            rgbBits += 3;
            cMask = cMask * 2;
        }
    }


    GdkBitmap *data = gdk_bitmap_create_from_data(wxGetRootWindow()->window,
                                                  (gchar *) bits, w, h);
    GdkBitmap *mask = gdk_bitmap_create_from_data(wxGetRootWindow()->window,
                                                  (gchar *) maskBits, w, h);

    m_refData = new wxCursorRefData;
    M_CURSORDATA->m_cursor = gdk_cursor_new_from_pixmap
                             (
                                data,
                                mask,
                                wxColour(255,255,255).GetColor() ,
                                wxColour(0,0,0).GetColor(),
                                hotSpotX, hotSpotY
                             );

    gdk_bitmap_unref( data );
    gdk_bitmap_unref( mask );
    delete [] bits;
    delete [] maskBits;
}

#else

UniversalCursor::UniversalCursor(const wxImage & img)
    : wxCursor(img)
{

}
#endif

