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

#include "jhead/jhead.h"

#include "common/wxPlatform.h"
#include "hugin/LensPanel.h"
#include "hugin/CommandHistory.h"
#include "hugin/ImageCache.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/ImagesList.h"
#include "hugin/ImageCenter.h"
#include "hugin/ImagesPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "hugin/TextKillFocusHandler.h"
#include "hugin/wxPanoCommand.h"

using namespace PT;
using namespace utils;
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

BEGIN_EVENT_TABLE(LensPanel, wxWindow) //wxEvtHandler)
    EVT_SIZE   ( LensPanel::FitParent )
    EVT_LIST_ITEM_SELECTED( XRCID("lenses_list_unknown"),
                            LensPanel::ListSelectionChanged )
    EVT_LIST_ITEM_DESELECTED( XRCID("lenses_list_unknown"),
                              LensPanel::ListSelectionChanged )
    EVT_COMBOBOX (XRCID("lens_val_projectionFormat"),LensPanel::LensTypeChanged)
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
    EVT_BUTTON ( XRCID("lens_button_center"), LensPanel::SetCenter )
    EVT_BUTTON ( XRCID("lens_button_loadEXIF"), LensPanel::OnReadExif )
    EVT_BUTTON ( XRCID("lens_button_save"), LensPanel::OnSaveLensParameters )
    EVT_BUTTON ( XRCID("lens_button_load"), LensPanel::OnLoadLensParameters )
    EVT_BUTTON ( XRCID("lens_button_newlens"), LensPanel::OnNewLens )
    EVT_BUTTON ( XRCID("lens_button_changelens"), LensPanel::OnChangeLens )
    EVT_CHECKBOX ( XRCID("lens_inherit_v"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_a"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_b"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_c"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_d"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_e"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_g"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_t"), LensPanel::OnVarInheritChanged )
END_EVENT_TABLE()

// Define a constructor for the Lenses Panel

LensPanel::LensPanel(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama* pano)
    : wxPanel (parent, -1, wxDefaultPosition, wxDefaultSize, wxEXPAND|wxGROW),
      pano(*pano)
{
    DEBUG_TRACE("ctor");
    pano->addObserver(this);

    // This controls must called by xrc handler and after it we play with it.
    wxXmlResource::Get()->LoadPanel (this, wxT("lens_panel"));

    // The following control creates itself. We dont care about xrc loading.
    images_list = new ImagesListLens (parent, pano);
    wxXmlResource::Get()->AttachUnknownControl (
        wxT("lenses_list_unknown"),
        images_list );
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

    m_degDigits = wxConfigBase::Get()->Read(wxT("/General/DegreeFractionalDigitsEdit"),3);
    m_pixelDigits = wxConfigBase::Get()->Read(wxT("/General/PixelFractionalDigitsEdit"),2);
    m_distDigitsEdit = wxConfigBase::Get()->Read(wxT("/General/DistortionFractionalDigitsEdit"),5);

#ifdef USE_WX25x
    m_lens_ctrls = XRCCTRL(*this, "lens_control_panel", wxScrolledWindow);
    DEBUG_ASSERT(m_lens_ctrls);
    m_lens_splitter = XRCCTRL(*this, "lens_panel_splitter", wxSplitterWindow);
    DEBUG_ASSERT(m_lens_splitter);

    m_lens_ctrls->FitInside();
    m_lens_ctrls->SetScrollRate(10, 10);
    m_lens_splitter->SetSashPosition(wxConfigBase::Get()->Read(wxT("/LensFrame/sashPos"),300));
    m_lens_splitter->SetMinimumPaneSize(20);
#endif

    // dummy to disable controls
    wxListEvent ev;
    ListSelectionChanged(ev);
    DEBUG_TRACE("");;
}


LensPanel::~LensPanel(void)
{
    DEBUG_TRACE("dtor");

#ifdef USE_WX25x
    int sashPos;
    sashPos = m_lens_splitter->GetSashPosition();
    DEBUG_INFO("Lens panel sash pos: " << sashPos);
    wxConfigBase::Get()->Write(wxT("/LensFrame/sashPos"), sashPos);
#endif

    // FIXME. why does this crash at exit?
    XRCCTRL(*this, "lens_val_v", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_flFactor", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_focalLength", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_a", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_b", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_c", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_d", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_e", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_g", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "lens_val_t", wxTextCtrl)->PopEventHandler(true);

    pano.removeObserver(this);
    DEBUG_TRACE("dtor about to finish");
}

void LensPanel::FitParent( wxSizeEvent & e )
{
#ifdef USE_WX25x
    int winWidth, winHeight;
    GetClientSize(&winWidth, &winHeight);
    // winHeight -= ConvertDialogToPixels(wxPoint(0, 30)).y;   // sizes of tabs and toolbar
    XRCCTRL(*this, "lens_panel", wxPanel)->SetSize (winWidth, winHeight);
    DEBUG_INFO( "lens panel: " << winWidth <<"x"<< winHeight );
    m_lens_splitter->SetSize( winWidth, winHeight );
    m_lens_splitter->GetWindow2()->GetSize(&winWidth, &winHeight);
    DEBUG_INFO( "lens controls: " << winWidth <<"x"<< winHeight );
    m_lens_ctrls->SetSize(winWidth, winHeight);
#else
    wxSize new_size = GetSize();
//    wxSize new_size = GetParent()->GetSize();
    XRCCTRL(*this, "lens_panel", wxPanel)->SetSize ( new_size );
//    DEBUG_INFO( "" << new_size.GetWidth() <<"x"<< new_size.GetHeight()  );
#endif
}

void LensPanel::UpdateLensDisplay ()
{
    DEBUG_TRACE("");
    if (m_selectedImages.size() == 0) {
        // no image selected
        return;
    }
    if (m_selectedImages.size() != 1) {
        // multiple images selected. do not update,
        // we cant display useful values, because they
        // might be different for each image
        return;
    }


    const Lens & lens = pano.getLens(*(m_selectedLenses.begin()));
    const VariableMap & imgvars = pano.getImageVariables(*m_selectedImages.begin());

    // update gui
    int guiPF = XRCCTRL(*this, "lens_val_projectionFormat",
                      wxComboBox)->GetSelection();
    if (lens.getProjection() != (Lens::LensProjectionFormat) guiPF) {
        DEBUG_DEBUG("changing projection format in gui to: " << lens.getProjection());
        XRCCTRL(*this, "lens_val_projectionFormat", wxComboBox)->SetSelection(
            lens.getProjection()  );
    }

    for (char** varname = Lens::variableNames; *varname != 0; ++varname) {
        // update parameters
        int ndigits = m_distDigitsEdit;
        if (strcmp(*varname, "hfov") == 0 || strcmp(*varname, "d") == 0 ||
            strcmp(*varname, "e") == 0 )
        {
            ndigits = m_pixelDigits;
        }
        m_XRCCTRL(*this, wxString(wxT("lens_val_")).append(wxString(*varname, *wxConvCurrent)), wxTextCtrl)->SetValue(
            doubleTowxString(const_map_get(imgvars,*varname).getValue(),ndigits));
        bool linked = const_map_get(lens.variables, *varname).isLinked();
        m_XRCCTRL(*this, wxString(wxT("lens_inherit_")).append(wxString(*varname, *wxConvCurrent)), wxCheckBox)->SetValue(linked);
    }

    // update focal length
    double focal_length = lens.getFocalLength();
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

    // we need to do something if the image we are editing has changed.
    bool update=false;
    UIntSet intersection;

    std::set_intersection(m_selectedLenses.begin(), m_selectedLenses.end(),
                          imgNr.begin(), imgNr.end(),
                          inserter(intersection, intersection.begin()));
    if (intersection.size() > 0) {
        UpdateLensDisplay();
    }
}


// Here we change the pano.
void LensPanel::LensTypeChanged ( wxCommandEvent & e )
{
    DEBUG_TRACE ("")
    if (m_selectedLenses.size() > 0) {
        for (UIntSet::iterator it = m_selectedLenses.begin();
             it != m_selectedLenses.end(); ++it)
        {
            // get lens from pano
            Lens lens = pano.getLens(*it);
            // uses enum Lens::LensProjectionFormat from PanoramaMemento.h
            int var = XRCCTRL(*this, "lens_val_projectionFormat",
                              wxComboBox)->GetSelection();
            if (lens.getProjection() != (Lens::LensProjectionFormat) var) {
                lens.setProjection((Lens::LensProjectionFormat) (var));
                GlobalCmdHist::getInstance().addCommand(
                    new PT::ChangeLensCmd( pano, *it, lens )
                    );
                DEBUG_INFO ("lens " << *it << " Lenstype " << var);
            }
        }
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


        VariableMapVector vars;
        UIntSet lensNrs;
        for (UIntSet::const_iterator it=m_selectedImages.begin();
             it != m_selectedImages.end();
             ++it)
        {
            vars.push_back(pano.getImageVariables(*it));
            Lens l = pano.getLens(pano.getImage(*it).getLensNr());
            l.setFocalLength(val);
            map_get(vars.back(),"v").setValue( map_get(l.variables,"v").getValue() );
        }

        GlobalCmdHist::getInstance().addCommand(
            new PT::UpdateImagesVariablesCmd(pano, m_selectedImages, vars)
            );
    }
}

void LensPanel::focalLengthFactorChanged(wxCommandEvent & e)
{
    DEBUG_TRACE ("");
    if (m_selectedImages.size() > 0) {
        wxString text=XRCCTRL(*this,"lens_val_flFactor",wxTextCtrl)->GetValue();
        DEBUG_INFO("focal length factor: " << text.mb_str());
        double val;
        if (!str2double(text, val)) {
            return;
        }

        UIntSet lensNrs;

        for (UIntSet::const_iterator it=m_selectedImages.begin();
             it != m_selectedImages.end();
             ++it)
        {
            lensNrs.insert(pano.getImage(*it).getLensNr());
        }

        vector<Lens> lenses;
        for (UIntSet::const_iterator it=lensNrs.begin(); it != lensNrs.end();
             ++it)
        {
            lenses.push_back(pano.getLens(*it));
            double fl = lenses.back().getFocalLength();
            lenses.back().setCropFactor(val);
            lenses.back().setFocalLength(fl);
        }
        GlobalCmdHist::getInstance().addCommand(
            new PT::ChangeLensesCmd( pano, lensNrs, lenses)
            );
    }
}


void LensPanel::OnVarChanged(wxCommandEvent & e)
{
    DEBUG_TRACE("")
    if (m_selectedImages.size() > 0) {
        string varname;
        DEBUG_TRACE (" var changed for control with id:" << e.m_id);
        if (e.m_id == XRCID("lens_val_a")) {
            varname = "a";
        } else if (e.m_id == XRCID("lens_val_b")) {
            varname = "b";
        } else if (e.m_id == XRCID("lens_val_c")) {
            varname = "c";
        } else if (e.m_id == XRCID("lens_val_d")) {
            varname = "d";
        } else if (e.m_id == XRCID("lens_val_e")) {
            varname = "e";
        } else if (e.m_id == XRCID("lens_val_g")) {
            varname = "g";
        } else if (e.m_id == XRCID("lens_val_t")) {
            varname = "t";
        } else if (e.m_id == XRCID("lens_val_v")) {
            varname = "v";
        } else {
            // not reachable
            DEBUG_ASSERT(0);
        }
        wxString ctrl_name(wxT("lens_val_"));
        ctrl_name.append(wxString(varname.c_str(), *wxConvCurrent));
        double val;
        wxString text = m_XRCCTRL(*this, ctrl_name, wxTextCtrl)->GetValue();
        DEBUG_DEBUG("setting variable " << varname << " to " << text);
        if (!str2double(text, val)){
            return;
        }
        Variable var(varname,val);
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetVariableCmd(pano, m_selectedImages, var)
            );
    }
}

void LensPanel::OnVarInheritChanged(wxCommandEvent & e)
{
    if (m_selectedLenses.size() > 0) {
        DEBUG_TRACE ("");
        std::string varname;
        if (e.m_id == XRCID("lens_inherit_a")) {
            varname = "a";
        } else if (e.m_id == XRCID("lens_inherit_b")) {
            varname = "b";
        } else if (e.m_id == XRCID("lens_inherit_c")) {
            varname = "c";
        } else if (e.m_id == XRCID("lens_inherit_d")) {
            varname = "d";
        } else if (e.m_id == XRCID("lens_inherit_e")) {
            varname = "e";
        } else if (e.m_id == XRCID("lens_inherit_g")) {
            varname = "g";
        } else if (e.m_id == XRCID("lens_inherit_t")) {
            varname = "t";
        } else if (e.m_id == XRCID("lens_inherit_v")) {
            varname = "v";
        } else {
            // not reachable
            DEBUG_ASSERT(0);
        }

        wxString ctrl_name(wxT("lens_inherit_"));
        ctrl_name.append(wxString(varname.c_str(), *wxConvCurrent));
        bool inherit = m_XRCCTRL(*this, ctrl_name, wxCheckBox)->IsChecked();
        for (UIntSet::iterator it = m_selectedLenses.begin();
             it != m_selectedLenses.end(); ++it)
        {
            // get the current Lens.
            unsigned int lensNr = *it;
            LensVariable lv = const_map_get(pano.getLens(lensNr).variables, varname);
            lv.setLinked(inherit);
            LensVarMap lmap;
            lmap.insert(make_pair(lv.getName(),lv));
            GlobalCmdHist::getInstance().addCommand(
                new PT::SetLensVariableCmd(pano, *it, lmap)
                );
        }
    }
}

void LensPanel::SetCenter ( wxCommandEvent & e )
{
//    wxLogError(_("temporarily disabled"));
    if (m_selectedImages.size() == 1) {
        ImgCenter *dlg = new ImgCenter((wxWindow*)this, wxDefaultPosition, wxDefaultSize, pano, m_selectedImages);

        // show an image preview
        wxImage * img = ImageCache::getInstance().getImage(
            pano.getImage(*(m_selectedImages.begin())).getFilename());
        dlg->ChangeView(*img);
        dlg->CentreOnParent ();
        dlg->Refresh();
        dlg->Show(true);
    }
    DEBUG_TRACE ("")
}


void LensPanel::ListSelectionChanged(wxListEvent& e)
{
    DEBUG_TRACE(e.GetIndex());
    m_selectedImages = images_list->GetSelected();
    m_selectedLenses.clear();
    for (UIntSet::iterator it = m_selectedImages.begin();
         it != m_selectedImages.end(); it++)
    {
        m_selectedLenses.insert(pano.getImage(*it).getLensNr());
    }
    DEBUG_DEBUG("selected Images: " << m_selectedImages.size());
    if (m_selectedImages.size() == 0) {
//        m_editImageNr = UINT_MAX;
//        m_editLensNr = UINT_MAX;
        DEBUG_DEBUG("no selection, disabling value display");
        // clear & disable display
        XRCCTRL(*this, "lens_val_projectionFormat", wxComboBox)->Disable();
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
        XRCCTRL(*this, "lens_inherit_v", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_a", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_b", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_c", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_d", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_e", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_g", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_t", wxCheckBox)->Disable();

        XRCCTRL(*this, "lens_button_center", wxButton)->Disable();
        XRCCTRL(*this, "lens_button_loadEXIF", wxButton)->Disable();
        XRCCTRL(*this, "lens_button_load", wxButton)->Disable();
        XRCCTRL(*this, "lens_button_save", wxButton)->Disable();
    } else {
//        m_editImageNr = *sel.begin();

        // one or more images selected
        if (XRCCTRL(*this, "lens_val_projectionFormat", wxComboBox)->Enable()) {
            // enable all other textboxes as well.
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
            XRCCTRL(*this, "lens_inherit_v", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_a", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_b", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_c", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_d", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_e", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_g", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_t", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_button_loadEXIF", wxButton)->Enable();
        }

        if (m_selectedImages.size() == 1) {
            // single selection, its parameters
            // update values
            unsigned int img = *(m_selectedImages.begin());
            DEBUG_DEBUG("updating LensPanel with Image " << img);
            XRCCTRL(*this, "lens_button_center", wxButton)->Enable();
            XRCCTRL(*this, "lens_button_load", wxButton)->Enable();
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
            XRCCTRL(*this, "lens_inherit_v", wxCheckBox)->SetValue(false);
            XRCCTRL(*this, "lens_inherit_a", wxCheckBox)->SetValue(false);
            XRCCTRL(*this, "lens_inherit_b", wxCheckBox)->SetValue(false);
            XRCCTRL(*this, "lens_inherit_c", wxCheckBox)->SetValue(false);
            XRCCTRL(*this, "lens_inherit_d", wxCheckBox)->SetValue(false);
            XRCCTRL(*this, "lens_inherit_e", wxCheckBox)->SetValue(false);
            XRCCTRL(*this, "lens_inherit_g", wxCheckBox)->SetValue(false);
            XRCCTRL(*this, "lens_inherit_t", wxCheckBox)->SetValue(false);

            XRCCTRL(*this, "lens_button_center", wxButton)->Disable();
            XRCCTRL(*this, "lens_button_load", wxButton)->Disable();
            XRCCTRL(*this, "lens_button_save", wxButton)->Disable();
        }
    }
}


void LensPanel::OnReadExif(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    if (m_selectedImages.size() > 0) {
        UIntSet tmpLenses;
        for (UIntSet::iterator it = m_selectedImages.begin();
             it != m_selectedImages.end(); ++it)
        {
            unsigned int imgNr = *it;
            unsigned int lensNr = pano.getImage(imgNr).getLensNr();
            if (! set_contains(tmpLenses, lensNr)) {
                tmpLenses.insert(lensNr);
                Lens lens = pano.getLens(lensNr);
                // check file extension
                wxFileName file(wxString(pano.getImage(imgNr).getFilename().c_str(), *wxConvCurrent));
                if (file.GetExt().CmpNoCase(wxT("jpg")) == 0 ||
                    file.GetExt().CmpNoCase(wxT("jpeg")) == 0 )
                {
                    double c=0;
                    initLensFromFile(pano.getImage(imgNr).getFilename().c_str(), c, lens);
                    GlobalCmdHist::getInstance().addCommand(
                        new PT::ChangeLensCmd( pano, lensNr,
                                               lens )
                        );
                } else {
                    wxLogError(_("Not a jpeg file:") + file.GetName());
                }
            }
        }
    } else {
        wxLogError(_("Please select an image and try again"));
    }

}



void LensPanel::OnSaveLensParameters(wxCommandEvent & e)
{
    DEBUG_TRACE("")
    if (m_selectedImages.size() == 1) {
        unsigned int imgNr = *(m_selectedImages.begin());
        unsigned int lensNr = pano.getImage(imgNr).getLensNr();
        const Lens & lens = pano.getLens(lensNr);
        const VariableMap & vars = pano.getImageVariables(imgNr);
        // get the variable map
        wxString fname;
        wxFileDialog dlg(this,
                         _("Save lens parameters file"),
                         wxConfigBase::Get()->Read(wxT("lensPath"),wxT("")), wxT(""),
                         _("Lens Project Files (*.ini)|*.ini|All files (*.*)|*.*"),
                         wxSAVE, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            fname = dlg.GetPath();
            wxConfig::Get()->Write(wxT("lensPath"), dlg.GetDirectory());  // remember for later
            // set numeric locale to C, for correct number output
            char * old_locale = setlocale(LC_NUMERIC,NULL);
            setlocale(LC_NUMERIC,"C");
            {
                wxFileConfig cfg(wxT("hugin lens file"),wxT(""),fname);
                cfg.Write(wxT("Lens/type"), (long) lens.getProjection());
                cfg.Write(wxT("Lens/hfov"), const_map_get(vars,"v").getValue());
                cfg.Write(wxT("Lens/hfov_link"), const_map_get(lens.variables,"v").isLinked() ? 1:0);
                cfg.Write(wxT("Lens/crop"), lens.getCropFactor());
                cfg.Write(wxT("Lens/a"), const_map_get(vars,"a").getValue());
                cfg.Write(wxT("Lens/a_link"), const_map_get(lens.variables,"a").isLinked() ? 1:0);
                cfg.Write(wxT("Lens/b"), const_map_get(vars,"b").getValue());
                cfg.Write(wxT("Lens/b_link"), const_map_get(lens.variables,"b").isLinked() ? 1:0);
                cfg.Write(wxT("Lens/c"), const_map_get(vars,"c").getValue());
                cfg.Write(wxT("Lens/c_link"), const_map_get(lens.variables,"c").isLinked() ? 1:0);
                cfg.Write(wxT("Lens/d"), const_map_get(vars,"d").getValue());
                cfg.Write(wxT("Lens/d_link"), const_map_get(lens.variables,"d").isLinked() ? 1:0);
                cfg.Write(wxT("Lens/e"), const_map_get(vars,"e").getValue());
                cfg.Write(wxT("Lens/e_link"), const_map_get(lens.variables,"e").isLinked() ? 1:0);
                cfg.Write(wxT("Lens/t"), const_map_get(vars,"t").getValue());
                cfg.Write(wxT("Lens/t_link"), const_map_get(lens.variables,"t").isLinked() ? 1:0);
                cfg.Write(wxT("Lens/g"), const_map_get(vars,"g").getValue());
                cfg.Write(wxT("Lens/g_link"), const_map_get(lens.variables,"g").isLinked() ? 1:0);
                cfg.Flush();
            }
            // reset locale
            setlocale(LC_NUMERIC,old_locale);
        }
    } else {
        wxLogError(_("Please select an image and try again"));
    }
}


void LensPanel::OnLoadLensParameters(wxCommandEvent & e)
{
    if (m_selectedImages.size() == 1) {
        unsigned int imgNr = *(m_selectedImages.begin());
        unsigned int lensNr = pano.getImage(imgNr).getLensNr();
        Lens lens = pano.getLens(lensNr);
        VariableMap vars = pano.getImageVariables(imgNr);
        wxString fname;
        wxFileDialog dlg(this,
                         _("Load lens parameters"),
                         wxConfigBase::Get()->Read(wxT("lensPath"),wxT("")), wxT(""),
                         _("Lens Project Files (*.ini)|*.ini|All files (*.*)|*.*"),
                         wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            fname = dlg.GetPath();
            wxConfig::Get()->Write(wxT("lensPath"), dlg.GetDirectory());  // remember for later
            // read with with standart C numeric format
            char * old_locale = setlocale(LC_NUMERIC,NULL);
            setlocale(LC_NUMERIC,"C");
            {
                wxFileConfig cfg(wxT("hugin lens file"),wxT(""),fname);
                int integer=0;
                double d;
                cfg.Read(wxT("Lens/type"), &integer);
                lens.setProjection ((Lens::LensProjectionFormat) integer);
                cfg.Read(wxT("Lens/hfov"), &d);map_get(vars,"v").setValue(d);
                cfg.Read(wxT("Lens/crop"), &d);lens.setCropFactor(d);
                cfg.Read(wxT("Lens/a"), &d);map_get(vars,"a").setValue(d);
                cfg.Read(wxT("Lens/b"), &d);map_get(vars,"b").setValue(d);
                cfg.Read(wxT("Lens/c"), &d);map_get(vars,"c").setValue(d);
                cfg.Read(wxT("Lens/d"), &d);map_get(vars,"d").setValue(d);
                cfg.Read(wxT("Lens/e"), &d);map_get(vars,"e").setValue(d);
                cfg.Read(wxT("Lens/t"), &d);map_get(vars,"t").setValue(d);
                cfg.Read(wxT("Lens/g"), &d);map_get(vars,"g").setValue(d);

                integer = 1;
                cfg.Read(wxT("Lens/hfov_link"), &integer);
                map_get(lens.variables, "v").setLinked(integer);
                integer = 1;
                cfg.Read(wxT("Lens/a_link"), &integer);
                map_get(lens.variables, "a").setLinked(integer);
                integer = 1;
                cfg.Read(wxT("Lens/b_link"), &integer);
                map_get(lens.variables, "b").setLinked(integer);
                integer = 1;
                cfg.Read(wxT("Lens/c_link"), &integer);
                map_get(lens.variables, "c").setLinked(integer);
                integer = 1;
                cfg.Read(wxT("Lens/d_link"), &integer);
                map_get(lens.variables, "d").setLinked(integer);
                integer = 1;
                cfg.Read(wxT("Lens/e_link"), &integer);
                map_get(lens.variables, "e").setLinked(integer);
                integer = 0;
                cfg.Read(wxT("Lens/t_link"), &integer);
                map_get(lens.variables, "t").setLinked(integer);
                integer = 0;
                cfg.Read(wxT("Lens/g_link"), &integer);
                map_get(lens.variables, "g").setLinked(integer);
            }
            // reset locale
            setlocale(LC_NUMERIC,old_locale);

            GlobalCmdHist::getInstance().addCommand(
                new PT::ChangeLensCmd(pano, lensNr, lens)
                );
            GlobalCmdHist::getInstance().addCommand(
                new PT::UpdateImageVariablesCmd(pano, imgNr, vars)
                );
        }
    } else {
        wxLogError(_("Please select an image and try again"));
    }
}

void LensPanel::OnNewLens(wxCommandEvent & e)
{
    if (m_selectedImages.size() > 0) {
        // create a new lens, try to read info from first image
        unsigned int imgNr = *(m_selectedImages.begin());
        Lens l;
        double crop=0;
        initLensFromFile(pano.getImage(imgNr).getFilename(), crop, l);
        GlobalCmdHist::getInstance().addCommand(
            new PT::AddNewLensToImagesCmd(pano, l, m_selectedImages)
            );
    } else {
        wxLogError(_("Please select an image and try again"));
    }
}

void LensPanel::OnChangeLens(wxCommandEvent & e)
{
    if (m_selectedImages.size() > 0) {
        // ask user for lens number.
        long nr = wxGetNumberFromUser(_("Enter new lens number"), _("Lens number"),
                                      _("Change lens number"), 0, 0,
                                      pano.getNrOfLenses()-1);
        if (nr >= 0) {
            // user accepted
            GlobalCmdHist::getInstance().addCommand(
                new PT::SetImageLensCmd(pano, m_selectedImages, nr)
                );
        }
    } else {
        wxLogError(_("Please select an image and try again"));
    }       
}



bool initLensFromFile(const std::string & filename, double &cropFactor, Lens & l)
{

    if (!l.initFromFile(filename, cropFactor)) {
        if (cropFactor == -1) {
            cropFactor = 1;
            wxConfigBase::Get()->Read(wxT("/LensDefaults/CropFactor"), &cropFactor);
            wxString tval;
            tval.Printf(wxT("%4.2f"),cropFactor);
            wxString t = wxGetTextFromUser(_("Enter Crop Factor for image\n\nThe crop factor is given by:\ncrop factor = 43.3 / diagonal \n\nwhere diagnal is the diagonal (in mm) of the film or imaging chip."),
                _("Adding Image"), tval);
            t.ToDouble(&cropFactor);
            wxConfigBase::Get()->Write(wxT("/LensDefaults/CropFactor"), cropFactor);

            return l.initFromFile(filename, cropFactor);
        }
        return false;
    }
    return true;
}
