// -*- c-basic-offset: 4 -*-

/** @file ImagesPanel.cpp
 *
 *  @brief implementation of ImagesPanel Class
 *
 *  @author Kai-Uwe Behrmann <web@tiscali.de> and
 *          Pablo d'Angelo <pablo.dangelo@web.de>
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

#include "panoinc.h"
#include "panoinc_WX.h"

#include <map>

#include <wx/xrc/xmlres.h>          // XRC XML resouces
#include <wx/listctrl.h>    // needed on mingw
#include <wx/imaglist.h>
#include <wx/file.h>
#include <wx/spinctrl.h>

#include <vigra_ext/PointMatching.h>
#include <vigra_ext/LoweSIFT.h>

#include "hugin/ImagesPanel.h"
#include "hugin/config.h"
#include "hugin/CommandHistory.h"
#include "hugin/TextKillFocusHandler.h"
#include "hugin/ImageCache.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/ImagesList.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "hugin/ImageOrientationFrame.h"

using namespace PT;
using namespace utils;
using namespace vigra;
using namespace vigra_ext;
using namespace std;

ImgPreview * canvas;

//------------------------------------------------------------------------------
#define GET_VAR(val) pano.getVariable(orientationEdit_RefImg).val.getValue()

BEGIN_EVENT_TABLE(ImagesPanel, wxWindow)
    EVT_SIZE   ( ImagesPanel::FitParent )
//    EVT_MOUSE_EVENTS ( ImagesPanel::OnMouse )
//    EVT_MOTION ( ImagesPanel::ChangePreview )
    EVT_LIST_ITEM_SELECTED( XRCID("images_list_unknown"),
                            ImagesPanel::ListSelectionChanged )
    EVT_LIST_ITEM_SELECTED( XRCID("images_list_unknown"),
                            ImagesPanel::ListSelectionChanged )
    EVT_BUTTON     ( XRCID("images_opt_anchor_button"), ImagesPanel::OnOptAnchorChanged)
    EVT_BUTTON     ( XRCID("images_set_orientation_button"), ImagesPanel::OnSelectAnchorPosition)
    EVT_BUTTON     ( XRCID("images_color_anchor_button"), ImagesPanel::OnColorAnchorChanged)
    EVT_BUTTON     ( XRCID("images_feature_matching"), ImagesPanel::SIFTMatching)
    EVT_BUTTON     ( XRCID("images_remove_cp"), ImagesPanel::OnRemoveCtrlPoints)
    EVT_BUTTON     ( XRCID("images_reset_pos"), ImagesPanel::OnResetImagePositions)
    EVT_TEXT_ENTER ( XRCID("images_text_yaw"), ImagesPanel::OnYawTextChanged )
    EVT_TEXT_ENTER ( XRCID("images_text_pitch"), ImagesPanel::OnPitchTextChanged )
    EVT_TEXT_ENTER ( XRCID("images_text_roll"), ImagesPanel::OnRollTextChanged )
END_EVENT_TABLE()


// Define a constructor for the Images Panel
ImagesPanel::ImagesPanel(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama* pano)
    : wxPanel (parent, -1, wxDefaultPosition, wxDefaultSize, wxEXPAND|wxGROW),
      pano(*pano)
{
    DEBUG_TRACE("");

    wxXmlResource::Get()->LoadPanel (this, wxT("images_panel"));
    DEBUG_TRACE("");

    images_list = new ImagesListImage (parent, pano);
    wxXmlResource::Get()->AttachUnknownControl (
        "images_list_unknown",
        images_list );

    DEBUG_TRACE("");

    m_optAnchorButton = XRCCTRL(*this, "images_opt_anchor_button", wxButton);
    DEBUG_ASSERT(m_optAnchorButton);

    m_setAnchorOrientButton = XRCCTRL(*this, "images_set_orientation_button", wxButton);
    DEBUG_ASSERT(m_setAnchorOrientButton);
    m_colorAnchorButton = XRCCTRL(*this, "images_color_anchor_button", wxButton);
    DEBUG_ASSERT(m_colorAnchorButton);
    // Image Preview

    // converts KILL_FOCUS events to usable TEXT_ENTER events
    XRCCTRL(*this, "images_text_yaw", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "images_text_roll", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "images_text_pitch", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));

    m_empty.LoadFile(MainFrame::Get()->GetXRCPath() +
                     "/data/" "druid.images.128.png",
                     wxBITMAP_TYPE_PNG);
    XRCCTRL(*this, "images_selected_image", wxStaticBitmap)->
        SetBitmap(m_empty);

    wxListEvent ev;
    ListSelectionChanged(ev);
    pano->addObserver(this);
    DEBUG_TRACE("end");
}


ImagesPanel::~ImagesPanel(void)
{
    DEBUG_TRACE("dtor");

    // FIXME crashes.. don't know why
/*
    XRCCTRL(*this, "images_text_yaw", wxTextCtrl)->PopEventHandler();
    XRCCTRL(*this, "images_text_roll", wxTextCtrl)->PopEventHandler();
    XRCCTRL(*this, "images_text_pitch", wxTextCtrl)->PopEventHandler();
    delete(m_tkf);
*/
    pano.removeObserver(this);
    delete images_list;
    DEBUG_TRACE("dtor end");
}

