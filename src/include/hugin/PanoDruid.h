// -*- c-basic-offset: 4 -*-

/** @file PanoDruid.h
 *
 *  @author Ed Halley <ed@halley.cc>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _PANODRUID_H
#define _PANODRUID_H

#include "PT/Panorama.h"
#include "PT/PanoramaMemento.h"

struct DruidHint;

/////////////////////////////////////////////////////////////////////////////

// the druid is the gui
class PanoDruid : public wxPanel
{
public:
    PanoDruid(wxWindow* parent);
    void Update(const PT::Panorama& pano);
    DruidHint* FindHint(const wxChar* name);

protected:
    int m_advice;
    wxStaticBoxSizer  * m_boxSizer;
    wxBitmap m_bitmap;
    wxStaticBitmap * m_graphic;
    wxStaticText * m_text;

    static int sm_hints;
    static int sm_chunk;
    static int sm_sorted;
    static DruidHint** sm_advice;

public:
    static void DefineHint(DruidHint* advice);
};

/////////////////////////////////////////////////////////////////////////////

struct DruidHint
{
    DruidHint(int rank,
              const wxChar* name,
              const wxChar* graphic,
              const wxChar* brief,
              const wxChar* text)
	{
            this->rank = rank;
            this->name = name;
            this->graphic = graphic;
            this->brief = brief;
            this->text = text;
            DEBUG_TRACE( "Adding DruidHint \"" << this->name.mb_str() << "\"..." );
            PanoDruid::DefineHint(this);
	}
    virtual int applies(const PT::Panorama& pano, const PT::PanoramaOptions& opts)
        { return FALSE; }
    int rank;
    // const wxChar* name;
    // const wxChar* graphic;
    // const wxChar* brief;
    // const wxChar* text;
    wxString name;
    wxString graphic;
    wxString brief;
    wxString text;
};

#define NEW_HINT(rank,name,graphic,brief,text) \
    struct hint##name : public DruidHint { \
        hint##name() : DruidHint(rank, wxT(#name), graphic, brief, text) { ; } \
    int applies(const PT::Panorama& pano, const PT::PanoramaOptions& opts)

#define END_HINT(name) \
    } _the##name;

/////////////////////////////////////////////////////////////////////////////

#endif // _PANODRUID_H
