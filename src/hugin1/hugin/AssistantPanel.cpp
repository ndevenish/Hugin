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

#include "base_wx/platform.h"

#include "common/stl_utils.h"

#include <map>

//#include <vigra_ext/PointMatching.h>
//#include <vigra_ext/LoweSIFT.h>

#include <PT/RandomPointSampler.h>
#include <PT/PhotometricOptimizer.h>
#include <PT/PTOptimise.h>

#include "common/wxPlatform.h"
#include "hugin/AssistantPanel.h"
#include "hugin/CommandHistory.h"
#include "base_wx/ImageCache.h"
#include "hugin/ImagesList.h"
#include "hugin/LensPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "hugin/AutoCtrlPointCreator.h"
#include "base_wx/PTWXDlg.h"
#include "hugin/TextKillFocusHandler.h"
#include "hugin/PanoDruid.h"
#include "base_wx/MyProgressDialog.h"
#include "hugin/config_defaults.h"

// Celeste header
#include "Celeste.h"
#include "CelesteGlobals.h"
#include "Utilities.h"

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
        size_t c=0;
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



BEGIN_EVENT_TABLE(AssistantPanel, wxPanel)
//    EVT_SIZE   ( AssistantPanel::OnSize )
    EVT_CHECKBOX   ( XRCID("ass_exif_cb"),          AssistantPanel::OnExifToggle)
    EVT_CHOICE     ( XRCID("ass_lens_proj_choice"), AssistantPanel::OnLensTypeChanged)
    EVT_TEXT_ENTER ( XRCID("ass_focallength_text"), AssistantPanel::OnFocalLengthChanged)
    EVT_TEXT_ENTER ( XRCID("ass_cropfactor_text"),  AssistantPanel::OnCropFactorChanged)
    EVT_BUTTON     ( XRCID("ass_load_lens_button"), AssistantPanel::OnLoadLens)
    EVT_BUTTON     ( XRCID("ass_load_images_button"), AssistantPanel::OnLoadImages)
    EVT_BUTTON     ( XRCID("ass_align_button"),     AssistantPanel::OnAlign)
    EVT_BUTTON     ( XRCID("ass_create_button"),    AssistantPanel::OnCreate)
END_EVENT_TABLE()


AssistantPanel::AssistantPanel()
{
    m_pano = 0;
}


// Define a constructor for the Assistant Panel
bool AssistantPanel::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                      long style, const wxString& name)
{
    DEBUG_TRACE("");
    m_pano = 0;
    m_noImage = true;

    if (! wxPanel::Create(parent, id, pos, size, style, name)) {
        return false;
    }

    wxXmlResource::Get()->LoadPanel(this, wxT("assistant_panel"));
    wxPanel * panel = XRCCTRL(*this, "assistant_panel", wxPanel);

    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
    topsizer->Add(panel, 1, wxEXPAND, 0);
    SetSizer( topsizer );
    //topsizer->SetSizeHints( this );

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

    m_panel = XRCCTRL(*this, "ass_control_panel", wxPanel);
    DEBUG_ASSERT(m_panel);

    m_panel->FitInside();

//    SetAutoLayout(false);

    m_degDigits = 2;


    return true;
}

void AssistantPanel::Init(Panorama * pano)
{
    m_pano = pano;
    // observe the panorama
    m_pano->addObserver(this);
}

AssistantPanel::~AssistantPanel(void)
{
    DEBUG_TRACE("dtor");
    m_focalLengthText->PopEventHandler(true);
    m_cropFactorText->PopEventHandler(true);
    m_pano->removeObserver(this);
    DEBUG_TRACE("dtor end");
}


// We need to override the default handling of size events because the
// sizers set the virtual size but not the actual size. We reverse
// the standard handling and fit the child to the parent rather than
// fitting the parent around the child
/*
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
*/
void AssistantPanel::panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & _imgNr)
{

}