void ImagesPanel::FitParent( wxSizeEvent & e )
{
    wxSize new_size = GetSize();
    XRCCTRL(*this, "images_panel", wxPanel)->SetSize ( new_size );
//    DEBUG_INFO( "" << new_size.GetWidth() <<"x"<< new_size.GetHeight()  );
}


void ImagesPanel::panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & _imgNr)
{
    DEBUG_TRACE("");

    // update text field if selected
    const UIntSet & selected = images_list->GetSelected();
    DEBUG_DEBUG("nr of sel Images: " << selected.size());
    if (selected.size() == 1 &&
        *(selected.begin()) < pano.getNrOfImages() &&
        set_contains(_imgNr, *(selected.begin())))
    {
        DEBUG_DEBUG("Img Nr: " << *(selected.begin()));
        ShowImgParameters( *(selected.begin()) );
    }
    else
    {
        ClearImgParameters( );
    }

    DEBUG_TRACE("");
}

void ImagesPanel::ChangePano ( std::string type, double var )
{
    Variable img_var(type,var);
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetVariableCmd(pano, images_list->GetSelected(), img_var)
        );
}


// #####  Here start the eventhandlers  #####

/** run sift matching on selected images, and add control points */
void ImagesPanel::SIFTMatching(wxCommandEvent & e)
{
    const UIntSet & selImg = images_list->GetSelected();
    if ( selImg.size() < 2 ) {
        wxMessageBox(_("At least 2 images must be selected"),
                     _("Error"), wxCANCEL | wxICON_ERROR);
        return;
    }

    long nFeatures = XRCCTRL(*this, "images_points_per_overlap"
                            , wxSpinCtrl)->GetValue();

#ifdef __WXMSW__
    wxString autopanoExe = wxConfigBase::Get()->Read("/PanoTools/AutopanoExe","autopano.exe");
    if (!wxFile::Exists(autopanoExe)){
        wxFileDialog dlg(this,_("Select autopano.exe (>= V 1.03"),
                         "", "autopano.exe",
                         "Executables (*.exe)|*.exe",
                         wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            autopanoExe = dlg.GetPath();
            wxConfigBase::Get()->Write("/PanoTools/AutopanoExe",autopanoExe);
        } else {
            wxLogError(_("No autopano.exe selected (download it from http://autopano.kolor.com)"));
            return;
        }
    }
#else
    wxString autopanoExe = wxConfigBase::Get()->Read("/PanoTools/AutopanoExe","autopano");
#endif


    // build a list of all image files, and a corrosponding connection map.
    // local img nr -> global (panorama) img number
    std::map<int,int> imgMapping;
    int imgNr=0;
    for(UIntSet::const_iterator it = selImg.begin(); it != selImg.end(); it++)
    {
        imgMapping[imgNr] = *it;
        autopanoExe.append(" ").append(pano.getImage(*it).getFilename().c_str());
        imgNr++;
    }
    wxString autopanoArgs = wxConfigBase::Get()->Read("/PanoTools/AutopanoArgs","-nomove  -search:1 -size:1024 -ransac:1  -noclean -hugin -keys:");

    wxString cmd;
    cmd.Printf("%s %s%ld",autopanoExe.c_str(), autopanoArgs.c_str(), nFeatures);
    DEBUG_DEBUG("Executing: " << cmd.c_str());
    // run autopano in an own output window
    wxShell(cmd);

    // parse resulting output file
    ifstream stream("pano0/pano0.pto");
    if (! stream.is_open()) {
        DEBUG_ERROR("Could not open autopano output: pano0/pano0.pto");
        return;
    }

    CPVector ctrlPoints;
    string line;
    while(!stream.eof()) {
        std::getline(stream, line);

        if (line.size() > 0 && line[0] == 'c') {
            int t;
            ControlPoint point;
            getParam(point.image1Nr, line, "n");
            point.image1Nr = imgMapping[point.image1Nr];
            getParam(point.image2Nr, line, "N");
            point.image2Nr = imgMapping[point.image2Nr];
            getParam(point.x1, line, "x");
            getParam(point.x2, line, "X");
            getParam(point.y1, line, "y");
            getParam(point.y2, line, "Y");
            getParam(t, line, "t");
            point.mode = (ControlPoint::OptimizeMode) t;
            ctrlPoints.push_back(point);
        } else {
            DEBUG_DEBUG("skipping line: " << line);
        }
    }

    GlobalCmdHist::getInstance().addCommand(
        new PT::AddCtrlPointsCmd(pano, ctrlPoints)
        );
}

