// -*- c-basic-offset: 4 -*-

/** @file AssistantPanel.cpp
 *
 *  @brief implementation of AssistantPanel Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
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

#include "common/stl_utils.h"

#include <map>

//#include <vigra_ext/PointMatching.h>
//#include <vigra_ext/LoweSIFT.h>

#include "PT/PTOptimise.h"
#include "common/wxPlatform.h"
#include "hugin/AssistantPanel.h"
#include "hugin/CommandHistory.h"
#include "hugin/ImageCache.h"
#include "hugin/ImagesList.h"
#include "hugin/LensPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "hugin/AutoCtrlPointCreator.h"
#include "hugin/PTWXDlg.h"
#include "hugin/TextKillFocusHandler.h"
#include "hugin/PanoDruid.h"
#include "hugin/config_defaults.h"

using namespace PT;
using namespace PTools;
using namespace utils;
using namespace vigra;
using namespace vigra_ext;
using namespace std;


//------------------------------------------------------------------------------
// utility function
static wxString Components2Str(const CPComponents & comp)
{
    wxString ret;
    for (unsigned i=0; i < comp.size(); i++) {
        ret = ret + wxT("[");
        CPComponents::value_type::const_iterator it;
        int c=0;
        for (it = comp[i].begin();
            it != comp[i].end();
            ++it) 
        {
            ret = ret + wxString::Format(wxT("%d"), (*it));
            if (c+1 != comp[i].size()) {
                ret = ret + wxT(", ");
            }
            c++;
        }
        if (i+1 != comp.size())
            ret = ret + wxT("], ");
        else
            ret = ret + wxT("]");
    }
    return ret;
}
//------------------------------------------------------------------------------



BEGIN_EVENT_TABLE(AssistantPanel, wxWindow)
    EVT_SIZE   ( AssistantPanel::OnSize )
    EVT_CHECKBOX   ( XRCID("ass_exif_cb"),          AssistantPanel::OnExifToggle)
    EVT_CHOICE     ( XRCID("ass_lens_proj_choice"), AssistantPanel::OnLensTypeChanged)
    EVT_TEXT_ENTER ( XRCID("ass_focallength_text"), AssistantPanel::OnFocalLengthChanged)
    EVT_TEXT_ENTER ( XRCID("ass_cropfactor_text"),  AssistantPanel::OnCropFactorChanged)
    EVT_BUTTON     ( XRCID("ass_load_lens_button"), AssistantPanel::OnLoadLens)
    EVT_BUTTON     ( XRCID("ass_load_images_button"), AssistantPanel::OnLoadImages)
    EVT_BUTTON     ( XRCID("ass_align_button"),     AssistantPanel::OnAlign)
    EVT_BUTTON     ( XRCID("ass_create_button"),    AssistantPanel::OnCreate)
END_EVENT_TABLE()


// Define a constructor for the Assistant Panel
AssistantPanel::AssistantPanel(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama* pano)
    : wxPanel (parent, -1, wxDefaultPosition, wxDefaultSize, wxEXPAND|wxGROW),
      m_pano(*pano), m_restoreLayoutOnResize(false), m_noImage(true)
{
    DEBUG_TRACE("");

    wxXmlResource::Get()->LoadPanel (this, wxT("assistant_panel"));

    m_imagesText = XRCCTRL(*this, "ass_load_images_text", wxStaticText);
    DEBUG_ASSERT(m_imagesText);

    m_exifToggle = XRCCTRL(*this, "ass_exif_cb", wxCheckBox);
    DEBUG_ASSERT(m_exifToggle);

    m_lensTypeChoice = XRCCTRL(*this, "ass_lens_proj_choice", wxChoice);
    DEBUG_ASSERT(m_lensTypeChoice);
    m_lensTypeChoice->SetSelection(0);

    m_focalLengthText = XRCCTRL(*this, "ass_focallength_text", wxTextCtrl);
    DEBUG_ASSERT(m_focalLengthText);
    m_focalLengthText->PushEventHandler(new TextKillFocusHandler(this));

    m_cropFactorText = XRCCTRL(*this, "ass_cropfactor_text", wxTextCtrl);
    DEBUG_ASSERT(m_cropFactorText);
    m_cropFactorText->PushEventHandler(new TextKillFocusHandler(this));

    m_alignButton = XRCCTRL(*this, "ass_align_button", wxButton);
    DEBUG_ASSERT(m_alignButton);
    m_alignButton->Disable();

    m_alignText = XRCCTRL(*this, "ass_align_text", wxStaticText);
    DEBUG_ASSERT(m_alignText);

    m_createButton = XRCCTRL(*this, "ass_create_button", wxButton);
    DEBUG_ASSERT(m_createButton);
    m_createButton->Disable();

    // druid is currently disabled
    m_druid = 0;
            /*
    m_druid = new PanoDruid(this);
    wxXmlResource::Get()->AttachUnknownControl (wxT("ass_druid"), m_druid );
    m_druid->Update(m_pano);
            */

