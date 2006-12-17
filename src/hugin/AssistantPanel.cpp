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
#define GET_VAR(val) m_pano.getVariable(orientationEdit_RefImg).val.getValue()

BEGIN_EVENT_TABLE(AssistantPanel, wxWindow)
    EVT_SIZE   ( AssistantPanel::OnSize )
//    EVT_MOUSE_EVENTS ( AssistantPanel::OnMouse )
//    EVT_MOTION ( AssistantPanel::ChangePreview )
    EVT_CHECKBOX   ( XRCID("ass_exif_cb"),          AssistantPanel::OnExifToggle)
    EVT_CHOICE     ( XRCID("ass_lens_type_choice"), AssistantPanel::OnLensTypeChanged)
    EVT_TEXT_ENTER ( XRCID("ass_focallength_text"), AssistantPanel::OnFocalLengthChanged)
    EVT_TEXT_ENTER ( XRCID("ass_cropfactor_text"),  AssistantPanel::OnCropFactorChanged)
    EVT_BUTTON     ( XRCID("ass_load_lens_button"), AssistantPanel::OnLoadLens)
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

    m_degDigits = 1;

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
#ifdef USE_WX253
    int winWidth, winHeight;
    GetClientSize(&winWidth, &winHeight);
    XRCCTRL(*this, "assistant_panel", wxPanel)->SetSize (winWidth, winHeight);
    DEBUG_INFO( "assistant panel: " << winWidth <<"x"<< winHeight );
    DEBUG_INFO( "assistant controls: " << winWidth <<"x"<< winHeight );
    m_panel->SetSize(winWidth, winHeight);
#else
    wxSize new_size = GetSize();
    XRCCTRL(*this, "assistant_panel", wxPanel)->SetSize ( new_size );
    DEBUG_INFO( "assistant panel: " << new_size.GetWidth() <<"x"<< new_size.GetHeight()  );
#endif

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
        // TODO: check if the positions are not all zero, if more than 1 image has been loaded
        m_createButton->Enable();
        m_imagesText->SetLabel(wxString::Format(_("%d images loaded."), pano.getNrOfImages()));

        const Lens & lens = pano.getLens(0);

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

        // update focal length
        double focal_length = lens.getFocalLength();
        m_focalLengthText->SetValue(doubleTowxString(focal_length,m_degDigits));

        // update focal length factor
        double focal_length_factor = lens.getCropFactor();
        m_cropFactorText->SetValue(doubleTowxString(focal_length_factor,m_degDigits));
    }

    if (pano.getNrOfImages() > 1) {
        wxString alignMsg = wxString::Format(_("%d control points.\n"), pano.getCtrlPoints().size());

        double min;
        double max;
        double mean;
        double var;
        m_pano.calcCtrlPntsErrorStats( min, max, mean, var);

        wxString distStr;
        if (mean < 1)
            distStr = _("Very good fit");
        else if (mean < 3)
            distStr = _("Good fit");
        else if (mean < 7)
            distStr = _("Bad fit, some control points might be bad, or there are parallax and movement errors");
        else
            distStr = _("Very bad fit. Check for bad control points, or images with parallax or movement. The optimizer might have failed. Manual intervention required.");

        alignMsg = alignMsg + wxString::Format(_("Mean error after optimization: %.1f pixel\n"), mean)
                + distStr; 

        // TODO: check if all images are connected, and notify user, if they are not.

        // need to resize the text widget somehow!
        m_alignText->SetLabel(alignMsg);
    }

    // TODO: update meaningful help text and dynamic links to relevant tabs
}

// #####  Here start the eventhandlers  #####

