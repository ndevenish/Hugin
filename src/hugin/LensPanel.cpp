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

#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/xrc/xmlres.h>          // XRC XML resouces
#include <wx/listctrl.h>
#include <wx/imaglist.h>
#include <wx/spinctrl.h>

#include "hugin/LensPanel.h"

#include "common/stl_utils.h"
#include "hugin/config.h"
#include "hugin/CommandHistory.h"
#include "hugin/ImageCache.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/ImagesList.h"
#include "hugin/ImageCenter.h"
#include "hugin/ImagesPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "PT/PanoCommand.h"
#include "PT/Panorama.h"

using namespace PT;
using namespace utils;
using namespace std;

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
    EVT_TEXT_ENTER ( XRCID("lens_val_a"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_b"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_c"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_d"), LensPanel::OnVarChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_e"), LensPanel::OnVarChanged )
    EVT_BUTTON ( XRCID("lens_button_center"), LensPanel::SetCenter )
    EVT_CHECKBOX ( XRCID("lens_inherit_v"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_a"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_b"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_c"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_d"), LensPanel::OnVarInheritChanged )
    EVT_CHECKBOX ( XRCID("lens_inherit_e"), LensPanel::OnVarInheritChanged )
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

    DEBUG_TRACE("");;
}


LensPanel::~LensPanel(void)
{
    DEBUG_TRACE("");
    pano.removeObserver(this);
    // not sure if this is needed.
//    delete images_list;
    DEBUG_TRACE("");
}

void LensPanel::FitParent( wxSizeEvent & e )
{
    wxSize new_size = GetSize();
//    wxSize new_size = GetParent()->GetSize();
    XRCCTRL(*this, "lens_panel", wxPanel)->SetSize ( new_size );
//    DEBUG_INFO( "" << new_size.GetWidth() <<"x"<< new_size.GetHeight()  );
}

void LensPanel::UpdateLensDisplay (unsigned int imgNr)
{
    DEBUG_TRACE("");

    m_editImageNr = imgNr;
    m_editLensNr = 0;

    const Lens & lens = pano.getLens(m_editLensNr);
    const VariableMap & imgvars = pano.getImageVariables(m_editImageNr);

    // FIXME should get a bottom to update
//    edit_Lens->readEXIF(pano.getImage(lensEdit_ReferenceImage).getFilename().c_str());

    // update gui
    XRCCTRL(*this, "lens_val_projectionFormat", wxComboBox)->SetSelection(
        lens.projectionFormat  );

    for (char** varname = Lens::variableNames; *varname != 0; ++varname) {
        // update parameters
        XRCCTRL(*this, wxString("lens_val_").append(*varname), wxTextCtrl)->SetValue(
            doubleToString(const_map_get(imgvars,*varname).getValue()).c_str());
        bool linked = const_map_get(lens.variables, *varname).isLinked();
        XRCCTRL(*this, wxString("lens_inherit_").append(*varname), wxCheckBox)->SetValue(linked);

    }

    for (char** varname = Lens::variableNames; *varname != 0; ++varname) {
        // update parameters
        XRCCTRL(*this, wxString("lens_val_").append(*varname), wxTextCtrl)->SetValue(
            doubleToString(const_map_get(imgvars,*varname).getValue()).c_str());
    }


    DEBUG_TRACE("");
}

void LensPanel::panoramaImagesChanged (PT::Panorama &pano, const PT::UIntSet & imgNr)
{
    // we need to do something if the image we are editing has changed.
    if ( pano.getNrOfImages() <= m_editImageNr) {
        // the image we were editing has been removed.
        m_editImageNr = 0;
    } else if (set_contains(imgNr, m_editImageNr)) {
        UpdateLensDisplay(m_editImageNr);
    }
}


// Here we change the pano.
void LensPanel::LensTypeChanged ( wxCommandEvent & e )
{
    DEBUG_TRACE ("")
    if (images_list->GetSelected().size() > 0) {
        // get lens from pano
        Lens lens = pano.getLens(m_editLensNr);
        // uses enum Lens::LensProjectionFormat from PanoramaMemento.h
        int var = XRCCTRL(*this, "lens_val_projectionFormat",
                          wxComboBox)->GetSelection();
        lens.projectionFormat = (Lens::LensProjectionFormat) (var);
        GlobalCmdHist::getInstance().addCommand(
            new PT::ChangeLensCmd( pano, m_editLensNr, lens )
            );
        DEBUG_INFO ("lens " << m_editLensNr << " Lenstype " << var);
    }
}

void LensPanel::focalLengthChanged ( wxCommandEvent & e )
{
    const UIntSet & selected = images_list->GetSelected();
    if (selected.size() > 0) {
        DEBUG_TRACE ("");
        double val;
        wxString text=XRCCTRL(*this,"lens_val_focalLength",wxTextCtrl)->GetValue();
        if (!text.ToDouble(&val)) {
            wxLogError(_("Value must be numeric."));
            return;
        }
        // this command is complicated and need the different lenses
        // conversion factors, FIXME change for multiple lenses
        int first = *(selected.begin());
        double factor = pano.getLens(pano.getImage(first).getLensNr()).focalLengthConversionFactor;
        double focalLength35mm = val * factor;
        double HFOV = 2.0 * atan((36/2) / focalLength35mm) * 180/M_PI;
        Variable var("v",HFOV);
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetVariableCmd( pano, selected, var)
            );
    }
    DEBUG_TRACE ("")
}