#ifdef USE_WX253
    m_panel = XRCCTRL(*this, "ass_control_panel", wxScrolledWindow);
    DEBUG_ASSERT(m_panel);

    m_panel->FitInside();
    m_panel->SetScrollRate(10, 10);
#endif


#ifdef USE_WX253
    SetAutoLayout(false);
#endif

    m_degDigits = 2;

    // observe the panorama
    m_pano.addObserver(this);

}


AssistantPanel::~AssistantPanel(void)
{
    DEBUG_TRACE("dtor");
    m_focalLengthText->PopEventHandler(true);
    m_cropFactorText->PopEventHandler(true);
    m_pano.removeObserver(this);
    DEBUG_TRACE("dtor end");
}

void AssistantPanel::RestoreLayout()
{
	DEBUG_TRACE("");
#ifdef USE_WX253
    int winWidth, winHeight;
    GetClientSize(&winWidth, &winHeight);
    DEBUG_INFO( "window size: " << winWidth <<"x"<< winHeight);
#endif

}

// We need to override the default handling of size events because the
// sizers set the virtual size but not the actual size. We reverse
// the standard handling and fit the child to the parent rather than
// fitting the parent around the child

void AssistantPanel::OnSize( wxSizeEvent & e )
{
    wxSize new_size = GetSize();
    XRCCTRL(*this, "assistant_panel", wxPanel)->SetSize ( new_size );
    DEBUG_INFO( "assistant panel: " << new_size.GetWidth() <<"x"<< new_size.GetHeight()  );

    if (m_restoreLayoutOnResize) {
        m_restoreLayoutOnResize = false;
        RestoreLayout();
    }

    e.Skip();
}

void AssistantPanel::panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & _imgNr)
{

}

