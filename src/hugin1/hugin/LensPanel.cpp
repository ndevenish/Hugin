// -*- c-basic-offset: 4 -*-

/** @file LensPanel.cpp
 *
 *  @brief implementation of LensPanel Class
 *
 *  @author Kai-Uwe Behrmann <web@tiscali.de> and
 *          Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  Rewritten by Pablo d'Angelo
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

#include <algorithm>

#include "base_wx/wxPlatform.h"
#include "hugin/LensPanel.h"
#include "hugin/CommandHistory.h"
#include "base_wx/wxImageCache.h"
#include "base_wx/LensTools.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/ImagesList.h"
//#include "hugin/ImageCenter.h"
#include "hugin/ImagesPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "hugin/TextKillFocusHandler.h"
#include "hugin/wxPanoCommand.h"
//#include "hugin/VigCorrDialog.h"
#include "hugin/ResetDialog.h"
#include <wx/arrstr.h>
#include <base_wx/wxLensDB.h>

using namespace PT;
using namespace hugin_utils;
using namespace std;

#define m_XRCID(str_id) \
    wxXmlResource::GetXRCID(str_id)

#ifdef __WXDEBUG__
#define m_XRCCTRL(window, id, type) \
    (wxDynamicCast((window).FindWindow(m_XRCID(id)), type))
#else
#define m_XRCCTRL(window, id, type) \
    ((type*)((window).FindWindow(m_XRCID(id))))
#endif

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(LensPanel, wxPanel) //wxEvtHandler)
    EVT_LIST_ITEM_SELECTED( XRCID("lenses_list_unknown"),
                            LensPanel::ListSelectionChanged )
    EVT_LIST_ITEM_DESELECTED( XRCID("lenses_list_unknown"),
                              LensPanel::ListSelectionChanged )
    EVT_CHOICE (XRCID("lens_val_projectionFormat"),LensPanel::LensTypeChanged)
    EVT_CHOICE (XRCID("lens_val_responseType"),LensPanel::ResponseTypeChanged)
    EVT_TEXT_ENTER ( XRCID("lens_val_v"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_focalLength"),LensPanel::focalLengthChanged)
    EVT_TEXT_ENTER ( XRCID("lens_val_flFactor"),LensPanel::focalLengthFactorChanged)
    EVT_TEXT_ENTER ( XRCID("lens_val_a"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_b"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_c"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_d"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_e"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_g"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_t"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_Eev"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_Er"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_Eb"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_Vb"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_Vc"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_Vd"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_Vx"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_Vy"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_Ra"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_Rb"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_Rc"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_Rd"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_Re"), LensPanel::OnVarChanged )
//    EVT_BUTTON ( XRCID("lens_button_center"), LensPanel::SetCenter )
    EVT_BUTTON ( XRCID("lens_button_loadEXIF"), LensPanel::OnReadExif )
    EVT_BUTTON ( XRCID("lens_button_save"), LensPanel::OnSaveLensParameters )
    EVT_BUTTON ( XRCID("lens_button_load"), LensPanel::OnLoadLensParameters )
    EVT_BUTTON ( XRCID("lens_button_newlens"), LensPanel::OnNewLens )
    EVT_BUTTON ( XRCID("lens_button_changelens"), LensPanel::OnChangeLens )
    EVT_BUTTON ( XRCID("lens_button_reset"), LensPanel::OnReset )
    EVT_CHECKBOX ( XRCID("lens_inherit_v"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_a"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_d"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_g"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_Eev"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_Er"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_Eb"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_R"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_Vb"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_Vx"), LensPanel::OnVarInheritChanged )
END_EVENT_TABLE()

// Define a constructor for the Lenses Panel
LensPanel::LensPanel()
{
    pano = 0,
    m_restoreLayoutOnResize = false;
}


bool LensPanel::Create(wxWindow* parent, wxWindowID id,
                           const wxPoint& pos,
                           const wxSize& size,
                           long style,
                           const wxString& name)
{
    DEBUG_TRACE(" Create called *************");
    if (! wxPanel::Create(parent, id, pos, size, style, name) ) {
        return false;
    }

    DEBUG_TRACE("");
    wxXmlResource::Get()->LoadPanel(this, wxT("lens_panel"));
    wxPanel * panel = XRCCTRL(*this, "lens_panel", wxPanel);

    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
    topsizer->Add(panel, 1, wxEXPAND, 0);
    SetSizer( topsizer );
    //topsizer->SetSizeHints( panel );

    // The following control creates itself. We dont care about xrc loading.
    images_list = XRCCTRL(*this, "lenses_list_unknown", ImagesListLens);
    assert(images_list);

    /*
    images_list = new ImagesListLens (parent, pano);
    wxXmlResource::Get()->AttachUnknownControl (
        wxT("lenses_list_unknown"),
        images_list );
    */   
