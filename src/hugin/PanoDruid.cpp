// -*- c-basic-offset: 4 -*-

/** @file PanoDruid.cpp
 *
 *  @brief the Panorama Druid and its DruidHints database
 *
 *  @author Ed Halley <ed@halley.cc>
 *
 *  $Id$
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

#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/xrc/xmlres.h>          // XRC XML resouces
#include <wx/tglbtn.h>
#include <wx/config.h>

#include "hugin/PanoDruid.h"
#include "hugin/MainFrame.h"

/////////////////////////////////////////////////////////////////////////////

// Add the hint definitions in reverse order, with increasing rank.
// A good panorama passes all of the tests from the bottom (highest rank)
// to the top (lowest rank).

//////////

NEW_HINT(0, ERROR, "druid.images.128.png",
		 _("The druid has no advice."),
		 _(""))
{
	return TRUE;
}
END_HINT(ERROR);

//////////

NEW_HINT(1, READY, "druid.stitch.128.png",
		 _("The druid finds no problems with your panorama."),
		 _("Stitch your final image now, and then use an image editor\n"
		   "such as the GNU Image Manipulation Program (the GIMP)\n"
		   "to add any finishing touches."))
{
	return TRUE;
}
END_HINT(READY);

//////////

NEW_HINT(5, UNSAVED, "druid.stitch.128.png",
		 _("Warning:  you haven't saved the current project."),
		 _("While everything else seems to be ready to stitch,\n"
		   "don't forget to save your project file so you can\n"
		   "experiment or adjust the settings later."))
{
	return FALSE; //TODO: pano.isDirty(); // isn't currently signaled right
}
END_HINT(UNSAVED);

//////////

NEW_HINT(20, HUGE_FINAL, "druid.stitch.128.png",
		 _("Warning:  current stitch has huge dimensions."),
		 _("Very large pixel dimensions are currently entered.\n"
		   "Some computers may take an excessively long time\n"
		   "to render such a large final image.\n"
		   "For best results, use the automatic Calc button on\n"
		   "the Panorama Options tab to determine the\n"
		   "pixel dimensions which will give the best quality."))
{
	//REVIEW: calc input images and/or memory
	unsigned long threshold = 100000000L;
	return (((unsigned long)opts.width * opts.getHeight() >= threshold));
}
END_HINT(HUGE_FINAL);

//////////

NEW_HINT(25, LOW_HFOV, "druid.lenses.128.png",
		 _("The Horizontal Field of View (HFOV) may be too low."),
		 _("Check that the focal lengths and/or hfov figures\n"
		   "for each image are correct for the camera settings.\n"
		   "Then calculate the visible field of view again.\n"
		   "HFOV is measured in degrees of arc, usually between\n"
		   "5 and 120 degrees per image unless using specialized\n"
		   "lenses."))
{
	return (opts.HFOV <= 2.0);
}
END_HINT(LOW_HFOV);

//////////

NEW_HINT(45, NO_PLUMB_GUIDES, "druid.control.128.png",
		 _("Consider adding a vertical or horizontal guide."),
		 _("By adding vertical guides, the optimizer can ensure\n"
		   "that buildings or trees or other vertical features\n"
		   "appear vertical in the final result.  A horizontal\n"
		   "guide can help ensure a horizon that does not bend."))
{
	return FALSE; // TODO: scan for plumb guide pairs
}
END_HINT(NO_PLUMB_GUIDES);

//////////

NEW_HINT(46, FEW_GUIDES, "druid.control.128.png",
		 _("Add more control points to improve the stitch quality."),
		 _("For best results, there should be at least four pairs\n"
		   "of control points for each pair of overlapping images.\n"
		   "More points, accurately placed, will improve the match."))
{
	return FALSE; // TODO: scan for guide pairs vs image pair ratio
}
END_HINT(FEW_GUIDES);

//////////

NEW_HINT(47, UNGUIDED_IMAGE, "druid.control.128.png",
		 _("At least one image has no control points at all."),
		 _("For best results, there should be at least four pairs\n"
		   "of control points for each pair of overlapping images.\n"
		   "An image with no control points cannot be aligned."))
{
	return FALSE; // TODO: scan for image with no guide pairs
}
END_HINT(UNGUIDED_IMAGE);

//////////

NEW_HINT(48, NO_GUIDES, "druid.control.128.png",
		 _("Add stitching control points to each pair of images."),
		 _("The Optimizer relies on your control points to arrange\n"
		   "and blend the images properly.  On the Control Points\n"
		   "tab, add pairs of points that correspond to identical\n"
		   "visual features in each pair of overlapping images."))
{
	return (0 == pano.getNrOfCtrlPoints());
}
END_HINT(NO_GUIDES);

//////////

NEW_HINT(85, NO_LENSES, "druid.lenses.128.png",
		 _("There are no lenses defined or inferred."),
		 _("Add and configure the appropriate lens information\n"
		   "on the Camera and Lens tab."))
{
	return FALSE; // TODO: (0 == pano.getLensCount());
}
END_HINT(NO_LENSES);

//////////

NEW_HINT(95, ONE_IMAGE, "druid.images.128.png",
		 _("Add at least one more image."),
		 _("You should have at least two files listed in the Images tab."))
{
	//REVIEW: may ignore this hint if there are already control points
	return (1 == pano.getNrOfImages());
}
END_HINT(ONE_IMAGE);

//////////

NEW_HINT(100, NO_IMAGES, "druid.images.128.png",
		 _("To get started, add some image files."),
		 _("You can add any number of images using the Images tab."))
{
	return (0 == pano.getNrOfImages());
}
END_HINT(NO_IMAGES);

//////////

// Other hints to be added:

//TODO: all images have the same center
//TODO: no variables are marked to be optimized
//TODO: all variables are marked to be optimized
//TODO: optimizing for different projection from final
//TODO: above 90 hfov, above 5 images, but rectlinear projection

//////////////////////////////////////////////////////////////////////////////

// The Panorama Druid is a set of tiered heuristics and advice on how to
// improve the current panorama.

// The static "hints" database is a vector of pointers to instances.
// Each instance is a singleton, derived from the basic DruidHint class.
// Each hint definition above will automatically add itself to the array.
// Each hint has a basic detector function and a set of static strings.

/* static */ int PanoDruid::sm_hints = 0;
/* static */ int PanoDruid::sm_chunk = 0;
/* static */ int PanoDruid::sm_sorted = FALSE;
/* static */ DruidHint** PanoDruid::sm_advice = NULL;