void AssistantPanel::panoramaChanged(PT::Panorama &pano)
{
    DEBUG_TRACE("");

    if (m_druid) m_druid->Update(m_pano);

    m_alignButton->Enable(pano.getNrOfImages() > 1);

    if (pano.getNrOfImages() == 0) {
        m_createButton->Disable();
        m_imagesText->SetLabel(_("Please load images by pressing on the Load images button."));
        m_exifToggle->Disable();
        XRCCTRL(*this, "ass_lens_group", wxPanel)->Disable();
        m_noImage = true;
    } else {
        int images = pano.getNrOfImages();
        bool enableCreate = false;;
        if (images > 1) {
            while (images)
            {
                --images;
                const VariableMap & vars = pano.getImageVariables(images);
                if (const_map_get(vars,"y").getValue() != 0.0) {
                    enableCreate = true;
                    break;
                }
                if (const_map_get(vars,"p").getValue() != 0.0) {
                    enableCreate = true;
                    break;
                }
                if (const_map_get(vars,"r").getValue() != 0.0) {
                    enableCreate = true;
                    break;
                }
            }
        }

        images = pano.getNrOfImages();
        m_createButton->Enable(enableCreate);

        wxString imgMsg = wxString::Format(_("%d images loaded."), pano.getNrOfImages());

        const Lens & lens = pano.getLens(0);

        /*
        if (!lens.m_hasExif) {
            imgMsg = imgMsg + wxT("\n") + _("No EXIF data found. Please enter focal length.");
        }
        */

        if (m_noImage) {
            // straight after loading the first image, set exif checkbox, if available
            if (lens.m_hasExif) {
                m_exifToggle->Enable();
                m_exifToggle->SetValue(true);
            } else {
                m_exifToggle->Disable();
                m_exifToggle->SetValue(false);
                XRCCTRL(*this, "ass_lens_group", wxPanel)->Enable();
            }
        }
        m_noImage = false;

        // update data in lens display
        m_lensTypeChoice->SetSelection(lens.getProjection());
        double focal_length = lens.getFocalLength();
        m_focalLengthText->SetValue(doubleTowxString(focal_length,m_degDigits));
        double focal_length_factor = lens.getCropFactor();
        m_cropFactorText->SetValue(doubleTowxString(focal_length_factor,m_degDigits));

        m_imagesText->SetLabel(imgMsg);
    }

    if (pano.getNrOfImages() > 1) {
        wxString alignMsg = wxString::Format(_("Images are connected by %d control points.\n"), pano.getCtrlPoints().size());

        if (m_pano.getNrOfCtrlPoints() > 0) {
            // find components..
            CPGraph graph;
            createCPGraph(m_pano, graph);
            CPComponents comps;
            int n= findCPComponents(graph, comps);
            if (n > 1) {
                alignMsg += wxString::Format(_("%d unconnected image groups found: "), n) + Components2Str(comps) + wxT("\n");
                alignMsg += _("Please use the Control Points tab to connect all images with control points.\n");
            } else {
                if (m_pano.needsOptimization()) {
                    alignMsg += _("Images or control points have changed, new alignment is needed.");
                } else {
                    double min;
                    double max;
                    double mean;
                    double var;
                    m_pano.calcCtrlPntsErrorStats( min, max, mean, var);

                    if (max != 0.0) {
                        wxString distStr;
                        if (mean < 1)
                            distStr = _("Very good fit.");
                        else if (mean < 3)
                            distStr = _("Good fit.");
                        else if (mean < 7)
                            distStr = _("Bad fit, some control points might be bad, or there are parallax and movement errors");
                        else
                            distStr = _("Very bad fit. Check for bad control points, lens parameters, or images with parallax or movement. The optimizer might have failed. Manual intervention required.");

                        alignMsg = alignMsg + wxString::Format(_("Mean error after optimization: %.1f pixel, max: %.1f\n"), mean, max)
                                + distStr; 
                    }
                }
            }
        }

        // need to resize the text widget somehow!
        m_alignText->SetLabel(alignMsg);
    } else {
        m_alignText->SetLabel(wxT(""));
    }
    // TODO: update meaningful help text and dynamic links to relevant tabs
}

// #####  Here start the eventhandlers  #####

void AssistantPanel::OnLoadImages( wxCommandEvent & e )
{
    // load the images.
    wxCommandEvent dummy;
    MainFrame::Get()->OnAddImages(dummy);

    long autoAlign = wxConfigBase::Get()->Read(wxT("/Assistant/autoAlign"), HUGIN_ASS_AUTO_ALIGN); 

    if (autoAlign) {
        OnAlign(dummy);
    }

}