#if 0
/** run sift matching (using builtin matcher) on selected images, and
    add control points */
void ImagesPanel::SIFTMatchingBuiltin(wxCommandEvent & e)
{

#if 0
    wxString text = XRCCTRL(*this, "images_points_per_overlap"
                            , wxTextCtrl) ->GetValue();
    long nFeatures = 10;
    if (!text.ToLong(&nFeatures)) {
        wxLogError(_("Value must be numeric."));
        return;
    }
    // pyramid level to use
    int pyrLevel = 2;
    DEBUG_DEBUG("SIFTMatching, on pyramid level " << pyrLevel << " desired features per overlap: " << nFeatures);
    const UIntSet & selImg = images_list->GetSelected();
    if ( selImg.size() > 0 ) {
        // do a stupid, match every Image with every image.

        // first, calculate all sift features.
        std::vector<std::vector<SIFTFeature> > ftable(selImg.size());

        DEBUG_DEBUG("starting keypoint detection (with Dr. Lowes keypoint program)");
        DEBUG_DEBUG("This will take time, please be patient");
        int i=0;
        for(UIntSet::const_iterator it = selImg.begin();
            it != selImg.end(); ++it, ++i)
        {
            std::string fname = pano.getImage(*it).getFilename();
            // get a small image, to keep SIFT detector time low
            const vigra::BImage & img = ImageCache::getInstance().getPyramidImage(fname, pyrLevel);
            loweDetectSIFT(srcImageRange(img), ftable[i]);
        }
        DEBUG_DEBUG("Feature points detected, starting matching");

        // the accumulated Control Points.
        CPVector newPoints;

        // match all images...
        // slow...
        int i1=0;
        for (UIntSet::const_iterator it1 = selImg.begin();
             it1 != selImg.end(); ++it1, ++i1)
        {
            UIntSet::const_iterator tmpit(it1);
            tmpit++;
            int i2=i1+1;
            for (UIntSet::const_iterator it2(tmpit);
                 it2 != selImg.end(); ++it2, ++i2)
            {
                vector<triple<int,SIFTFeature, SIFTFeature> > matches;
                // resulting features
                if (MatchImageFeatures(ftable[i1],
                                       ftable[i2],
                                       matches))
                {
                    DEBUG_NOTICE("Found " << matches.size() << " points between image " << *it1 << " and " << *it2);

                    // sort matches by distance..
                    sort(matches.begin(), matches.end(),matchDistance());
                    // we found matches! add control points.
                    // copy once more...
                    for (vector<triple<int, SIFTFeature, SIFTFeature> >::iterator mit = matches.begin();
                         mit != matches.end() && mit - matches.begin() < nFeatures;
                         ++mit)
                    {
                        // factor to undo pyramid level
                        double f = 1 << pyrLevel;
                        newPoints.push_back(ControlPoint(*it1,
                                                         mit->second.pos.x * f,
                                                         mit->second.pos.y * f,
                                                         *it2,
                                                         mit->third.pos.x * f,
                                                         mit->third.pos.y * f)
                            );
                    }
                } else {
                    DEBUG_NOTICE("No matches between image " << *it1 << " and " << *it2);
                }
            }
        }
        if (newPoints.size() != 0) {
            // add all control points
            GlobalCmdHist::getInstance().addCommand(
                new PT::AddCtrlPointsCmd (pano, newPoints)
                );
        }
    }
#endif
}

