// -*- c-basic-offset: 4 -*-

/** @file AssistantPanel.cpp
 *
 *  @brief implementation of AssistantPanel Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
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
#include "base_wx/huginConfig.h"
#include "base_wx/LensTools.h"

#include "PT/ImageGraph.h"
#include "base_wx/wxPlatform.h"
#include "hugin/AssistantPanel.h"
#include "hugin/CommandHistory.h"
#include "base_wx/RunStitchPanel.h"
#include "hugin/ImagesList.h"
#include "hugin/LensPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "hugin/TextKillFocusHandler.h"
#include "hugin/config_defaults.h"
#include "hugin/wxPanoCommand.h"

using namespace hugin_utils;
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
    EVT_BUTTON     ( XRCID("ass_align_batch_button"), AssistantPanel::OnAlignSendToBatch)
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
    FillLensProjectionList(m_lensTypeChoice);
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

    m_alignBatchButton = XRCCTRL(*this, "ass_align_batch_button", wxButton);
    DEBUG_ASSERT(m_alignBatchButton);
    m_alignBatchButton->Disable();

    m_alignText = XRCCTRL(*this, "ass_align_text", wxStaticText);
    DEBUG_ASSERT(m_alignText);

    m_createButton = XRCCTRL(*this, "ass_create_button", wxButton);
    DEBUG_ASSERT(m_createButton);
    m_createButton->Disable();

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
    m_variable_groups = new HuginBase::StandardImageVariableGroups(*m_pano);
    // observe the panorama
    m_pano->addObserver(this);
}

AssistantPanel::~AssistantPanel(void)
{
    DEBUG_TRACE("dtor");
    m_focalLengthText->PopEventHandler(true);
    m_cropFactorText->PopEventHandler(true);
    m_pano->removeObserver(this);
    delete m_variable_groups;
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
    
    m_variable_groups->update();

    m_alignButton->Enable(pano.getNrOfImages() > 1);
    m_alignBatchButton->Enable(pano.getNrOfImages() > 1);

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

        // in wxWidgets 2.9, format must have types that exactly match.
        // However std::size_t could be any unsiged integer, so we cast it to
        // unsigned long.int to be on the safe side.
        wxString imgMsg = wxString::Format(_("%lu images loaded."), (unsigned long int) pano.getNrOfImages());

        const Lens & lens = m_variable_groups->getLens(0);

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
        SelectProjection(m_lensTypeChoice, lens.getProjection());
        double focal_length = lens.getFocalLength();
        m_focalLengthText->SetValue(doubleTowxString(focal_length,m_degDigits));
        double focal_length_factor = lens.getCropFactor();
        m_cropFactorText->SetValue(doubleTowxString(focal_length_factor,m_degDigits));

        m_imagesText->SetLabel(imgMsg);
    }

    if (pano.getNrOfImages() > 1) {
        // in wxWidgets 2.9, format must have types that exactly match.
        // However std::size_t could be any unsiged integer, so we cast it to
        // unsigned long.int to be on the safe side.
        wxString alignMsg = wxString::Format(_("Images are connected by %lu control points.\n"), (unsigned long int) pano.getCtrlPoints().size());

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
        m_alignText->SetLabel(alignMsg);
    } else {
        m_alignText->SetLabel(_("Note: automatic alignment uses default settings from the preferences. If you customize settings for this project in the advanced tabs and want to use these customized settings, run the CP detection from the Images Tab, the geometrical optimization from the Optimizer tab and the photometric optimization from the Exposure tab."));
    }
    /** @todo Use width of avaliable space to wrap this message.
     */
        m_alignText->Wrap(600);
    // re-layout panel (adjusts m_alignText size)
    m_panel->Layout();
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
    if (m_pano->getNrOfImages() < 2) {
        wxMessageBox(_("At least two images are required.\nPlease add more images."),_("Error"), wxOK, this);
        return;
    }

    //generate list of all necessary programs with full path
    wxString bindir = huginApp::Get()->GetUtilsBinDir();
    wxConfigBase* config=wxConfigBase::Get();
    AssistantPrograms progs = getAssistantProgramsConfig(bindir, config);

    //read main settings
    bool runCeleste=config->Read(wxT("/Celeste/Auto"), HUGIN_CELESTE_AUTO)!=0;
    double celesteThreshold;
    config->Read(wxT("/Celeste/Threshold"), &celesteThreshold, HUGIN_CELESTE_THRESHOLD);
    bool celesteSmall=config->Read(wxT("/Celeste/Filter"), HUGIN_CELESTE_FILTER)==0;
    bool runCPClean=config->Read(wxT("/Assistant/AutoCPClean"), HUGIN_ASS_AUTO_CPCLEAN)!=0;
    double scale;
    config->Read(wxT("/Assistant/panoDownsizeFactor"), &scale, HUGIN_ASS_PANO_DOWNSIZE_FACTOR);
    int scalei=roundi(scale*100);

    //save project into temp directory
    wxString tempDir= config->Read(wxT("tempDir"),wxT(""));
    if(!tempDir.IsEmpty())
        if(tempDir.Last()!=wxFileName::GetPathSeparator())
            tempDir.Append(wxFileName::GetPathSeparator());
    wxString scriptName=wxFileName::CreateTempFileName(tempDir+wxT("ha"));
    std::ofstream script(scriptName.mb_str(HUGIN_CONV_FILENAME));
    script.exceptions ( std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit );
    PT::UIntSet all;
    fill_set(all, 0, m_pano->getNrOfImages()-1);
    m_pano->printPanoramaScript(script, m_pano->getOptimizeVector(), m_pano->getOptions(), all, false);
    script.close();
    //generate makefile
    wxString makefileName=wxFileName::CreateTempFileName(tempDir+wxT("ham"));
    std::ofstream makefile(makefileName.mb_str(HUGIN_CONV_FILENAME));
    makefile.exceptions( std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit );
    std::string scriptString(scriptName.mb_str(HUGIN_CONV_FILENAME));
    HuginBase::AssistantMakefilelibExport::createMakefile(*m_pano,progs,runCeleste,celesteThreshold,celesteSmall,
        runCPClean,scale,makefile,scriptString);
    makefile.close();

    //execute makefile
    wxString args = wxT("-f ") + wxQuoteFilename(makefileName) + wxT(" all");
    int ret=MyExecuteCommandOnDialog(getGNUMakeCmd(args),wxEmptyString,this,_("Running assistant"),true);

    //read back panofile
    GlobalCmdHist::getInstance().addCommand(new wxLoadPTProjectCmd(*m_pano,
        (const char *)scriptName.mb_str(HUGIN_CONV_FILENAME),"",ret==0));

    //delete temporary files
    wxRemoveFile(scriptName);
    wxRemoveFile(makefileName);
    //if return value is non-zero, an error occured in assistant makefile
    if(ret!=0)
    {
        //check for unconnected images
        CPGraph graph;
        createCPGraph(*m_pano, graph);
        CPComponents comps;
        int n = findCPComponents(graph, comps);
        if(n > 1)
        {
            // switch to images panel.
            unsigned i1 = *(comps[0].rbegin());
            unsigned i2 = *(comps[1].begin());
            MainFrame::Get()->ShowCtrlPointEditor( i1, i2);
            // display message box with 
            wxMessageBox(wxString::Format(_("Warning %d unconnected image groups found:"), n) + Components2Str(comps) + wxT("\n")
                + _("Please create control points between unconnected images using the Control Points tab.\n\nAfter adding the points, press the \"Align\" button again"),_("Error"), wxOK , this);
            return;
        };
        wxMessageBox(_("The assistant did not complete successfully. Please check the resulting project file."),
                     _("Warning"),wxOK | wxICON_INFORMATION, this); 
    };

    // show preview frame
    wxCommandEvent dummy;
    long preview=wxConfigBase::Get()->Read(wxT("/Assistant/PreviewWindow"),HUGIN_ASS_PREVIEW);
    switch(preview)
    {
        case 1:   
            MainFrame::Get()->OnToggleGLPreviewFrame(dummy); 
            break;
        case 2:
            MainFrame::Get()->OnTogglePreviewFrame(dummy);
            break;
    };

    // enable stitch button
    m_createButton->Enable();
}