// Yaw by text -> double
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

    // TODO: make configurable
    long nFeatures = 15;

    bool createCtrlP = true;
    // TODO: handle existing control points properly instead of adding them twice.
    if (m_pano.getNrOfCtrlPoints() > 0) {
        int a = wxMessageBox(wxString::Format(_("The panorama already has %d control points.\n\nSkip control points creation?"), m_pano.getNrOfCtrlPoints()),
                     _("Skip control point creation?"), wxICON_QUESTION | wxYES_NO);
        createCtrlP = a != wxYES;
    }

    wxString alignMsg;
    if (createCtrlP) {
        AutoCtrlPointCreator matcher;
        CPVector cps = matcher.automatch(m_pano, imgs, nFeatures);
        int nCP = cps.size();

        GlobalCmdHist::getInstance().addCommand(
                new PT::AddCtrlPointsCmd(m_pano, cps)
                                               );
    }

    // TODO: check if enough control points have been created. and that there are
    // no missing links

    // optimize panorama

    Panorama optPano = m_pano.getSubset(imgs);

    // TODO: set proper optimisation settings.
    // set TIFF_m with enblend
    PanoramaOptions opts = m_pano.getOptions();
    opts.outputFormat = PanoramaOptions::TIFF;
    opts.blendMode = PanoramaOptions::ENBLEND_BLEND;
    opts.remapper = PanoramaOptions::NONA;
    opts.tiff_saveROI = true;
    opts.tiffCompression = "Deflate";
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

    // center and resize frame
    optPano.centerHorizontically();
    opts = optPano.getOptions();
    double hfov, vfov, height;
    optPano.fitPano(hfov, height);
    opts.setHFOV(hfov);
    opts.setHeight(roundi(height));
    vfov = opts.getVFOV();

    // choose proper projection type
    if (vfov < 100) {
        // cylindrical or rectilinear
        if (hfov < 100) {
            opts.setProjection(PanoramaOptions::RECTILINEAR);
        } else {
            opts.setProjection(PanoramaOptions::CYLINDRICAL);
        }
    }

    // calc optimal size using output projection
    optPano.setOptions(opts);
    w = optPano.calcOptimalWidth();
    opts.setWidth(w, true);
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
    wxCommandEvent dummy;
    MainFrame::Get()->OnDoStitch(dummy);
}

void AssistantPanel::OnLoadLens(wxCommandEvent & e)
{
    unsigned int imgNr = 0;
    unsigned int lensNr = 0;
    Lens lens = m_pano.getLens(lensNr);
    VariableMap vars = m_pano.getImageVariables(imgNr);
    ImageOptions imgopts = m_pano.getImage(imgNr).getOptions();
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
            cfg.Read(wxT("Lens/crop"), &d);
            lens.setCropFactor(d);
            cfg.Read(wxT("Lens/hfov"), &d);
            map_get(vars,"v").setValue(d);
            DEBUG_DEBUG("read lens hfov: " << d);

                // loop to load lens variables
            char ** varname = Lens::variableNames;
            while (*varname) {
                wxString key(wxT("Lens/"));
                key.append(wxString(*varname, *wxConvCurrent));
                d = 0;
                if (cfg.Read(key,&d)) {
                        // only set value if variabe was found in the script
                    map_get(vars,*varname).setValue(d);

                    integer = 1;
                    key.append(wxT("_link"));
                    cfg.Read(key, &integer);
                    map_get(lens.variables, *varname).setLinked(integer != 0);
                }


                varname++;
            }
            long vigCorrMode=0;
            cfg.Read(wxT("Lens/vigCorrMode"), &vigCorrMode);
            imgopts.m_vigCorrMode = vigCorrMode;

            wxString flatfield;
            bool readok = cfg.Read(wxT("Lens/flatfield"), &flatfield);
            imgopts.m_flatfield = std::string((const char *)flatfield.mb_str());

                // TODO: crop parameters

        }
            // reset locale
        setlocale(LC_NUMERIC,old_locale);

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
        // if activated, load exif info
        Lens lens = m_pano.getLens(0);
        // check file extension
        wxFileName file(wxString(m_pano.getImage(0).getFilename().c_str(), *wxConvCurrent));
        if (file.GetExt().CmpNoCase(wxT("jpg")) == 0 ||
            file.GetExt().CmpNoCase(wxT("jpeg")) == 0 )
        {
            double c=0;
            double roll = 0;
            initLensFromFile(m_pano.getImage(0).getFilename().c_str(), c, lens, roll);
            GlobalCmdHist::getInstance().addCommand(
                    new PT::ChangeLensCmd( m_pano, 0,
                                           lens )
                                                   );
        } else {
            wxLogError(_("Not a jpeg file:") + file.GetName());
        }
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