#endif

// Yaw by text -> double
void ImagesPanel::OnYawTextChanged ( wxCommandEvent & e )
{
    if ( images_list->GetSelected().size() > 0 ) {
        wxString text = XRCCTRL(*this, "images_text_yaw"
                                , wxTextCtrl) ->GetValue();
        if (text == "") {
            return;
        }
        DEBUG_INFO ("yaw = " << text );

        double val;
        if (!text.ToDouble(&val)) {
            wxLogError(_("Value must be numeric."));
            return;
        }
        ChangePano ( "y" , val );

    }
}

void ImagesPanel::OnPitchTextChanged ( wxCommandEvent & e )
{
    if ( images_list->GetSelected().size() > 0 ) {
        wxString text = XRCCTRL(*this, "images_text_pitch"
                                , wxTextCtrl) ->GetValue();
        DEBUG_INFO ("pitch = " << text );
        if (text == "") {
            return;
        }

        double val;
        if (!text.ToDouble(&val)) {
            wxLogError(_("Value must be numeric."));
            return;
        }
        ChangePano ( "p" , val );
    }
}

void ImagesPanel::OnRollTextChanged ( wxCommandEvent & e )
{
    if ( images_list->GetSelected().size() > 0 ) {
        wxString text = XRCCTRL(*this, "images_text_roll"
                                , wxTextCtrl) ->GetValue();
        DEBUG_INFO ("roll = " << text );
        if (text == "") {
            return;
        }

        double val;
        if (!text.ToDouble(&val)) {
            wxLogError(_("Value must be numeric."));
            return;
        }
        ChangePano ( "r" , val );
    }
}

void ImagesPanel::OnOptAnchorChanged(wxCommandEvent &e )
{
    const UIntSet & sel = images_list->GetSelected();
    if ( sel.size() == 1 ) {
        // set first image to be the anchor
        PanoramaOptions opt = pano.getOptions();
        opt.optimizeReferenceImage = *(sel.begin());

        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( pano, opt )
            );
    }
}

void ImagesPanel::OnSelectAnchorPosition(wxCommandEvent & e)
{
    // first, change anchor
    OnOptAnchorChanged(e);

    // open a frame to show the image
    ImageOrientationFrame * t = new ImageOrientationFrame(this, pano);
    t->Show();
}

void ImagesPanel::OnColorAnchorChanged(wxCommandEvent &e )
{
    const UIntSet & sel = images_list->GetSelected();
    if ( sel.size() == 1 ) {
        // set first image to be the anchor
        PanoramaOptions opt = pano.getOptions();
        opt.colorReferenceImage = *(sel.begin());

        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( pano, opt )
            );
    }
}


void ImagesPanel::ListSelectionChanged(wxListEvent & e)
{
    DEBUG_TRACE(e.GetIndex());
    const UIntSet & sel = images_list->GetSelected();
    DEBUG_DEBUG("selected Images: " << sel.size());
    if (sel.size() > 0) {
        DEBUG_DEBUG("first sel. image: " << *(sel.begin()));
    }
    if (sel.size() == 0) {
        // nothing to edit
        DisableImageCtrls();
    } else {
        // enable edit
        EnableImageCtrls();
        if (sel.size() == 1) {
            unsigned int imgNr = *(sel.begin());
            // single selection, show its parameters
            ShowImgParameters(imgNr);
            m_optAnchorButton->Enable();
            m_setAnchorOrientButton->Enable();
            m_colorAnchorButton->Enable();
        } else {
            DEBUG_DEBUG("Multiselection");
            // multiselection, clear all values
            // we don't know which images parameters to show.
            ClearImgParameters();
            m_optAnchorButton->Disable();
            m_setAnchorOrientButton->Disable();
            m_colorAnchorButton->Disable();
        }
    }
}


// ######  Here end the eventhandlers  #####

void ImagesPanel::DisableImageCtrls()
{
    // disable controls
    XRCCTRL(*this, "images_text_yaw", wxTextCtrl) ->Disable();
    XRCCTRL(*this, "images_text_roll", wxTextCtrl) ->Disable();
    XRCCTRL(*this, "images_text_pitch", wxTextCtrl) ->Disable();
    XRCCTRL(*this, "images_selected_image", wxStaticBitmap)->
        SetBitmap(m_empty);
    m_optAnchorButton->Disable();
    m_colorAnchorButton->Disable();
    m_setAnchorOrientButton->Disable();
}