void AssistantPanel::OnAlignSendToBatch(wxCommandEvent &e)
{
	wxCommandEvent dummy;
	MainFrame::Get()->OnSaveProject(dummy);
	wxString projectFile = MainFrame::Get()->getProjectName();
	if(wxFileName::FileExists(projectFile))
	{
#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
        wxExecute(_T("open -b net.sourceforge.hugin.PTBatcherGUI -a"+wxQuoteFilename(projectFile)));
#else
#ifdef __WINDOWS__
		wxString huginPath = getExePath(wxGetApp().argv[0])+wxFileName::GetPathSeparator(); 
#else
		wxString huginPath = _T("");	//we call the batch processor directly without path on linux
#endif	
		wxExecute(huginPath+wxT("PTBatcherGUI -a ")+wxQuoteFilename(projectFile));
#endif
	}
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
    unsigned int lensNr = m_variable_groups->getLenses().getPartNumber(imgNr);
    Lens lens = m_variable_groups->getLensForImage(imgNr);
    VariableMap vars = m_pano->getImageVariables(imgNr);
    ImageOptions imgopts = m_pano->getImage(imgNr).getOptions();

    if (LoadLensParametersChoose(this, lens, vars, imgopts)) {
        /** @todo maybe this isn't the best way to load the lens data.
         * Check with LoadLensParamtersChoose how this is done, and use only
         * the image variables, rather than merging in the lens ones.
         */
        for (LensVarMap::iterator it = lens.variables.begin();
             it != lens.variables.end(); it++)
        {
            vars.insert(pair<std::string, HuginBase::Variable>(
                        it->first,
                        HuginBase::Variable(it->second.getName(),
                                            it->second.getValue() )
                    ));
        }
        /** @todo I think the sensor size should be copied over,
         * but SrcPanoImage doesn't have such a variable yet.
         */
        std::vector<PanoCommand*> commands;
        commands.push_back(new PT::UpdateImageVariablesCmd(*m_pano, imgNr, vars));
        // get all images with the current lens.
        UIntSet imgs;
        for (unsigned int i = 0; i < m_pano->getNrOfImages(); i++) {
            if (m_variable_groups->getLenses().getPartNumber(i) == lensNr) {
                imgs.insert(i);
            }
        }

        // set image options.
        commands.push_back(new PT::SetImageOptionsCmd(*m_pano, imgopts, imgs) );
        GlobalCmdHist::getInstance().addCommand(
                new PT::CombinedPanoCommand(*m_pano, commands));
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
        bool ok = initImageFromFile(srcImg, focalLength, cropFactor, true);
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
    size_t var = GetSelectedProjection(m_lensTypeChoice);
    Lens lens = m_variable_groups->getLens(0);
    if (lens.getProjection() != (Lens::LensProjectionFormat) var) {
        //double crop = lens.getCropFactor();
        double fl = lens.getFocalLength();
        UIntSet imgs;
        imgs.insert(0);
        std::vector<PanoCommand*> commands;
        commands.push_back(
                new PT::ChangeImageProjectionCmd(
                                    *m_pano,
                                    imgs,
                                    (HuginBase::SrcPanoImage::Projection) var
                                )
            );
        
        commands.push_back(new PT::UpdateFocalLengthCmd(*m_pano, imgs, fl));
        GlobalCmdHist::getInstance().addCommand(
                new PT::CombinedPanoCommand(*m_pano, commands));
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
    UIntSet images0;
    images0.insert(0);
    GlobalCmdHist::getInstance().addCommand(
        new PT::UpdateFocalLengthCmd(*m_pano, images0, val)
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

    UIntSet images;
    images.insert(0);
    GlobalCmdHist::getInstance().addCommand(
        new PT::UpdateCropFactorCmd(*m_pano,images,val)
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
