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

#include <config.h>
#include "panoinc_WX.h"
#include "panoinc.h"
#include "hugin/PanoDruid.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"

/////////////////////////////////////////////////////////////////////////////


// Add the hint definitions in reverse order, with increasing rank.
// A good panorama passes all of the tests from the bottom (highest rank)
// to the top (lowest rank).

//////////

NEW_HINT(0, ERROR, wxT("druid.images.128.png"),
         _("The druid has no advice."),
         wxT(""))
{
    return TRUE;
}
END_HINT(ERROR);

//////////

NEW_HINT(1, READY, wxT("druid.stitch.128.png"),
         _("The druid finds no problems with your panorama."),
         _("Stitch your final image now, and then use an image editor\nsuch as the GNU Image Manipulation Program (the GIMP)\nto add any finishing touches."))
{
    return TRUE;
}
END_HINT(READY);

//////////

NEW_HINT(5, UNSAVED, wxT("druid.stitch.128.png"),
         _("Warning:  you haven't saved the current project."),
         _("While everything else seems to be ready to stitch,\ndon't forget to save your project file so you can\nexperiment or adjust the settings later."))
{
    return pano.isDirty();
}
END_HINT(UNSAVED);

//////////

NEW_HINT(20, HUGE_FINAL, wxT("druid.stitch.128.png"),
         _("Warning:  current stitch has huge dimensions."),
         _("Very large pixel dimensions are currently entered.\nSome computers may take an excessively long time\nto render such a large final image.\nFor best results, use the Calculate Optimal Size button on\nthe Panorama Options tab to determine the\npixel dimensions which will give the best quality."))
{
    unsigned long dst_mp = (unsigned long)opts.getWidth() * opts.getHeight();

    // Destination is more than an arbitrary threshold.
    unsigned long threshold = 400000000L; // 400 megapixels
    if (dst_mp >= threshold)
        return TRUE;

    unsigned long src_mp = 0;
    int images = pano.getNrOfImages();
    while (images)
    {
        --images;
        const PanoImage& image =
            pano.getImage(images);
        src_mp += image.getWidth() * image.getHeight();
    }

    // Destination is more all source images.
    if (dst_mp > src_mp)
        return TRUE;

    return FALSE;
}
END_HINT(HUGE_FINAL);

//////////

NEW_HINT(25, LOW_HFOV, wxT("druid.lenses.128.png"),
         _("The Horizontal Field of View (HFOV) may be too low."),
         _("Check that the focal lengths and/or hfov figures\nfor each image are correct for the camera settings.\nThen calculate the visible field of view again.\nHFOV is measured in degrees of arc, usually between\n5 and 120 degrees per image unless using specialized\nlenses."))
{
    return (opts.getHFOV() <= 2.0);
}
END_HINT(LOW_HFOV);

//////////

NEW_HINT(42, NO_PLUMB_GUIDES, wxT("druid.control.128.png"),
         _("Consider adding a vertical or horizontal guide."),
         _("By adding vertical guides, the optimizer can ensure\nthat buildings or trees or other vertical features\nappear vertical in the final result.  A horizontal\nguide can help ensure that a horizon does not bend."))
{
    int images = pano.getNrOfImages();
    if (images < 3)
        return FALSE;

    int points = pano.getNrOfCtrlPoints();
    while (points)
    {
        --points;
        const ControlPoint& point =
            pano.getCtrlPoint(points);
        if (point.mode != ControlPoint::X_Y)
            return FALSE;
    }
    return TRUE;
}
END_HINT(NO_PLUMB_GUIDES);

//////////

NEW_HINT(45, OPTIMIZER_NOT_RUN, wxT("druid.control.128.png"),
         _("Run the Optimizer to estimate the image positions."),
         _("The Optimizer uses the control points to estimate the\npositions of the individual images in the final panorama\n\nThe optimizer can be invoked in the Optimizer tab.\n"))
{
    int images = pano.getNrOfImages();
    if (images > 1) {
        while (images)
        {
            --images;
            const VariableMap & vars = pano.getImageVariables(images);
            if (const_map_get(vars,"y").getValue() != 0.0) {
                return FALSE;
            }
            if (const_map_get(vars,"p").getValue() != 0.0) {
                return FALSE;
            }
            if (const_map_get(vars,"r").getValue() != 0.0) {
                return FALSE;
            }
        }
        return TRUE;
    }
    return FALSE;
}
END_HINT(OPTIMIZER_NOT_RUN);