//    images_list->AssignImageList(img_icons, wxIMAGE_LIST_SMALL );

    // converts KILL_FOCUS events to usable TEXT_ENTER events
    XRCCTRL(*this, "lens_val_v", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_focalLength", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_flFactor", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_a", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_b", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_c", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_d", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_e", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_g", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_t", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));

    XRCCTRL(*this, "lens_val_Eev", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_Er", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_Eb", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_Vb", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_Vc", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_Vd", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_Vx", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_Vy", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_Ra", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_Rb", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_Rc", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_Rd", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "lens_val_Re", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));

    m_degDigits = wxConfigBase::Get()->Read(wxT("/General/DegreeFractionalDigitsEdit"),3);
    m_pixelDigits = wxConfigBase::Get()->Read(wxT("/General/PixelFractionalDigitsEdit"),2);
    m_distDigitsEdit = wxConfigBase::Get()->Read(wxT("/General/DistortionFractionalDigitsEdit"),5);

    m_lens_ctrls = XRCCTRL(*this, "lens_control_panel", wxPanel);
    DEBUG_ASSERT(m_lens_ctrls);

    // fill list of projection formats
    FillLensProjectionList(XRCCTRL(*this, "lens_val_projectionFormat", wxChoice));

    // dummy to disable controls
    wxListEvent ev;
    ListSelectionChanged(ev);

    DEBUG_TRACE("");;

    return true;
}

void LensPanel::Init(PT::Panorama * panorama)
{
    pano = panorama;
    variable_groups = new HuginBase::StandardImageVariableGroups(*pano);
    images_list->Init(pano);
    pano->addObserver(this);
}


LensPanel::~LensPanel(void)
{
    DEBUG_TRACE("dtor");

    XRCCTRL(*this, "lens_val_v", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_focalLength", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_flFactor", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_a", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_b", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_c", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_d", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_e", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_g", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_t", wxTextCtrl)->PopEventHandler(true);

    XRCCTRL(*this, "lens_val_Eev", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_Er", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_Eb", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_Vb", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_Vc", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_Vd", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_Vx", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_Vy", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_Ra", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_Rb", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_Rc", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_Rd", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_Re", wxTextCtrl)->PopEventHandler(true);

    pano->removeObserver(this);
    delete variable_groups;
    DEBUG_TRACE("dtor about to finish");
}

// We need to override the default handling of size events because the
// sizers set the virtual size but not the actual size. We reverse
// the standard handling and fit the child to the parent rather than
// fitting the parent around the child


void LensPanel::UpdateLensDisplay ()
{
    DEBUG_TRACE("");

    if ((m_selectedImages.size() == 0) || (m_selectedLenses.size() == 0)){
        // no image selected
        EnableInputs();
        return;
    }
    if (m_selectedImages.size() != 1) {
        // multiple images selected. do not update,
        // we cant display useful values, because they
        // might be different for each image
        return;
    }
    
    const Lens lens = variable_groups->getLens(*(m_selectedLenses.begin()));
    const VariableMap & imgvars = pano->getImageVariables(*m_selectedImages.begin());

    // update gui
    wxChoice* choice_projection=XRCCTRL(*this, "lens_val_projectionFormat",wxChoice);
    Lens::LensProjectionFormat guiPF = (Lens::LensProjectionFormat)(GetSelectedProjection(choice_projection));
    if (lens.getProjection() != guiPF) {
        DEBUG_DEBUG("changing projection format in gui to: " << lens.getProjection());
        SelectProjection(choice_projection,lens.getProjection());
    }

    // set response type
    XRCCTRL(*this, "lens_val_responseType", wxChoice)->SetSelection(
            pano->getImage(*m_selectedImages.begin()).getResponseType());

    for (const char** varname = m_varNames; *varname != 0; ++varname) {
        // update parameters
        int ndigits = m_distDigitsEdit;
        if (strcmp(*varname, "hfov") == 0 || strcmp(*varname, "d") == 0 ||
            strcmp(*varname, "e") == 0 )
        {
            ndigits = m_pixelDigits;
        }
        m_XRCCTRL(*this, wxString(wxT("lens_val_")).append(wxString(*varname, wxConvLocal)), wxTextCtrl)->SetValue(
            doubleTowxString(const_map_get(imgvars,*varname).getValue(),ndigits));

        bool linked = const_map_get(lens.variables, *varname).isLinked();
        // special case for exposure and response parameters.
        if ((*varname)[0] == 'R' ) {
            m_XRCCTRL(*this, wxT("lens_inherit_R"), wxCheckBox)->SetValue(linked);
        } else if ((*varname)[0] == 'V') {
            if ((*varname)[1] == 'b' || (*varname)[1] == 'x') {
                m_XRCCTRL(*this, wxString(wxT("lens_inherit_")).append(wxString(*varname, wxConvLocal)), wxCheckBox)->SetValue(linked);
            }
        }
        else if (((*varname)[0] != 'b' && (*varname)[0] != 'c' &&
                  (*varname)[0] != 'e' && (*varname)[0] != 't' &&
                  (*varname)[0] != 'y') || (*varname)[1] != '\0')
        {
            m_XRCCTRL(*this, wxString(wxT("lens_inherit_")).append(wxString(*varname, wxConvLocal)), wxCheckBox)->SetValue(linked);
        }
    }

    // update focal length
    const SrcPanoImage &img=pano->getImage(*(m_selectedImages.begin()));
    double focal_length = SrcPanoImage::calcFocalLength(img.getProjection(),img.getHFOV(),
        img.getExifCropFactor(),img.getSize());
    m_XRCCTRL(*this, wxT("lens_val_focalLength"), wxTextCtrl)->SetValue(
        doubleTowxString(focal_length,m_distDigitsEdit));

    // update focal length factor
    double focal_length_factor = lens.getCropFactor();
    m_XRCCTRL(*this, wxT("lens_val_flFactor"), wxTextCtrl)->SetValue(
        doubleTowxString(focal_length_factor,m_distDigitsEdit));


    DEBUG_TRACE("");
}

void LensPanel::panoramaImagesChanged (PT::Panorama &pano, const PT::UIntSet & imgNr)
{
    // Update the lens information
    variable_groups->update();
    // rebuild lens selection, in case a selected lens has been removed.
    UIntSet selImgs;
    m_selectedLenses.clear();
    for (UIntSet::iterator it = m_selectedImages.begin();
         it != m_selectedImages.end(); it++)
    {
        // need to check, since the m_selectedImages list might still be in an old state
        if (*it < pano.getNrOfImages()) {
            selImgs.insert(*it);
            unsigned int lNr = variable_groups->getLenses().getPartNumber(*it);
            m_selectedLenses.insert(lNr);
        }
    }
    // set new selected images.
    m_selectedImages = selImgs;

    // we need to do something if the image we are editing has changed.
    UIntSet intersection;

    std::set_intersection(m_selectedImages.begin(), m_selectedImages.end(),
                          imgNr.begin(), imgNr.end(),
                          inserter(intersection, intersection.begin()));
    if (intersection.size() > 0) {
        UpdateLensDisplay();
    } else if (m_selectedImages.size() == 0) {
        UpdateLensDisplay();
    }
    if(pano.getNrOfImages()>0)
        XRCCTRL(*this, "lens_button_reset", wxButton)->Enable();
    else
        XRCCTRL(*this, "lens_button_reset", wxButton)->Disable();
}


// Here we change the pano->
void LensPanel::LensTypeChanged ( wxCommandEvent & e )
{
    DEBUG_TRACE ("");
    if (m_selectedImages.size() > 0) {
        UIntSet imgs;
        // uses enum HuginBase::SrcPanoImage::Projection from SrcPanoImage.h
        wxChoice* choice_projection=XRCCTRL(*this, "lens_val_projectionFormat", wxChoice);
        HuginBase::SrcPanoImage::Projection var = (HuginBase::SrcPanoImage::Projection)(GetSelectedProjection(choice_projection));
        for (UIntSet::iterator it = m_selectedImages.begin();
             it != m_selectedImages.end(); ++it)
        {
            if (pano->getImage(*it).getProjection() != var)
            {
                imgs.insert(*it);
                DEBUG_INFO ("lens " << *it << " Lenstype " << var);
            }
        }
        GlobalCmdHist::getInstance().addCommand(
            new PT::ChangeImageProjectionCmd(*pano, imgs, var)
        );
    }
}

void LensPanel::ResponseTypeChanged ( wxCommandEvent & e )
{
    DEBUG_TRACE ("");
    HuginBase::ImageVariableGroup & lenses = variable_groups->getLenses();
    if (m_selectedLenses.size() > 0) {
        std::vector<SrcPanoImage> SrcImgs;
        UIntSet imgs;
        for (size_t i = 0 ; i < pano->getNrOfImages(); i++) {
            if (set_contains(m_selectedLenses, lenses.getPartNumber(i)))
            {
                imgs.insert(i);
                HuginBase::SrcPanoImage img=pano->getSrcImage(i);
                img.setResponseType((SrcPanoImage::ResponseType)e.GetSelection());
                SrcImgs.push_back(img);
            }
        }
        GlobalCmdHist::getInstance().addCommand(
            new PT::UpdateSrcImagesCmd( *pano, imgs, SrcImgs )
                                               );
    }
}

void LensPanel::focalLengthChanged ( wxCommandEvent & e )
{
    if (m_selectedImages.size() > 0) {
        DEBUG_TRACE ("");
        double val;
        wxString text=XRCCTRL(*this,"lens_val_focalLength",wxTextCtrl)->GetValue();
        if (!str2double(text, val)) {
            return;
        }

        GlobalCmdHist::getInstance().addCommand(
            new PT::UpdateFocalLengthCmd(*pano,m_selectedImages, val)
        );

    }
}

void LensPanel::focalLengthFactorChanged(wxCommandEvent & e)
{
    DEBUG_TRACE ("");
    if (m_selectedImages.size() > 0) {
        wxString text=XRCCTRL(*this,"lens_val_flFactor",wxTextCtrl)->GetValue();
        DEBUG_INFO("focal length factor: " << text.mb_str(wxConvLocal));
        double val;
        if (!str2double(text, val)) {
            return;
        }

        GlobalCmdHist::getInstance().addCommand(
            new PT::UpdateCropFactorCmd(*pano,m_selectedImages,val)
        );
    }
}


void LensPanel::OnVarChanged(wxCommandEvent & e)
{
    DEBUG_TRACE("")
    if (m_selectedImages.size() > 0) {
        string varname;
        DEBUG_TRACE (" var changed for control with id:" << e.GetId());
        if (e.GetId() == XRCID("lens_val_a")) {
            varname = "a";
        } else if (e.GetId() == XRCID("lens_val_b")) {
            varname = "b";
        } else if (e.GetId() == XRCID("lens_val_c")) {
            varname = "c";
        } else if (e.GetId() == XRCID("lens_val_d")) {
            varname = "d";
        } else if (e.GetId() == XRCID("lens_val_e")) {
            varname = "e";
        } else if (e.GetId() == XRCID("lens_val_g")) {
            varname = "g";
        } else if (e.GetId() == XRCID("lens_val_t")) {
            varname = "t";
        } else if (e.GetId() == XRCID("lens_val_v")) {
            varname = "v";
        } else if (e.GetId() == XRCID("lens_val_Eev")) {
            varname = "Eev";
        } else if (e.GetId() == XRCID("lens_val_Er")) {
            varname = "Er";
        } else if (e.GetId() == XRCID("lens_val_Eb")) {
            varname = "Eb";
        } else if (e.GetId() == XRCID("lens_val_Vb")) {
            varname = "Vb";
        } else if (e.GetId() == XRCID("lens_val_Vc")) {
            varname = "Vc";
        } else if (e.GetId() == XRCID("lens_val_Vd")) {
            varname = "Vd";
        } else if (e.GetId() == XRCID("lens_val_Vx")) {
            varname = "Vx";
        } else if (e.GetId() == XRCID("lens_val_Vy")) {
            varname = "Vy";
        } else if (e.GetId() == XRCID("lens_val_Ra")) {
            varname = "Ra";
        } else if (e.GetId() == XRCID("lens_val_Rb")) {
            varname = "Rb";
        } else if (e.GetId() == XRCID("lens_val_Rc")) {
            varname = "Rc";
        } else if (e.GetId() == XRCID("lens_val_Rd")) {
            varname = "Rd";
        } else if (e.GetId() == XRCID("lens_val_Re")) {
            varname = "Re";
        } else {
            // not reachable
            DEBUG_ASSERT(0);
        }

        wxString ctrl_name(wxT("lens_val_"));
        ctrl_name.append(wxString(varname.c_str(), wxConvLocal));
        double val;
        wxString text = m_XRCCTRL(*this, ctrl_name, wxTextCtrl)->GetValue();
        DEBUG_DEBUG("setting variable " << varname << " to " << text);
        if (!str2double(text, val)){
            return;
        }
        Variable var(varname,val);
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetVariableCmd(*pano, m_selectedImages, var)
            );
    }
}

void LensPanel::OnVarInheritChanged(wxCommandEvent & e)
{
    if (m_selectedLenses.size() > 0)
    {
        DEBUG_TRACE ("");
        // Which variable should be linked or unlinked?
        HuginBase::ImageVariableGroup::ImageVariableEnum varname;
        if (e.GetId() == XRCID("lens_inherit_a") || e.GetId() == XRCID("lens_inherit_b") || e.GetId() == XRCID("lens_inherit_c")) {
            varname = HuginBase::ImageVariableGroup::IVE_RadialDistortion;
        } else if (e.GetId() == XRCID("lens_inherit_d") || e.GetId() == XRCID("lens_inherit_e")) {
            varname = HuginBase::ImageVariableGroup::IVE_RadialDistortionCenterShift;
        } else if (e.GetId() == XRCID("lens_inherit_g") || e.GetId() == XRCID("lens_inherit_t")) {
            varname = HuginBase::ImageVariableGroup::IVE_Shear;
        } else if (e.GetId() == XRCID("lens_inherit_v")) {
            varname = HuginBase::ImageVariableGroup::IVE_HFOV;
        } else if (e.GetId() == XRCID("lens_inherit_Vb")) {
            varname = HuginBase::ImageVariableGroup::IVE_RadialVigCorrCoeff;
        } else if (e.GetId() == XRCID("lens_inherit_Vx")) {
            varname = HuginBase::ImageVariableGroup::IVE_RadialVigCorrCenterShift;
        } else if (e.GetId() == XRCID("lens_inherit_R")) {
            varname = HuginBase::ImageVariableGroup::IVE_EMoRParams;
        } else if (e.GetId() == XRCID("lens_inherit_Eev")) {
            varname = HuginBase::ImageVariableGroup::IVE_ExposureValue;
        } else if (e.GetId() == XRCID("lens_inherit_Er")) {
            varname = HuginBase::ImageVariableGroup::IVE_WhiteBalanceRed;
        } else if (e.GetId() == XRCID("lens_inherit_Eb")) {
            varname = HuginBase::ImageVariableGroup::IVE_WhiteBalanceBlue;
        } else {
            // not reachable
            DEBUG_ASSERT(0);
        }
        // are we linking or unlinking?
        bool inherit = e.IsChecked();
        /* make a set of variables containg the one variable we are changing for
         * the command object. */
        std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> variables;
        variables.insert(varname);
        GlobalCmdHist::getInstance().addCommand(
            new PT::ChangePartImagesLinkingCmd(*pano, m_selectedImages,
                                                 variables, inherit,
                    HuginBase::StandardImageVariableGroups::getLensVariables())
            );
    }
}


void LensPanel::EditVigCorr ( wxCommandEvent & e )
{
    if (m_selectedImages.size() > 0) {
//        VigCorrDialog *dlg = new VigCorrDialog(this, pano, *(m_selectedImages.begin()));
//        dlg->Show();
    }
}

void LensPanel::ListSelectionChanged(wxListEvent& e)
{
    DEBUG_TRACE(e.GetIndex());
    m_selectedImages = images_list->GetSelected();
    m_selectedLenses.clear();
    for (UIntSet::iterator it = m_selectedImages.begin();
         it != m_selectedImages.end(); it++)
    {
        m_selectedLenses.insert(variable_groups->getLenses().getPartNumber(*it));
    }
    DEBUG_DEBUG("selected Images: " << m_selectedImages.size());
    EnableInputs();
};

void LensPanel::EnableInputs()
{
    if (m_selectedImages.size() == 0) {
//        m_editImageNr = UINT_MAX;
//        m_editLensNr = UINT_MAX;
        DEBUG_DEBUG("no selection, disabling value display");
        // clear & disable display
        XRCCTRL(*this, "lens_val_projectionFormat", wxChoice)->Disable();
        XRCCTRL(*this, "lens_val_responseType", wxChoice)->Disable();
        XRCCTRL(*this, "lens_val_v", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_focalLength", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_flFactor", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_a", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_b", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_c", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_d", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_e", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_g", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_t", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_Eev", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_Er", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_Eb", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_Vb", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_Vc", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_Vd", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_Vx", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_Vy", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_Ra", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_Rb", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_Rc", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_Rd", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_Re", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_inherit_v", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_a", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_d", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_g", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_Eev", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_Er", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_Eb", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_Vb", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_Vx", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_R", wxCheckBox)->Disable();

        XRCCTRL(*this, "lens_button_loadEXIF", wxButton)->Disable();
        XRCCTRL(*this, "lens_button_load", wxButton)->Disable();
        XRCCTRL(*this, "lens_button_save", wxButton)->Disable();
        XRCCTRL(*this, "lens_button_newlens", wxButton)->Disable();
        XRCCTRL(*this, "lens_button_changelens", wxButton)->Disable();
    } else {
//        m_editImageNr = *sel.begin();

        // one or more images selected
        if (XRCCTRL(*this, "lens_val_projectionFormat", wxChoice)->Enable()) {
            // enable all other textboxes as well.
            XRCCTRL(*this, "lens_val_responseType", wxChoice)->Enable();
            XRCCTRL(*this, "lens_val_v", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_focalLength", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_flFactor", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_a", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_b", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_c", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_d", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_e", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_g", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_t", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_Eev", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_Er", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_Eb", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_Vb", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_Vc", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_Vd", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_Vx", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_Vy", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_Ra", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_Rb", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_Rc", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_Rd", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_Re", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_inherit_v", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_a", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_d", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_g", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_Eev", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_Er", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_Eb", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_R", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_Vb", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_Vx", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_button_load", wxButton)->Enable();
            XRCCTRL(*this, "lens_button_loadEXIF", wxButton)->Enable();
            XRCCTRL(*this, "lens_button_newlens", wxButton)->Enable();
            XRCCTRL(*this, "lens_button_changelens", wxButton)->Enable();
        }

        if (m_selectedImages.size() == 1) {
            // single selection, its parameters
            XRCCTRL(*this, "lens_button_save", wxButton)->Enable();
            UpdateLensDisplay();
        } else {
            XRCCTRL(*this, "lens_val_v", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_focalLength", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_flFactor", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_a", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_b", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_c", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_d", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_e", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_g", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_t", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_Eev", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_Er", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_Eb", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_Vb", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_Vc", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_Vd", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_Vx", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_Vy", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_Ra", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_Rb", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_Rc", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_Rd", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_Re", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_inherit_v", wxCheckBox)->SetValue(false);
            XRCCTRL(*this, "lens_inherit_a", wxCheckBox)->SetValue(false);
            XRCCTRL(*this, "lens_inherit_d", wxCheckBox)->SetValue(false);
            XRCCTRL(*this, "lens_inherit_g", wxCheckBox)->SetValue(false);
            XRCCTRL(*this, "lens_inherit_Eev", wxCheckBox)->SetValue(false);
            XRCCTRL(*this, "lens_inherit_Er", wxCheckBox)->SetValue(false);
            XRCCTRL(*this, "lens_inherit_Eb", wxCheckBox)->SetValue(false);
            XRCCTRL(*this, "lens_inherit_Vb", wxCheckBox)->SetValue(false);
            XRCCTRL(*this, "lens_inherit_Vx", wxCheckBox)->SetValue(false);
            XRCCTRL(*this, "lens_inherit_R", wxCheckBox)->SetValue(false);
            XRCCTRL(*this, "lens_button_save", wxButton)->Disable();
        }
    }
}


void LensPanel::OnReadExif(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    UIntSet selectedImages = m_selectedImages;
    if (selectedImages.size() > 0) {
        for (UIntSet::iterator it = selectedImages.begin();
             it != selectedImages.end(); ++it)
        {
            unsigned int imgNr = *it;
            // check file extension
            wxFileName file(wxString(pano->getImage(imgNr).getFilename().c_str(), HUGIN_CONV_FILENAME));
            double cropFactor = 0;
            double focalLength = 0;
            SrcPanoImage srcImg = pano->getSrcImage(imgNr);
            bool ok = initImageFromFile(srcImg, focalLength, cropFactor, true);
            if (! ok) {
                if ( ! getLensDataFromUser(this, srcImg, focalLength, cropFactor)) {
                    srcImg.setHFOV(50);
                }
            }
            //initLensFromFile(pano->getImage(imgNr).getFilename().c_str(), c, lens, vars, imgopts, true);
            GlobalCmdHist::getInstance().addCommand(new PT::UpdateSrcImageCmd( *pano, imgNr, srcImg));
        }
    } else {
        wxLogError(_("Please select an image and try again"));
    }

}

void LensPanel::SaveLensParametersToIni()
{
    DEBUG_TRACE("")
    if (m_selectedImages.size() == 1)
    {
        unsigned int imgNr = *(m_selectedImages.begin());
        wxFileDialog dlg(this,
                         _("Save lens parameters file"),
                         wxConfigBase::Get()->Read(wxT("/lensPath"),wxT("")), wxT(""),
                         _("Lens Project Files (*.ini)|*.ini|All files (*)|*"),
                         wxFD_SAVE, wxDefaultPosition);
        dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/lensPath"),wxT("")));
        if (dlg.ShowModal() == wxID_OK)
        {
            wxFileName filename(dlg.GetPath());
            if(!filename.HasExt())
            {
                filename.SetExt(wxT("ini"));
            }
            wxConfig::Get()->Write(wxT("/lensPath"), dlg.GetDirectory());  // remember for later
            SaveLensParameters(filename.GetFullPath(),pano,imgNr);
        }
    }
    else 
    {
        wxLogError(_("Please select an image and try again"));
    }
}

void LensPanel::OnSaveLensParameters(wxCommandEvent & e)
{
    if (m_selectedImages.size() == 1)
    {
        unsigned int imgNr = *(m_selectedImages.begin());
        wxArrayString choices;
        choices.push_back(_("Save lens parameters to ini file"));
        choices.push_back(_("Save lens parameters to lensfun database"));
        choices.push_back(_("Save camera parameters to lensfun database"));
        wxSingleChoiceDialog save_dlg(this,_("Saving lens data"),_("Save lens"),choices);
        if(save_dlg.ShowModal()==wxID_OK)
        {
            switch(save_dlg.GetSelection())
            {
                case 1:
                    SaveLensParameters(this,pano->getImage(imgNr));
                    break;
                case 2:
                    SaveCameraCropFactor(this,pano->getImage(imgNr));
                    break;
                case 0:
                default:
                    SaveLensParametersToIni();
                    break;
            };
        }
    }
    else 
    {
        wxLogError(_("Please select an image and try again"));
    }
};

void LensPanel::OnLoadLensParameters(wxCommandEvent & e)
{
    if (m_selectedImages.size() > 0)
    {
        UIntSet images=m_selectedImages;
        if(images.size()==1)
        {
            if(wxMessageBox(_("You selected only one image.\nShould the loaded parameters applied to all images with the same lens?"),_("Question"), wxICON_QUESTION | wxYES_NO)==wxYES)
            {
                unsigned int lensNr = variable_groups->getLenses().getPartNumber(*images.begin());
                // get all images with the current lens.
                for (size_t i = 0; i < pano->getNrOfImages(); i++)
                {
                    if (variable_groups->getLenses().getPartNumber(i) == lensNr)
                    {
                        images.insert(i);
                    };
                };
            };
        };
        vigra::Size2D sizeImg0=pano->getImage(*(images.begin())).getSize();
        //check if all images have the same size
        bool differentImageSize=false;
        for(UIntSet::const_iterator it=images.begin();it!=images.end() && !differentImageSize;it++)
        {
            differentImageSize=(pano->getImage(*it).getSize()!=sizeImg0);
        };
        if(differentImageSize)
        {
            if(wxMessageBox(_("You selected images with different sizes.\nApply lens parameter file can result in unwanted results.\nApply settings anyway?"), _("Error"), wxICON_QUESTION |wxYES_NO)==wxID_NO)
            {
                return;
            };
        };
        PT::PanoCommand* cmd=NULL;
        wxArrayString choices;
        choices.push_back(_("From ini file"));
        choices.push_back(_("From lensfun database"));
        wxSingleChoiceDialog load_dlg(this,_("Loading lens data from"),_("Load lens"),choices);
        if(load_dlg.ShowModal()==wxID_OK)
        {
            bool isLoaded=false;
            if(load_dlg.GetSelection()==0)
            {
                isLoaded=ApplyLensParameters(this,pano,images,cmd);
            }
            else
            {
                isLoaded=ApplyLensDBParameters(this,pano,images,cmd);
            };
            if(isLoaded)
            {
                GlobalCmdHist::getInstance().addCommand(cmd);
            };
        };
    }
    else
    {
        wxLogError(_("Please select an image and try again"));
    }
}

void LensPanel::OnNewLens(wxCommandEvent & e)
{
    if (m_selectedImages.size() > 0) {
        // create a new lens, start with a copy of the old lens.
        GlobalCmdHist::getInstance().addCommand(
                new PT::NewPartCmd(
                    *pano,
                    m_selectedImages,
                    HuginBase::StandardImageVariableGroups::getLensVariables()
                                  )
            );
    } else {
        wxLogError(_("Please select an image and try again"));
    }
}

void LensPanel::OnChangeLens(wxCommandEvent & e)
{
    if (m_selectedImages.size() > 0) {
        if (variable_groups->getLenses().getNumberOfParts() == 1)
        {
            wxLogError(_("Your project must have at least two lenses before you can change which lens these images use."));
            return;
        }
        // ask user for lens number.
        long nr = wxGetNumberFromUser(
                        _("Enter new lens number"),
                        _("Lens number"),
                        _("Change lens number"), 0, 0,
                        variable_groups->getLenses().getNumberOfParts()-1
                                     );
        if (nr >= 0) {
            // user accepted
            GlobalCmdHist::getInstance().addCommand(
                new PT::ChangePartNumberCmd(*pano, m_selectedImages, nr,
                    HuginBase::StandardImageVariableGroups::getLensVariables())
                );
        }
    } else {
        wxLogError(_("Please select an image and try again"));
    }
}

void LensPanel::OnReset(wxCommandEvent & e)
{
  ResetDialog reset_dlg(this);
  if(reset_dlg.ShowModal()==wxID_OK)
  {
     //reset 
    UIntSet selImg = images_list->GetSelected();
    if ( selImg.size() < 1) {
        // add all images.
        selImg.clear();
        unsigned int nImg = pano->getNrOfImages();
        for (unsigned int i=0; i < nImg; i++) {
            selImg.insert(i);
        }
    }

    if (selImg.size() == 0) {
        return;
    }
    // If we should unlink exposure value (to load it from EXIF)
    bool needs_unlink = false;
    VariableMapVector vars;
    for(UIntSet::const_iterator it = selImg.begin(); it != selImg.end(); it++)
    {
        unsigned int imgNr = *it;
        VariableMap ImgVars=pano->getImageVariables(imgNr);
        if(reset_dlg.GetResetPos())
        {
            map_get(ImgVars,"y").setValue(0);
            map_get(ImgVars,"p").setValue(0);
            map_get(ImgVars,"r").setValue(pano->getSrcImage(imgNr).getExifOrientation());
            map_get(ImgVars,"TrX").setValue(0);
            map_get(ImgVars,"TrY").setValue(0);
            map_get(ImgVars,"TrZ").setValue(0);
        };
        double cropFactor = 0;
        double focalLength = 0;
        double eV = 0;
        SrcPanoImage srcImg = pano->getSrcImage(imgNr);
        if(reset_dlg.GetResetFOV() || reset_dlg.GetResetExposure())
            srcImg.readEXIF(focalLength,cropFactor,eV,false,false);
        if(reset_dlg.GetResetFOV())
        {
            if(focalLength!=0&&cropFactor!=0)
            {
                double newHFOV=calcHFOV(srcImg.getProjection(),focalLength,cropFactor,srcImg.getSize());
                if(newHFOV!=0)
                    map_get(ImgVars,"v").setValue(newHFOV);
            };
        };
        if(reset_dlg.GetResetLens())
        {
            map_get(ImgVars,"a").setValue(0);
            map_get(ImgVars,"b").setValue(0);
            map_get(ImgVars,"c").setValue(0);
            map_get(ImgVars,"d").setValue(0);
            map_get(ImgVars,"e").setValue(0);
            map_get(ImgVars,"g").setValue(0);
            map_get(ImgVars,"t").setValue(0);
        };
        if(reset_dlg.GetResetExposure())
        {
            if(reset_dlg.GetResetExposureToExif())
            {
                //reset to exif value
                
                if (pano->getImage(*it).ExposureValueisLinked())
                {
                    /* Unlink exposure value variable so the EXIF values can be
                     * independant. */
                    needs_unlink = true;
                }
                if(eV!=0)
                    map_get(ImgVars,"Eev").setValue(eV);
            }
            else
            {
                //reset to zero
                map_get(ImgVars,"Eev").setValue(0);
            };
        };
        if(reset_dlg.GetResetColor())
        {
            map_get(ImgVars,"Er").setValue(1);
            map_get(ImgVars,"Eb").setValue(1);
        };
        if(reset_dlg.GetResetVignetting())
        {
            map_get(ImgVars,"Vb").setValue(0);
            map_get(ImgVars,"Vc").setValue(0);
            map_get(ImgVars,"Vd").setValue(0);
            map_get(ImgVars,"Vx").setValue(0);
            map_get(ImgVars,"Vy").setValue(0);

        };
        if(reset_dlg.GetResetResponse())
        {
            map_get(ImgVars,"Ra").setValue(0);
            map_get(ImgVars,"Rb").setValue(0);
            map_get(ImgVars,"Rc").setValue(0);
            map_get(ImgVars,"Rd").setValue(0);
            map_get(ImgVars,"Re").setValue(0);
        };
        vars.push_back(ImgVars);    
    };
    std::vector<PanoCommand *> reset_commands;
    if (needs_unlink)
    {
        std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> variables;
        variables.insert(HuginBase::ImageVariableGroup::IVE_ExposureValue);
        
        reset_commands.push_back(
                new ChangePartImagesLinkingCmd(
                            *pano,
                            selImg,
                            variables,
                            false,
                            HuginBase::StandardImageVariableGroups::getLensVariables())
                );
    }
    reset_commands.push_back(
                            new PT::UpdateImagesVariablesCmd(*pano, selImg,vars)
                                           );
    if(reset_dlg.GetResetExposure())
    {
        //reset panorama output exposure value
        reset_commands.push_back(new PT::ResetToMeanExposure(*pano));
    };
    GlobalCmdHist::getInstance().addCommand(
            new PT::CombinedPanoCommand(*pano, reset_commands));
  }
};

const char *LensPanel::m_varNames[] = { "v", "a", "b", "c", "d", "e", "g", "t",
                                  "Eev", "Er", "Eb", 
                                  "Vb", "Vc", "Vd", "Vx", "Vy",
                                  "Ra", "Rb", "Rc", "Rd", "Re", 0};

IMPLEMENT_DYNAMIC_CLASS(LensPanel, wxPanel)

                                        
LensPanelXmlHandler::LensPanelXmlHandler()
: wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *LensPanelXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, LensPanel)

    cp->Create(m_parentAsWindow,
               GetID(),
               GetPosition(), GetSize(),
               GetStyle(wxT("style")),
               GetName());

    SetupWindow( cp);

    return cp;
}

bool LensPanelXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("LensPanel"));
}

IMPLEMENT_DYNAMIC_CLASS(LensPanelXmlHandler, wxXmlResourceHandler)
                        