/////////////////////////////////////////////////////////////////////////////

PanoDruid::PanoDruid(wxWindow* parent)
		: wxStaticBoxSizer(
			new wxStaticBox(parent, -1, _("the Panorama druid")),
			wxHORIZONTAL)
{
	DEBUG_TRACE("");
	m_parent = parent;
	m_advice = -1;
	m_bitmap.LoadFile(MainFrame::Get()->GetXRCPath() +
					  "/data/" "druid.stitch.128.png",
					  wxBITMAP_TYPE_PNG);
	m_graphic.Create(parent, -1, m_bitmap, wxPoint(0, 0));
	m_text.Create(parent, -1, _(""), wxPoint(0, 0));
	Add(&m_graphic, 0, wxADJUST_MINSIZE);
	Add(&m_text, 1, wxADJUST_MINSIZE);
}

void PanoDruid::Update(const PT::Panorama& pano)
{
    DEBUG_TRACE("PanoramaDruid::Update()");
	const PT::PanoramaOptions& opts = pano.getOptions();

	if (!sm_sorted)
	{
		// sort the array by rank
	}

	int hint = sm_hints;
	while (hint)
	{
		--hint;
		if (!sm_advice[hint])
			continue;
		DEBUG_INFO( "checking hint " << hint
					<< " named \"" << sm_advice[hint]->name << "\"" );
		if (sm_advice[hint]->applies(pano, opts))
			break;
	}

	if (hint < 0)
		return;

	DEBUG_INFO( "PanoDruid::Update() found \""
				<< sm_advice[hint]->name << "\"" );

	// set the controls to contain the appropriate text
	if (m_advice != hint)
	{
		DEBUG_INFO( "updatePanoDruid() updating the visuals" );

		wxString full = sm_advice[hint]->brief;
		full += '\n';
		full += sm_advice[hint]->text;
		m_text.SetLabel(full);
		m_bitmap.LoadFile(MainFrame::Get()->GetXRCPath() +
						  "/data/" + sm_advice[hint]->graphic,
						  wxBITMAP_TYPE_PNG);
		m_graphic.SetBitmap(m_bitmap);
		m_parent->Layout();

		m_advice = hint;
	}
}

/* static */ void PanoDruid::DefineHint(DruidHint* advice)
{
	// First call, or grow the chunk.
	if (sm_hints >= sm_chunk)
	{
		sm_chunk = sm_hints + 10;
		void* target = malloc(sm_chunk * sizeof(DruidHint*));
		if (!target)
			return;
		memset(target, 0, sm_chunk * sizeof(DruidHint*));
		if (sm_advice && sm_hints)
			memcpy(target, sm_advice, sm_hints * sizeof(DruidHint*));
		if (sm_advice)
			free(sm_advice);
		sm_advice = (DruidHint**)target;
	}

	// Keep the hint.
	sm_advice[sm_hints++] = advice;
}

DruidHint* PanoDruid::FindHint(const wxChar* name)
{
	if (!sm_advice || !sm_hints)
		return NULL;
	int i = sm_hints;
	wxString wanted = name;
	while (i)
	{
		--i;
		if (wanted.IsSameAs(sm_advice[i]->name))
			return sm_advice[i];
	}
	return NULL;
}