//////////

NEW_HINT(46, FEW_GUIDES, wxT("druid.control.128.png"),
         _("Add more control points to improve the stitch quality."),
         _("For best results, there should be at least four pairs\nof control points for each pair of overlapping images.\nMore points, accurately placed, will improve the match."))
{
    int points = pano.getNrOfCtrlPoints();
    int images = pano.getNrOfImages();
    if (images == 1 && points <=1 ) {
        return TRUE;
    }
    // for partial panoramas.
    if (images > 1 && (points/3) < (images-1)) {
        return TRUE;
    }
    return FALSE;
}
END_HINT(FEW_GUIDES);

//////////

NEW_HINT(47, UNGUIDED_IMAGE, wxT("druid.control.128.png"),
         _("At least one image has no control points at all."),
         _("For best results, there should be at least four pairs\nof control points for each pair of overlapping images.\nAn image with no control points cannot be aligned."))
{
    int images = pano.getNrOfImages();
    while (images)
    {
        --images;
        std::vector<unsigned int> points =
            pano.getCtrlPointsForImage(images);
        if (points.size() == 0)
            return TRUE;
    }
    return FALSE;
}
END_HINT(UNGUIDED_IMAGE);

//////////

NEW_HINT(48, NO_GUIDES, wxT("druid.control.128.png"),
         _("Add stitching control points to each pair of images."),
         _("The Optimizer relies on your control points to arrange\nand blend the images properly.  On the Control Points\ntab, add pairs of points that correspond to identical\nvisual features in each pair of overlapping images."))
{
    return (0 == pano.getNrOfCtrlPoints());
}
END_HINT(NO_GUIDES);

//////////

NEW_HINT(95, ONE_IMAGE, wxT("druid.images.128.png"),
         _("Add at least one more image."),
         _("You should have at least two files listed in the Images tab."))
{
    // Ignores this hint if they've already added some control points.
    if (pano.getNrOfCtrlPoints())
        return FALSE;
    if (1 != pano.getNrOfImages())
        return FALSE;
    return TRUE;
}
END_HINT(ONE_IMAGE);

//////////