void LensPanel::OnVarChanged(wxCommandEvent & e)
{
    const UIntSet & selected = images_list->GetSelected();
    if (selected.size() > 0) {
        string varname;
        DEBUG_TRACE ("");
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
        } else if (e.m_id == XRCID("lens_val_v")) {
            varname = "v";
        } else {
            // not reachable
            DEBUG_ASSERT(0);
        }
        std::string ctrl_name("lens_val_");
        ctrl_name.append(varname);
        double val;
        wxString text = XRCCTRL(*this, ctrl_name.c_str(), wxTextCtrl)->GetValue();
        if (!text.ToDouble(&val)){
            wxLogError(_("Value must be numeric."));
            return;
        }
        Variable var(varname,val);
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetVariableCmd(pano, selected, var)
            );
    }
}

void LensPanel::OnVarInheritChanged(wxCommandEvent & e)
{
    const UIntSet & selected = images_list->GetSelected();
    if (selected.size() > 0) {
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
        } else if (e.m_id == XRCID("lens_inherit_v")) {
            varname = "v";
        } else {
            // not reachable
            DEBUG_ASSERT(0);
        }

        std::string ctrl_name("lens_inherit_");
        ctrl_name.append(varname);
        bool inherit = XRCCTRL(*this, ctrl_name.c_str(), wxCheckBox)->IsChecked();
        // get the current Lens.
        unsigned int lensNr = pano.getImage(*(selected.begin())).getLensNr();
        LensVariable lv = const_map_get(pano.getLens(lensNr).variables, varname);
        lv.setLinked(inherit);
        LensVarMap lmap;
        lmap.insert(make_pair(lv.getName(),lv));
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetLensVariableCmd(pano, lensNr, lmap)
            );
    }
}

void LensPanel::SetCenter ( wxCommandEvent & e )
{
    wxLogError("temporarily disabled");
#if 0
    const UIntSet & selected = images_list->GetSelected();
    if (selected.size() > 0) {
        unsigned int first = *(selected.begin());
        DEBUG_TRACE ("");
// temporarily disabled center panel
        ImgCenter *dlg = new ImgCenter((wxWindow*)this, wxDefaultPosition, wxDefaultSize, pano, &first);

        // show an image preview
        wxImage img = ImageCache::getInstance().getImage(
            pano.getImage(first).getFilename());
        dlg->ChangeView(img);
        dlg->CentreOnParent ();
        dlg->Refresh();
        dlg->Show();
    }
    DEBUG_TRACE ("")
#endif
}


void LensPanel::ListSelectionChanged(wxListEvent& e)
{
    DEBUG_TRACE(e.GetIndex());
    const UIntSet & sel = images_list->GetSelected();
    DEBUG_DEBUG("selected Images: " << sel.size());
    if (sel.size() == 0) {
        DEBUG_DEBUG("no selection, disabling value display");
        // clear & disable display
        XRCCTRL(*this, "lens_val_projectionFormat", wxComboBox)->Disable();
        XRCCTRL(*this, "lens_val_v", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_focalLength", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_a", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_b", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_c", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_d", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_val_e", wxTextCtrl)->Disable();
        XRCCTRL(*this, "lens_inherit_v", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_a", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_b", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_c", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_d", wxCheckBox)->Disable();
        XRCCTRL(*this, "lens_inherit_e", wxCheckBox)->Disable();
    } else {
        // one or more images selected
        if (XRCCTRL(*this, "lens_val_projectionFormat", wxComboBox)->Enable()) {
            // enable all other textboxes as well.
            XRCCTRL(*this, "lens_val_v", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_focalLength", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_a", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_b", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_c", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_d", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_val_e", wxTextCtrl)->Enable();
            XRCCTRL(*this, "lens_inherit_v", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_a", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_b", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_c", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_d", wxCheckBox)->Enable();
            XRCCTRL(*this, "lens_inherit_e", wxCheckBox)->Enable();
        }

        if (sel.size() == 1) {
            // single selection, its parameters
            // update values
            unsigned int img = *(sel.begin());
            DEBUG_DEBUG("updating LensPanel with Image " << img);
            UpdateLensDisplay(img);
        } else {
            XRCCTRL(*this, "lens_val_projectionFormat", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_v", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_focalLength", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_a", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_b", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_c", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_d", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_val_e", wxTextCtrl)->Clear();
            XRCCTRL(*this, "lens_inherit_v", wxCheckBox)->Clear();
            XRCCTRL(*this, "lens_inherit_a", wxCheckBox)->Clear();
            XRCCTRL(*this, "lens_inherit_b", wxCheckBox)->Clear();
            XRCCTRL(*this, "lens_inherit_c", wxCheckBox)->Clear();
            XRCCTRL(*this, "lens_inherit_d", wxCheckBox)->Clear();
            XRCCTRL(*this, "lens_inherit_e", wxCheckBox)->Clear();
        }
    }
}