void AssistantPanel::OnAlign( wxCommandEvent & e )
{
    // create control points
    // all images..
    UIntSet imgs;
    if (m_pano.getNrOfImages() < 2) {
        wxMessageBox(_("At least two images are required."),_("Error"));
        return;
    }

    fill_set(imgs, 0, m_pano.getNrOfImages()-1);

    long nFeatures = wxConfigBase::Get()->Read(wxT("/Assistant/nControlPoints"), HUGIN_ASS_NCONTROLPOINTS); 

    /*
    bool createCtrlP = true;
    // TODO: handle existing control points properly instead of adding them twice.
    if (m_pano.getNrOfCtrlPoints() > 0) {
        int a = wxMessageBox(wxString::Format(_("The panorama already has %d control points.\n\nSkip control points creation?"), m_pano.getNrOfCtrlPoints()),
                     _("Skip control point creation?"), wxICON_QUESTION | wxYES_NO);
        createCtrlP = a != wxYES;
    }
    */

    bool createCtrlP = m_pano.getNrOfCtrlPoints() == 0;

    wxString alignMsg;
    if (createCtrlP) {
        AutoCtrlPointCreator matcher;
        CPVector cps = matcher.automatch(m_pano, imgs, nFeatures);
        GlobalCmdHist::getInstance().addCommand(
                new PT::AddCtrlPointsCmd(m_pano, cps)
                                               );
    }

    // find components..
    CPGraph graph;
    createCPGraph(m_pano, graph);
    CPComponents comps;
    int n = findCPComponents(graph, comps);

    if (n > 1) {
        // switch to images panel.
        unsigned i1 = *(comps[0].rbegin());
        unsigned i2 = *(comps[1].begin());
        MainFrame::Get()->ShowCtrlPointEditor( i1, i2);
        // display message box with 
        
        wxMessageBox(wxString::Format(_("Warning %d unconnected image groups found:"), n) + Components2Str(comps) + wxT("\n")
                     + _("Please create control points between unconnected images using the Control Points tab.\n\nAfter adding the points, press the \"Align\" button again"));
        return;
    }

    // optimize panorama

    Panorama optPano = m_pano.getSubset(imgs);

    // set TIFF_m with enblend
    PanoramaOptions opts = m_pano.getOptions();
    opts.outputFormat = PanoramaOptions::TIFF;
    opts.blendMode = PanoramaOptions::ENBLEND_BLEND;
    opts.remapper = PanoramaOptions::NONA;
    opts.tiff_saveROI = true;
    opts.tiffCompression = "DEFLATE";
    opts.setProjection(PanoramaOptions::EQUIRECTANGULAR);

    // calculate proper scaling, 1:1 resolution.
    // Otherwise optimizer distances are meaningless.
    opts.setWidth(30000, false);
    opts.setHeight(15000);

    optPano.setOptions(opts);
    int w = optPano.calcOptimalWidth();
    opts.setWidth(w);
    opts.setHeight(w/2);
    optPano.setOptions(opts);

    {
        wxBusyCursor bc;
        // temporarily disable PT progress dialog..
        deregisterPTWXDlgFcn();
        smartOptimize(optPano);
        registerPTWXDlgFcn();
    }

    // straighten
    optPano.straighten();

    // center and resize frame
    optPano.centerHorizontically();
    opts = optPano.getOptions();
    double hfov, vfov, height;
    optPano.fitPano(hfov, height);
    opts.setHFOV(hfov);
    opts.setHeight(roundi(height));
    vfov = opts.getVFOV();

    double mf = HUGIN_ASS_MAX_NORMAL_FOV;
    wxConfigBase::Get()->Read(wxT("/Assistant/maxNormalFOV"), &mf, HUGIN_ASS_MAX_NORMAL_FOV);
    // choose proper projection type
    if (vfov < mf) {
        // cylindrical or rectilinear
        if (hfov < mf) {
            opts.setProjection(PanoramaOptions::RECTILINEAR);
        } else {
            opts.setProjection(PanoramaOptions::CYLINDRICAL);
        }
    }

    double sizeFactor = HUGIN_ASS_PANO_DOWNSIZE_FACTOR;
    wxConfigBase::Get()->Read(wxT("/Assistant/panoDownsizeFactor"), &sizeFactor, HUGIN_ASS_PANO_DOWNSIZE_FACTOR);
    // calc optimal size using output projection
    // reduce optimal size a little
    optPano.setOptions(opts);
    w = optPano.calcOptimalWidth();
    opts.setWidth(roundi(w*sizeFactor), true);
    optPano.setOptions(opts);

    // TODO: merge the following commands.

    // copy information into the main panorama
    GlobalCmdHist::getInstance().addCommand(
            new PT::UpdateVariablesCPSetCmd(m_pano, imgs, optPano.getVariables(), optPano.getCtrlPoints())
         );

    // copy information into our panorama
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd(m_pano, opts)
         );

    // show preview frame
    wxCommandEvent dummy;
    MainFrame::Get()->OnTogglePreviewFrame(dummy);

    // enable stitch button
    m_createButton->Enable();
}

void AssistantPanel::OnCreate( wxCommandEvent & e )
{
    // just run the stitcher
    // this is kind of a bad hack, since several settings are determined
    // based on the current state of PanoPanel, and not the Panorama object itself

    // calc optimal size using output projection
    double sizeFactor = HUGIN_ASS_PANO_DOWNSIZE_FACTOR;
    wxConfigBase::Get()->Read(wxT("/Assistant/panoDownsizeFactor"), &sizeFactor, HUGIN_ASS_PANO_DOWNSIZE_FACTOR);
    PanoramaOptions opts = m_pano.getOptions();
    int w = m_pano.calcOptimalWidth();
    // check if resize was plausible!
    if (w> 0) {
        opts.setWidth(floori(w*sizeFactor), true);
        // copy information into our panorama
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd(m_pano, opts)
            );
    }

    wxCommandEvent dummy;
    MainFrame::Get()->OnDoStitch(dummy);
}