NEW_HINT(100, NO_IMAGES, wxT("druid.images.128.png"),
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
//TODO: above 90 hfov, above 5 images, but rectilinear projection
//TODO: detect bend panoramas?

//////////////////////////////////////////////////////////////////////////////

// The Panorama Druid is a set of tiered heuristics and advice on how to
// improve the current panorama.

// The static "hints" database is a vector of pointers to instances.
// The hints have to be added to the PanoDruid in the PanoDruid::AddHints()
// method
// Each hint has a basic detector function and a set of static strings.

#if 0
/* static */ int PanoDruid::sm_hints = 0;
/* static */ int PanoDruid::sm_chunk = 0;
/* static */ int PanoDruid::sm_sorted = FALSE;
/* static */ DruidHint** PanoDruid::sm_advice = NULL;
#endif

/////////////////////////////////////////////////////////////////////////////

PanoDruid::PanoDruid(wxWindow* parent)
  : wxPanel(parent)
{
    DEBUG_TRACE("");
    sm_hints = 0;
    sm_chunk = 0;
    sm_sorted = 0;
    sm_advice = NULL;

    m_boxSizer = new wxStaticBoxSizer(
        new wxStaticBox(this, -1, _("the Panorama druid")),
        wxHORIZONTAL );

    m_advice = -1;
    m_bitmap.LoadFile(huginApp::Get()->GetXRCPath() +
                      wxT("data/") + wxT("druid.stitch.128.png"),
                      wxBITMAP_TYPE_PNG);
    m_graphic = new wxStaticBitmap(this, -1, m_bitmap, wxPoint(0,0));
    m_text = new wxStaticText(this, -1, wxT(""),wxPoint(0,0));
    m_boxSizer->Add(m_graphic, 0, wxADJUST_MINSIZE);
    m_boxSizer->Add(m_text, 1, wxADJUST_MINSIZE);
    // populate hint database
    AddHints();
    SetSizer(m_boxSizer);
}

PanoDruid::~PanoDruid()
{
	// delete the hints...
    for (int i = 0; i < sm_hints; i++) {
        delete sm_advice[i];
    }
	// ...and free the allocated memory
	free(sm_advice);
}

void PanoDruid::AddHints()
{
    DefineHint(new hintERROR);
    DefineHint(new hintREADY);
    DefineHint(new hintUNSAVED);
    DefineHint(new hintHUGE_FINAL);
    DefineHint(new hintLOW_HFOV);
    DefineHint(new hintNO_PLUMB_GUIDES);
    DefineHint(new hintOPTIMIZER_NOT_RUN);
    DefineHint(new hintFEW_GUIDES);
    DefineHint(new hintUNGUIDED_IMAGE);
    DefineHint(new hintNO_GUIDES);
//    DefineHint(new hintONE_IMAGE);
    DefineHint(new hintNO_IMAGES);
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
                    << " named \"" << sm_advice[hint]->name.mb_str(wxConvLocal) << "\"" );
        if (sm_advice[hint]->applies(pano, opts))
            break;
    }

    if (hint < 0)
        return;

    DEBUG_INFO( "PanoDruid::Update() found \""
    		    << sm_advice[hint]->name.mb_str(wxConvLocal) << "\"" );

    // set the controls to contain the appropriate text
    if (m_advice != hint)
    {
        DEBUG_INFO( "updatePanoDruid() updating the visuals" );

        wxString full = wxGetTranslation(sm_advice[hint]->brief);
        full += '\n';
        full += wxGetTranslation(sm_advice[hint]->text);
        m_text->SetLabel(full);
        m_bitmap.LoadFile(huginApp::Get()->GetXRCPath() +
                          wxT("data/") + sm_advice[hint]->graphic,
                          wxBITMAP_TYPE_PNG);
        m_graphic->SetBitmap(m_bitmap);
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



//////////////////////////////////////////////////////////////////////////////

// The Panorama Druid is a set of tiered heuristics and advice on how to
// improve the current panorama.

struct advocation
{
    const wxChar* name;
    const wxChar* graphic;
    const wxChar* brief;
    const wxChar* text;
};

static struct advocation _advice[] =
{
    { wxT("ERROR"), wxT("druid.images.128.png"), // "ERROR" must be at index 0
    _("The druid has no advice at this time."), wxT("") },

    { wxT("READY"), wxT("druid.stitch.128.png"),
    _("The druid finds no problems with your panorama."),
    _("Stitch your final image now, and then use an image editor\nsuch as the GNU Image Manipulation Program (the GIMP)\nto add any finishing touches.") },

    { wxT("NO IMAGES"), wxT("druid.images.128.png"),
    _("To get started, add some image files."),
    _("You can add any number of images using the Images tab.") },

    { wxT("ONE IMAGE"), wxT("druid.images.128.png"),
    _("Add at least one more image."),
    _("You should have at least two files listed in the Images tab.") },

    { wxT("LOW HFOV"), wxT("druid.lenses.128.png"),
    _("The Horizontal Field of View (HFOV) may be too low."),
    _("Check that the focal lengths and/or hfov figures\nfor each image are correct for the camera settings.\nThen calculate the visible field of view again.\nHFOV is measured in degrees of arc, usually between\n5 and 120 degrees per image unless using specialized\nlenses.") },

    { wxT("HUGE FINAL"), wxT("druid.stitch.128.png"),
    _("Warning:  current stitch has huge dimensions."),
    _("Very large pixel dimensions are currently entered.\nSome computers may take an excessively long time\nto render such a large final image.\nFor best results, use the automatic Calc button on\nthe Panorama Options tab to determine the\npixel dimensions which will give the best quality.") },

    { wxT("UNSAVED"), wxT("druid.stitch.128.png"),
    _("Warning:  you haven't saved the current project."),
    _("While everything else seems to be ready to stitch,\ndon't forget to save your project file so you can\nexperiment or adjust the settings later.") },

    { NULL, NULL, wxT("") }
};