void ImagesPanel::EnableImageCtrls()
{
    // enable control if not already enabled
    if (XRCCTRL(*this, "images_text_yaw", wxTextCtrl)->Enable()) {
        XRCCTRL(*this, "images_text_roll", wxTextCtrl) ->Enable();
        XRCCTRL(*this, "images_text_pitch", wxTextCtrl) ->Enable();
    }
}

void ImagesPanel::ShowImgParameters(unsigned int imgNr)
{
    const VariableMap & vars = pano.getImageVariables(imgNr);

    std::string val;
    val = doubleToString(const_map_get(vars,"y").getValue());
    XRCCTRL(*this, "images_text_yaw", wxTextCtrl) ->SetValue(val.c_str());

    val = doubleToString(const_map_get(vars,"p").getValue());
    XRCCTRL(*this, "images_text_pitch", wxTextCtrl) ->SetValue(val.c_str());

    val = doubleToString(const_map_get(vars,"r").getValue());
    XRCCTRL(*this, "images_text_roll", wxTextCtrl) ->SetValue(val.c_str());

    ShowImage(imgNr);
}

void ImagesPanel::ClearImgParameters()
{
    XRCCTRL(*this, "images_text_yaw", wxTextCtrl) ->Clear();
    XRCCTRL(*this, "images_text_roll", wxTextCtrl) ->Clear();
    XRCCTRL(*this, "images_text_pitch", wxTextCtrl) ->Clear();

    XRCCTRL(*this, "images_selected_image", wxStaticBitmap)->
        SetBitmap(m_empty);
}

void ImagesPanel::ShowImage(unsigned int imgNr)
{
    // show preview image
    wxStaticBitmap * imgctrl = XRCCTRL(*this, "images_selected_image", wxStaticBitmap);
    DEBUG_ASSERT(imgctrl);
    wxSize sz = imgctrl->GetSize();
    const wxImage * img = ImageCache::getInstance().getSmallImage(
        pano.getImage(imgNr).getFilename());

    double sRatio = (double)sz.GetWidth() / sz.GetHeight();
    double iRatio = (double)img->GetWidth() / img->GetHeight();

    int w,h;
    if (iRatio > sRatio) {
        // image is wider than screen, display landscape
        w = sz.GetWidth();
        h = (int) (w / iRatio);
    } else {
        // portrait
        h = sz.GetHeight();
        w = (int) (h * iRatio);
    }
    wxImage scaled = img->Scale(w,h);
    imgctrl->SetBitmap(wxBitmap(scaled));
}


void ImagesPanel::OnResetImagePositions(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    const UIntSet & selImg = images_list->GetSelected();
    unsigned int nSelImg =  selImg.size();
    if ( nSelImg > 0 ) {
        VariableMapVector vars(nSelImg);
        unsigned int i=0;
        for (UIntSet::const_iterator it = selImg.begin();
             it != selImg.end(); ++it )
        {
            vars[i].insert(make_pair("y", Variable("y",0.0)));
            vars[i].insert(make_pair("p", Variable("p",0.0)));
            vars[i].insert(make_pair("r", Variable("r",0.0)));
            i++;
        }
        GlobalCmdHist::getInstance().addCommand(
            new PT::UpdateImagesVariablesCmd( pano, selImg, vars ));
    }

}


void ImagesPanel::OnRemoveCtrlPoints(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    const UIntSet & selImg = images_list->GetSelected();
    unsigned int nSelImg =  selImg.size();
    if ( nSelImg > 1 ) {
        UIntSet cpsToDelete;
        const CPVector & cps = pano.getCtrlPoints();
        for (CPVector::const_iterator it = cps.begin(); it != cps.end(); ++it){
            if (set_contains(selImg, (*it).image1Nr) &&
                set_contains(selImg, (*it).image2Nr) )
            {
                cpsToDelete.insert(it - cps.begin());
            }
        }
        GlobalCmdHist::getInstance().addCommand(
            new PT::RemoveCtrlPointsCmd( pano, cpsToDelete ));
    } else {
        wxLogError(_("Select at least two images"));
    }
}