void AssistantPanel::panoramaChanged(PT::Panorama &pano)
{
    DEBUG_TRACE("");

    if (m_druid) m_druid->Update(*m_pano);

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

        if (m_pano->getNrOfCtrlPoints() > 0) {
            // find components..
            CPGraph graph;
            createCPGraph(*m_pano, graph);
            CPComponents comps;
            int n= findCPComponents(graph, comps);
            if (n > 1) {
                alignMsg += wxString::Format(_("%d unconnected image groups found: "), n) + Components2Str(comps) + wxT("\n");
                alignMsg += _("Please use the Control Points tab to connect all images with control points.\n");
            } else {
                if (m_pano->needsOptimization()) {
                    alignMsg += _("Images or control points have changed, new alignment is needed.");
                } else {
                    double min;
                    double max;
                    double mean;
                    double var;
                    m_pano->calcCtrlPntsErrorStats( min, max, mean, var);

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
    // re-layout panel (adjusts m_alignText size)
    m_panel->FitInside();

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
    if (m_pano->getNrOfImages() < 2) {
        wxMessageBox(_("At least two images are required.\nPlease add more images."),_("Error"), wxOK, this);
        return;
    }

    fill_set(imgs, 0, m_pano->getNrOfImages()-1);

    long nFeatures = wxConfigBase::Get()->Read(wxT("/Assistant/nControlPoints"), HUGIN_ASS_NCONTROLPOINTS); 

    /*
    bool createCtrlP = true;
    // TODO: handle existing control points properly instead of adding them twice.
    if (m_pano->getNrOfCtrlPoints() > 0) {
    int a = wxMessageBox(wxString::Format(_("The panorama already has %d control points.\n\nSkip control points creation?"), m_pano->getNrOfCtrlPoints()),
    _("Skip control point creation?"), wxICON_QUESTION | wxYES_NO);
    createCtrlP = a != wxYES;
    }
    */

    bool createCtrlP = m_pano->getNrOfCtrlPoints() == 0;

    ProgressReporterDialog progress(5, _("Aligning images"), _("Finding corresponding points"),this);
    wxString alignMsg;
    if (createCtrlP) {
        AutoCtrlPointCreator matcher;
        CPVector cps = matcher.automatch(*m_pano, imgs, nFeatures,this);
        GlobalCmdHist::getInstance().addCommand(
            new PT::AddCtrlPointsCmd(*m_pano, cps)
            );
    }

    // Run Celeste
    bool t = wxConfigBase::Get()->Read(wxT("/Celeste/Auto"), HUGIN_CELESTE_AUTO); 
    if (t && m_pano->getNrOfCtrlPoints())
    {

        DEBUG_TRACE("Running Celeste");
        progress.increaseProgress(1, std::string(wxString(_("Running Celeste")).mb_str(wxConvLocal)));

        // set numeric locale to C, for correct number output
        char * old_locale = setlocale(LC_NUMERIC,NULL);
        setlocale(LC_NUMERIC,"C");

        // determine file name of SVM model file
        // get XRC path from application
        wxString wxstrModelFileName = huginApp::Get()->GetXRCPath() + wxT("data/") + wxT(HUGIN_CELESTE_MODEL);
        // convert wxString to string
        string strModelFileName(wxstrModelFileName.mb_str(wxConvUTF8));

        // SVM model file
        if ( wxFile::Exists(wxstrModelFileName) )
        {

            for (unsigned int imgNr = 0; imgNr < m_pano->getNrOfImages() - 1; imgNr++){

                double progress_amount =  (double)1/m_pano->getNrOfImages();
                progress.increaseProgress(progress_amount, std::string(wxString(_("Running Celeste")).mb_str(wxConvLocal)));

                const CPVector & controlPoints = m_pano->getCtrlPoints();
                unsigned int removed = 0;

                gNumLocs = 0;
                for (PT::CPVector::const_iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
                    PT::ControlPoint point = *it;
                    if (imgNr == point.image1Nr){
                        gNumLocs++;				
                    }
                    if (imgNr == point.image2Nr){
                        gNumLocs++;				
                    }	
                }		

                // Create the storage matrix
                gLocations = CreateMatrix( (int)0, gNumLocs, 2);
                unsigned int glocation_counter = 0;
                unsigned int cp_counter = 0;	
                vector<unsigned int> global_cp_nr;

                for (PT::CPVector::const_iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
                    PT::ControlPoint point = *it;
                    if (imgNr == point.image1Nr){	
                        gLocations[glocation_counter][0] = (int)point.x1;
                        gLocations[glocation_counter][1] = (int)point.y1;
                        global_cp_nr.push_back(cp_counter);	
                        glocation_counter++;				
                    }
                    if (imgNr == point.image2Nr){
                        gLocations[glocation_counter][0] = (int)point.x2;
                        gLocations[glocation_counter][1] = (int)point.y2;
                        global_cp_nr.push_back(cp_counter);	
                        glocation_counter++;				
                    }
                    cp_counter++;	
                }

                // SVM threshold
                double threshold = HUGIN_CELESTE_THRESHOLD;
                wxConfigBase::Get()->Read(wxT("/Celeste/Threshold"), &threshold, HUGIN_CELESTE_THRESHOLD);

                // Mask resolution - 1 sets it to fine
                bool t = wxConfigBase::Get()->Read(wxT("/Celeste/Filter"), HUGIN_CELESTE_FILTER);
                if (t){
                    //cerr <<"---Celeste--- Using small filter" << endl;
                    gRadius = 10;
                    spacing = (gRadius * 2) + 1;
                }

                // Image to analyse
                string imagefile = m_pano->getImage(imgNr).getFilename();

                // Print progress
                MainFrame::Get()->SetStatusText(_("searching for cloud-like control points..."),0);

                // Vector to store Gabor filter responses
                vector<double> svm_responses_ap;
                string mask_format = "PNG";
                unsigned int mask = 0;

                // Get responses
                get_gabor_response(imagefile, mask, strModelFileName, threshold, mask_format, svm_responses_ap);

                // Print SVM results
                for (unsigned int c = 0; c < svm_responses_ap.size(); c++){

                    unsigned int pNr = global_cp_nr[c] - removed;	

                    if (svm_responses_ap[c] >= threshold){

                        DEBUG_DEBUG("about to delete point " << pNr);
                        GlobalCmdHist::getInstance().addCommand(
                            new PT::RemoveCtrlPointCmd(*m_pano,pNr)
                            );
                        removed++;					
                        cout << "CP: " << c << "\tSVM Score: " << svm_responses_ap[c] << "\tremoved." << endl;
                    }
                }
                if (removed) cout << endl;

            }
        }else{	
            wxMessageBox(_("Celeste model file not found, Hugin needs to be properly installed." ), _("Fatal Error"));
        }	

        MainFrame::Get()->SetStatusText(_(""),0);

        // reset locale
        setlocale(LC_NUMERIC,old_locale);

    }
    DEBUG_TRACE("Finished running Celeste");

    progress.increaseProgress(1.0, std::string(wxString(_("Determining placement of the images")).mb_str(wxConvLocal)));

    // find components..
    CPGraph graph;
    createCPGraph(*m_pano, graph);
    CPComponents comps;
    int n = findCPComponents(graph, comps);

    if (n > 1) {
        // switch to images panel.
        unsigned i1 = *(comps[0].rbegin());
        unsigned i2 = *(comps[1].begin());
        MainFrame::Get()->ShowCtrlPointEditor( i1, i2);
        // display message box with 

        wxMessageBox(wxString::Format(_("Warning %d unconnected image groups found:"), n) + Components2Str(comps) + wxT("\n")
            + _("Please create control points between unconnected images using the Control Points tab.\n\nAfter adding the points, press the \"Align\" button again"),_("Error"), wxOK , this);
        return;
    }

    // optimize panorama

    Panorama optPano = m_pano->getSubset(imgs);

    // set TIFF_m with enblend
    PanoramaOptions opts = m_pano->getOptions();
    opts.outputFormat = PanoramaOptions::TIFF;
    opts.blendMode = PanoramaOptions::ENBLEND_BLEND;
    opts.remapper = PanoramaOptions::NONA;
    opts.tiff_saveROI = true;
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
        registerPTWXDlgFcn(MainFrame::Get());
    }

    progress.increaseProgress(1.0, std::string(wxString(_("Leveling the panorama")).mb_str(wxConvLocal)));

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

    progress.increaseProgress(1.0, std::string(wxString(_("Loading images")).mb_str(wxConvLocal)));

    // TODO: photometric optimisation.
    // first, ensure that vignetting and response coefficients are linked
    const char * varnames[] = {"Va", "Vb", "Vc", "Vd", "Vx", "Vy",
        "Ra", "Rb", "Rc", "Rd", "Re",  0};

    for (size_t i = 0; i < optPano.getNrOfLenses(); i++) {
        const Lens & l = optPano.getLens(i);
        for (const char ** v = varnames; *v; v++) {
            LensVariable var = const_map_get(l.variables, *v);
            if (!var.isLinked()) {
                var.setLinked();
                optPano.updateLensVariable(i, var);
            }
        }
    }

    // check if this is an HDR image
    // (check for large exposure differences)
    double min_exp, max_exp;
    min_exp = max_exp = const_map_get(m_pano->getImageVariables(0), "Eev").getValue();
    for (size_t i = 1; i < m_pano->getNrOfImages(); i++) {
        double ev = const_map_get(m_pano->getImageVariables(i), "Eev").getValue();
        min_exp = std::min(min_exp, ev);
        max_exp = std::max(max_exp, ev);
    }
    if (max_exp - min_exp > 3) {
        // decide between hdr and enfuse mode...
        // switch to enfuse mode...
        opts.outputLDRBlended = false;
        opts.outputLDRLayers = false;
        opts.outputLDRExposureLayers = false;
        opts.outputLDRExposureBlended = true;
        opts.outputHDRBlended = false;
        opts.outputHDRLayers = false;
        opts.outputHDRStacks = false;

        opts.outputMode = PanoramaOptions::OUTPUT_HDR;
    } else {
        // normal mode, no special multiple exposure blending
        opts.outputLDRBlended = true;
        opts.outputLDRLayers = false;
        opts.outputLDRExposureLayers = false;
        opts.outputLDRExposureBlended = false;
        opts.outputHDRBlended = false;
        opts.outputHDRLayers = false;
        opts.outputHDRStacks = false;
    }

    MainFrame::Get()->resetProgress(3);

    // photometric estimation
    int nPoints = wxConfigBase::Get()->Read(wxT("OptimizePhotometric/nRandomPointsPerImage"),200l);
    nPoints = nPoints * optPano.getNrOfImages();
    // get the small images
    std::vector<vigra::FRGBImage *> srcImgs;
    for (size_t i=0; i < optPano.getNrOfImages(); i++) {
        ImageCache::EntryPtr e = ImageCache::getInstance().getSmallImage(optPano.getImage(i).getFilename());
        vigra::FRGBImage * img = new FRGBImage;
        if (!e) {
            wxMessageBox(_("Error: could not load all images"), 
                _("Error"),wxOK,this);
            return;
        }
        if (e->origType == "UINT8") {
            reduceToNextLevel(*(e->image8), *img);
            transformImage(vigra::srcImageRange(*img), vigra::destImage(*img),
                vigra::functor::Arg1()/vigra::functor::Param(255.0));
        } else if (e->origType == "UINT16") {
            reduceToNextLevel(*(e->image16), *img);
            transformImage(vigra::srcImageRange(*img), vigra::destImage(*img),
                vigra::functor::Arg1()/vigra::functor::Param(65535.0));
        } else {
            reduceToNextLevel(*(e->imageFloat), *img);
        }
        srcImgs.push_back(img);
    }
    bool randomPoints = true;
    std::vector<vigra_ext::PointPairRGB> points;
    extractPoints(optPano, srcImgs, nPoints, randomPoints, *(MainFrame::Get()), points);

    progress.increaseProgress(1.0, std::string(wxString(_("Vignetting and exposure correction")).mb_str(wxConvLocal)));

    PhotometricOptimizeMode poptmode = OPT_PHOTOMETRIC_LDR;
    if (opts.outputMode == PanoramaOptions::OUTPUT_HDR) {
        poptmode = OPT_PHOTOMETRIC_HDR;
    }
    double error;
    smartOptimizePhotometric(optPano, poptmode,
        points, *(MainFrame::Get()), error);
    cout << "Auto align, photometric error: " << error *255 << " grey values" << std::endl;

    // calculate the mean exposure.
    opts.outputExposureValue = calcMeanExposure(*m_pano);

    // TODO: merge the following commands.


    // copy information into the main panorama
    GlobalCmdHist::getInstance().addCommand(
        new PT::UpdateVariablesCPSetCmd(*m_pano, imgs, optPano.getVariables(), optPano.getCtrlPoints())
        );

    // copy information into our panorama
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd(*m_pano, opts)
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
    PanoramaOptions opts = m_pano->getOptions();
    int w = m_pano->calcOptimalWidth();
    // check if resize was plausible!
    if (w> 0) {
        opts.setWidth(floori(w*sizeFactor), true);
        // copy information into our panorama
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd(*m_pano, opts)
            );
    }

    wxCommandEvent dummy;
    MainFrame::Get()->OnDoStitch(dummy);
}

void AssistantPanel::OnLoadLens(wxCommandEvent & e)
{
    unsigned int imgNr = 0;
    unsigned int lensNr = m_pano->getImage(imgNr).getLensNr();
    Lens lens = m_pano->getLens(lensNr);
    VariableMap vars = m_pano->getImageVariables(imgNr);
    ImageOptions imgopts = m_pano->getImage(imgNr).getOptions();

    if (LoadLensParametersChoose(this, lens, vars, imgopts)) {
        GlobalCmdHist::getInstance().addCommand(
                new PT::ChangeLensCmd(*m_pano, lensNr, lens)
                                               );
        GlobalCmdHist::getInstance().addCommand(
                new PT::UpdateImageVariablesCmd(*m_pano, imgNr, vars)
                                               );
                // get all images with the current lens.
        UIntSet imgs;
        for (unsigned int i = 0; i < m_pano->getNrOfImages(); i++) {
            if (m_pano->getImage(i).getLensNr() == lensNr) {
                imgs.insert(i);
            }
        }

        // set image options.
        GlobalCmdHist::getInstance().addCommand(
                new PT::SetImageOptionsCmd(*m_pano, imgopts, imgs) );
    }

}

void AssistantPanel::OnExifToggle (wxCommandEvent & e)
{
    if (m_exifToggle->GetValue()) {
        unsigned int imgNr = 0;
        // if activated, load exif info
        double cropFactor = 0;
        double focalLength = 0;
        SrcPanoImage srcImg = m_pano->getSrcImage(imgNr);
        bool ok = initImageFromFile(srcImg, focalLength, cropFactor);
        if (! ok) {
            if (!getLensDataFromUser(this, srcImg, focalLength, cropFactor)) {
                // hmm, we don't know anything, assume a standart lens.
                srcImg.setHFOV(50);
            }
        }
                //initLensFromFile(pano.getImage(imgNr).getFilename().c_str(), c, lens, vars, imgopts, true);
        GlobalCmdHist::getInstance().addCommand(
                new PT::UpdateSrcImageCmd( *m_pano, imgNr, srcImg)
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
    Lens lens = m_pano->getLens(0);
    if (lens.getProjection() != (Lens::LensProjectionFormat) var) {
        //double crop = lens.getCropFactor();
        double fl = lens.getFocalLength();
        lens.setProjection((Lens::LensProjectionFormat) (var));
        lens.setFocalLength(fl);
        GlobalCmdHist::getInstance().addCommand(
                new PT::ChangeLensCmd(*m_pano, 0, lens )
            );
    }
}

void AssistantPanel::OnFocalLengthChanged(wxCommandEvent & e)
{
    if (m_pano->getNrOfImages() == 0) return;

    // always change first lens
    wxString text = m_focalLengthText->GetValue();
    DEBUG_INFO("focal length: " << text.mb_str(wxConvLocal));
    double val;
    if (!str2double(text, val)) {
        return;
    }


    // always change first lens...
    Lens lens = m_pano->getLens(0);
    lens.setFocalLength(val);
    LensVariable v = map_get(lens.variables, "v");
    LensVarMap lmv;
    lmv.insert(make_pair("v", v));

    std::vector<LensVarMap> lvars;
    UIntSet lenses;
    for (unsigned i=0; i < m_pano->getNrOfLenses(); i++) {
        lenses.insert(i);
        lvars.push_back(lmv);
    }
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetLensesVariableCmd(*m_pano, lenses, lvars)
                                           );
}

void AssistantPanel::OnCropFactorChanged(wxCommandEvent & e)
{
    wxString text = m_cropFactorText->GetValue();
    DEBUG_INFO("crop factor: " << text.mb_str(wxConvLocal));
    double val;
    if (!str2double(text, val)) {
        return;
    }

    // always change first lens...
    Lens lens = m_pano->getLens(0);
    double fl = lens.getFocalLength();
    lens.setCropFactor(val);
    lens.setFocalLength(fl);
    LensVariable v = map_get(lens.variables, "v");
    LensVarMap lmv;
    lmv.insert(make_pair("v", v));

    std::vector<LensVarMap> lvars;
    UIntSet lenses;
    for (unsigned i=0; i < m_pano->getNrOfLenses(); i++) {
        lenses.insert(i);
        lvars.push_back(lmv);
    }
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetLensesVariableCmd(*m_pano, lenses, lvars)
                                           );
    // TODO: update crop factor as well without destroying other information.
    SrcPanoImage img0 = m_pano->getSrcImage(0);
    img0.setExifCropFactor(val);
    GlobalCmdHist::getInstance().addCommand(
            new PT::UpdateSrcImageCmd(*m_pano, 0, img0)
                                           );
    
}

IMPLEMENT_DYNAMIC_CLASS(AssistantPanel, wxPanel)

AssistantPanelXmlHandler::AssistantPanelXmlHandler()
                : wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *AssistantPanelXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, AssistantPanel)

    cp->Create(m_parentAsWindow,
                   GetID(),
                   GetPosition(), GetSize(),
                   GetStyle(wxT("style")),
                   GetName());

    SetupWindow( cp);

    return cp;
}

bool AssistantPanelXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("AssistantPanel"));
}

IMPLEMENT_DYNAMIC_CLASS(AssistantPanelXmlHandler, wxXmlResourceHandler)