void AssistantPanel::OnLoadLens(wxCommandEvent & e)
{
    unsigned int imgNr = 0;
    unsigned int lensNr = m_pano.getImage(imgNr).getLensNr();
    Lens lens = m_pano.getLens(lensNr);
    VariableMap vars = m_pano.getImageVariables(imgNr);
    ImageOptions imgopts = m_pano.getImage(imgNr).getOptions();

    if (LoadLensParametersChoose(lens, vars, imgopts)) {
        GlobalCmdHist::getInstance().addCommand(
                new PT::ChangeLensCmd(m_pano, lensNr, lens)
                                               );
        GlobalCmdHist::getInstance().addCommand(
                new PT::UpdateImageVariablesCmd(m_pano, imgNr, vars)
                                               );
                // get all images with the current lens.
        UIntSet imgs;
        for (unsigned int i = 0; i < m_pano.getNrOfImages(); i++) {
            if (m_pano.getImage(i).getLensNr() == lensNr) {
                imgs.insert(i);
            }
        }

        // set image options.
        GlobalCmdHist::getInstance().addCommand(
                new PT::SetImageOptionsCmd(m_pano, imgopts, imgs) );
    }

}

void AssistantPanel::OnExifToggle (wxCommandEvent & e)
{
    if (m_exifToggle->GetValue()) {
        unsigned int imgNr = 0;
        // if activated, load exif info
        double cropFactor = 0;
        double focalLength = 0;
        SrcPanoImage srcImg = m_pano.getSrcImage(imgNr);
        bool ok = initImageFromFile(srcImg, focalLength, cropFactor);
        if (! ok) {
            getLensDataFromUser(this, srcImg, focalLength, cropFactor);
        }
                //initLensFromFile(pano.getImage(imgNr).getFilename().c_str(), c, lens, vars, imgopts, true);
        GlobalCmdHist::getInstance().addCommand(
                new PT::UpdateSrcImageCmd( m_pano, imgNr, srcImg)
                                               );
        XRCCTRL(*this, "ass_lens_group", wxPanel)->Disable();
    } else {
        // exif disabled
        XRCCTRL(*this, "ass_lens_group", wxPanel)->Enable();
    }
}

void AssistantPanel::OnLensTypeChanged (wxCommandEvent & e)
{
    // uses enum Lens::LensProjectionFormat from PanoramaMemento.h
    int var = m_lensTypeChoice->GetSelection();
    Lens lens = m_pano.getLens(0);
    if (lens.getProjection() != (Lens::LensProjectionFormat) var) {
        lens.setProjection((Lens::LensProjectionFormat) (var));
        GlobalCmdHist::getInstance().addCommand(
                new PT::ChangeLensCmd( m_pano, 0, lens )
            );
    }
}

void AssistantPanel::OnFocalLengthChanged(wxCommandEvent & e)
{
    // always change first lens
    wxString text = m_focalLengthText->GetValue();
    DEBUG_INFO("focal length: " << text.mb_str());
    double val;
    if (!str2double(text, val)) {
        return;
    }

    // always change first lens...
    Lens lens = m_pano.getLens(0);
    lens.setFocalLength(val);

    GlobalCmdHist::getInstance().addCommand(
            new PT::ChangeLensCmd( m_pano, 0, lens)
                                           );
}

void AssistantPanel::OnCropFactorChanged(wxCommandEvent & e)
{
    wxString text = m_cropFactorText->GetValue();
    DEBUG_INFO("crop factor: " << text.mb_str());
    double val;
    if (!str2double(text, val)) {
        return;
    }

    // always change first lens...
    Lens lens = m_pano.getLens(0);
    double fl = lens.getFocalLength();
    lens.setCropFactor(val);
    lens.setFocalLength(fl);

    GlobalCmdHist::getInstance().addCommand(
            new PT::ChangeLensCmd( m_pano, 0, lens)
        );
}

