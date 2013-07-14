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

#include <config.h>
#include "panoinc_WX.h"
#include "panoinc.h"
#include <time.h>

#include "base_wx/platform.h"
#include "base_wx/wxPlatform.h"
#include <vector>
#include <map>

#include "hugin/ImagesPanel.h"
#include "hugin/CommandHistory.h"
#include "hugin/TextKillFocusHandler.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/ImagesList.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "icpfind/AutoCtrlPointCreator.h"
#include "hugin/config_defaults.h"
#include "base_wx/MyProgressDialog.h"
#include "base_wx/PTWXDlg.h"
#include <PT/PTOptimise.h>
#include "base_wx/LensTools.h"
#include "hugin/ImagesTree.h"
#include "panodata/OptimizerSwitches.h"

#include <panodata/StandardImageVariableGroups.h>

using namespace PT;
using namespace hugin_utils;
using namespace vigra;
using namespace vigra_ext;
using namespace std;

BEGIN_EVENT_TABLE(ImagesPanel, wxPanel)
    EVT_TREE_SEL_CHANGED(XRCID("images_tree_ctrl"), ImagesPanel::OnSelectionChanged )
    EVT_CHOICE     ( XRCID("images_lens_type"), ImagesPanel::OnLensTypeChanged)
    EVT_CHOICE     ( XRCID("images_group_mode"), ImagesPanel::OnGroupModeChanged)
    EVT_RADIOBOX   ( XRCID("images_column_radiobox"), ImagesPanel::OnDisplayModeChanged)
    EVT_CHOICE     ( XRCID("images_optimize_mode"), ImagesPanel::OnOptimizerSwitchChanged)
    EVT_CHOICE     ( XRCID("images_photo_optimize_mode"), ImagesPanel::OnPhotometricOptimizerSwitchChanged)
    EVT_TEXT_ENTER ( XRCID("images_focal_length"), ImagesPanel::OnFocalLengthChanged)
    EVT_TEXT_ENTER ( XRCID("images_crop_factor"),  ImagesPanel::OnCropFactorChanged)
    EVT_TEXT_ENTER ( XRCID("images_overlap"), ImagesPanel::OnMinimumOverlapChanged)
    EVT_TEXT_ENTER ( XRCID("images_maxev"), ImagesPanel::OnMaxEvDiffChanged)
    EVT_BUTTON     ( XRCID("images_feature_matching"), ImagesPanel::CPGenerate)
    EVT_BUTTON     ( XRCID("images_optimize"), ImagesPanel::OnOptimizeButton)
    EVT_BUTTON     ( XRCID("images_photo_optimize"), ImagesPanel::OnPhotometricOptimizeButton)
END_EVENT_TABLE()

ImagesPanel::ImagesPanel()
{
    m_pano = 0;
    m_guiLevel=GUI_SIMPLE;
}

bool ImagesPanel::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                      long style, const wxString& name)
{
    if (! wxPanel::Create(parent, id, pos, size, style, name)) {
        return false;
    }

    wxXmlResource::Get()->LoadPanel(this, wxT("images_panel"));
    wxPanel * panel = XRCCTRL(*this, "images_panel", wxPanel);
    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
    topsizer->Add(panel, 1, wxEXPAND, 0);
    SetSizer(topsizer);

    m_images_tree = XRCCTRL(*this, "images_tree_ctrl", ImagesTreeCtrl);
    DEBUG_ASSERT(m_images_tree);

    m_showImgNr = INT_MAX;

    m_matchingButton = XRCCTRL(*this, "images_feature_matching", wxButton);
    DEBUG_ASSERT(m_matchingButton);

    m_CPDetectorChoice = XRCCTRL(*this, "cpdetector_settings", wxChoice);

    // Image Preview
    m_smallImgCtrl = XRCCTRL(*this, "images_selected_image", wxStaticBitmap);
    DEBUG_ASSERT(m_smallImgCtrl);

    m_empty.Create(0,0);
    m_smallImgCtrl->SetBitmap(m_empty);

    m_lenstype = XRCCTRL(*this, "images_lens_type", wxChoice);
    DEBUG_ASSERT(m_lenstype);
    FillLensProjectionList(m_lenstype);
    m_lenstype->SetSelection(0);

    m_focallength = XRCCTRL(*this, "images_focal_length", wxTextCtrl);
    DEBUG_ASSERT(m_focallength);
    m_focallength->PushEventHandler(new TextKillFocusHandler(this));

    m_cropfactor = XRCCTRL(*this, "images_crop_factor", wxTextCtrl);
    DEBUG_ASSERT(m_cropfactor);
    m_cropfactor->PushEventHandler(new TextKillFocusHandler(this));

    m_overlap = XRCCTRL(*this, "images_overlap", wxTextCtrl);
    DEBUG_ASSERT(m_overlap);
    m_overlap->PushEventHandler(new TextKillFocusHandler(this));

    m_maxEv = XRCCTRL(*this, "images_maxev", wxTextCtrl);
    DEBUG_ASSERT(m_maxEv);
    m_maxEv->PushEventHandler(new TextKillFocusHandler(this));

    FillGroupChoice();

    wxTreeEvent ev;
    OnSelectionChanged(ev);
    DEBUG_TRACE("end");

    m_optChoice = XRCCTRL(*this, "images_optimize_mode", wxChoice);
    DEBUG_ASSERT(m_optChoice);

    m_optPhotoChoice = XRCCTRL(*this, "images_photo_optimize_mode", wxChoice);
    DEBUG_ASSERT(m_optPhotoChoice);

    FillOptimizerChoice();

    wxConfigBase* config=wxConfigBase::Get();
    m_degDigits = config->Read(wxT("/General/DegreeFractionalDigitsEdit"),3);
    //read autopano generator settings
    cpdetector_config.Read(config,huginApp::Get()->GetDataPath()+wxT("default.setting"));
    //write current autopano generator settings
    cpdetector_config.Write(config);
    config->Flush();
    cpdetector_config.FillControl(m_CPDetectorChoice,true);
    Layout();

    return true;
}

void ImagesPanel::Init(Panorama * panorama)
{
    m_pano = panorama;
    m_images_tree->Init(m_pano);
    // observe the panorama
    m_pano->addObserver(this);
}

void DeleteClientData(wxChoice* cb)
{
    if(cb->HasClientUntypedData())
    {
        for(size_t i = 0; i < cb->GetCount(); i++)
        {
            delete static_cast<int*>(cb->GetClientData(i));
        };
    };
};

ImagesPanel::~ImagesPanel()
{
    DEBUG_TRACE("dtor");
    m_focallength->PopEventHandler(true);
    m_cropfactor->PopEventHandler(true);
    m_overlap->PopEventHandler(true);
    m_maxEv->PopEventHandler(true);
    m_pano->removeObserver(this);
    wxChoice* group=XRCCTRL(*this,"images_group_mode", wxChoice);
    size_t sel=group->GetSelection();
    DeleteClientData(group);
    DeleteClientData(m_optChoice);
    DeleteClientData(m_optPhotoChoice);
    DEBUG_TRACE("dtor end");
}

// We need to override the default handling of size events because the
// sizers set the virtual size but not the actual size. We reverse
// the standard handling and fit the child to the parent rather than
// fitting the parent around the child

void ImagesPanel::OnSize( wxSizeEvent & e )
{
    int winWidth, winHeight;
    GetClientSize(&winWidth, &winHeight);
    DEBUG_INFO( "image panel: " << winWidth <<"x"<< winHeight );
    UpdatePreviewImage();

    e.Skip();
}

void ImagesPanel::panoramaChanged(PT::Panorama & pano)
{
    //update optimizer choice selection
    int optSwitch=m_pano->getOptimizerSwitch();
    int found=wxNOT_FOUND;
    for(size_t i=0;i<m_optChoice->GetCount();i++)
    {
        if(optSwitch==*static_cast<int*>(m_optChoice->GetClientData(i)))
        {
            found=i;
            break;
        };
    };
    if(found==wxNOT_FOUND)
    {
        GlobalCmdHist::getInstance().addCommand(
            new PT::UpdateOptimizerSwitchCmd(*m_pano, 0)
        );
    }
    else
    {
        m_optChoice->SetSelection(found);
    };

    //update photometric optimizer choice selection
    optSwitch=m_pano->getPhotometricOptimizerSwitch();
    found=wxNOT_FOUND;
    for(size_t i=0;i<m_optPhotoChoice->GetCount();i++)
    {
        if(optSwitch==*static_cast<int*>(m_optPhotoChoice->GetClientData(i)))
        {
            found=i;
            break;
        };
    };
    if(found==wxNOT_FOUND)
    {
        GlobalCmdHist::getInstance().addCommand(
            new PT::UpdatePhotometricOptimizerSwitchCmd(*m_pano, 0)
        );
    }
    else
    {
        m_optPhotoChoice->SetSelection(found);
    };
    const PanoramaOptions opts=m_pano->getOptions();
    m_overlap->SetValue(hugin_utils::doubleTowxString(opts.outputStacksMinOverlap,3));
    m_maxEv->SetValue(hugin_utils::doubleTowxString(opts.outputLayersExposureDiff,2));
}

void ImagesPanel::panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & _imgNr)
{
    DEBUG_TRACE("");

    // update text field if selected
    const UIntSet & selected = m_images_tree->GetSelectedImages();
    DEBUG_DEBUG("nr of sel Images: " << selected.size());
    if (pano.getNrOfImages() == 0)
    {
        DisableImageCtrls();
        m_matchingButton->Disable();
    }
    else
    {
        m_matchingButton->Enable();
        wxTreeEvent ev;
        OnSelectionChanged(ev);
    };
    //enable/disable optimize buttons
    XRCCTRL(*this, "images_optimize", wxButton)->Enable(pano.getNrOfImages()>0);
    XRCCTRL(*this, "images_photo_optimize", wxButton)->Enable(pano.getNrOfImages()>1);
}

// #####  Here start the eventhandlers  #####

/** run control point detector on selected images, and add control points */
void ImagesPanel::CPGenerate(wxCommandEvent & e)
{
    UIntSet selImg = m_images_tree->GetSelectedImages();
    //if only one image is selected, run detector on all images, except for linefind
    wxString progName=cpdetector_config.settings[m_CPDetectorChoice->GetSelection()].GetProg().Lower();
    if ((selImg.size()==0) || (selImg.size()==1 && progName.Find(wxT("linefind"))==wxNOT_FOUND))
    {
        // add all images.
        selImg.clear();
        fill_set(selImg,0,m_pano->getNrOfImages()-1);
    }

    if (selImg.size() == 0)
    {
        return;
    }

    wxConfigBase* config=wxConfigBase::Get();
    long nFeatures = HUGIN_ASS_NCONTROLPOINTS;
#if wxCHECK_VERSION(2,9,4)
    if(wxGetKeyState(WXK_COMMAND))
#else
    if(wxGetKeyState(WXK_CONTROL))
#endif
    {
        nFeatures = config->Read(wxT("/MainFrame/nControlPoints"), HUGIN_ASS_NCONTROLPOINTS);
        nFeatures = wxGetNumberFromUser(
                            _("Enter maximal number of control points per image pair"),
                            _("Points per Overlap"),
                            _("Control point detector option"), 
                            nFeatures, 1, 10000
                                 );
        if(nFeatures<1)
        {
            return;
        };
        config->Write(wxT("/MainFrame/nControlPoints"), nFeatures);
    }
    else
    {
        nFeatures = config->Read(wxT("/Assistant/nControlPoints"), HUGIN_ASS_NCONTROLPOINTS);
    };

    AutoCtrlPointCreator matcher;
    CPVector cps = matcher.automatch(cpdetector_config.settings[m_CPDetectorChoice->GetSelection()],
        *m_pano, selImg, nFeatures,this);
    wxString msg;
    wxMessageBox(wxString::Format(_("Added %lu control points"), (unsigned long) cps.size()), _("Control point detector result"),wxOK|wxICON_INFORMATION,this);
    GlobalCmdHist::getInstance().addCommand(
            new PT::AddCtrlPointsCmd(*m_pano, cps)
                                           );

};

void ImagesPanel::OnSelectionChanged(wxTreeEvent & e)
{
    const UIntSet & sel = m_images_tree->GetSelectedImages();
    DEBUG_DEBUG("selected Images: " << sel.size());
    if (sel.size() == 0)
    {
        // nothing to edit
        DisableImageCtrls();
    }
    else
    {
        // enable edit
        EnableImageCtrls();
        const SrcPanoImage& img=m_pano->getImage(*sel.begin());
        bool identical_projection=true;
        SrcPanoImage::Projection proj=img.getProjection();
        double focallength=SrcPanoImage::calcFocalLength(img.getProjection(),img.getHFOV(),
                img.getExifCropFactor(),img.getSize());;
        double cropFactor=img.getExifCropFactor();
        for(UIntSet::const_iterator it=sel.begin(); it!=sel.end(); it++)
        {
            const SrcPanoImage& img2=m_pano->getImage(*it);
            if(proj!=img2.getProjection())
            {
                identical_projection=false;
            };
            double focallength2 = SrcPanoImage::calcFocalLength(img2.getProjection(),img2.getHFOV(),
                img2.getExifCropFactor(),img2.getSize());
            if(focallength>0 && fabs(focallength-focallength2)>0.05)
            {
                focallength=-1;
            };
            if(fabs(cropFactor-img2.getExifCropFactor())>0.1)
            {
                cropFactor=-1;
            };
        };

        if(identical_projection)
        {
            SelectProjection(m_lenstype, proj);
        }
        else
        {
            m_lenstype->Select(wxNOT_FOUND);
        };
        if(focallength>0)
        {
            m_focallength->SetValue(hugin_utils::doubleTowxString(focallength,m_degDigits));
        }
        else
        {
            m_focallength->Clear();
        };
        if(cropFactor>0)
        {
            m_cropfactor->SetValue(doubleTowxString(cropFactor,m_degDigits));
        }
        else
        {
            m_cropfactor->Clear();
        };

        if (sel.size() == 1)
        {
            ShowImage(*(sel.begin()));
        }
        else
        {
            m_smallImgCtrl->SetBitmap(m_empty);
            m_smallImgCtrl->GetParent()->Layout();
            m_smallImgCtrl->Refresh();
        };
    }
}

void ImagesPanel::DisableImageCtrls()
{
    // disable controls
    m_lenstype->Disable();
    m_focallength->Disable();
    m_cropfactor->Disable();
    m_smallImgCtrl->SetBitmap(m_empty);
    m_smallImgCtrl->GetParent()->Layout();
    m_smallImgCtrl->Refresh();
}

void ImagesPanel::EnableImageCtrls()
{
    // enable control if not already enabled
    m_lenstype->Enable();
    m_focallength->Enable();
    m_cropfactor->Enable();
}

void ImagesPanel::ShowImage(unsigned int imgNr)
{
    m_showImgNr = imgNr;
    UpdatePreviewImage();
}

void ImagesPanel::UpdatePreviewImage()
{
    if (m_showImgNr < 0 || m_showImgNr >= m_pano->getNrOfImages())
    {
        return;
    }
    ImageCache::EntryPtr cacheEntry = ImageCache::getInstance().getSmallImageIfAvailable(
            m_pano->getImage(m_showImgNr).getFilename());
    if (!cacheEntry.get())
    {
        // image currently isn't loaded.
        // Instead of loading and displaying the image now, request it for
        // later. Then the user can switch between images in the list quickly,
        // even when not all images previews are in the cache.
        thumbnail_request = ImageCache::getInstance().requestAsyncSmallImage(
                m_pano->getImage(m_showImgNr).getFilename());
        // When the image is ready, try this function again.
        thumbnail_request->ready.connect(
            boost::bind(&ImagesPanel::UpdatePreviewImage, this));
    } else {
        // forget any request now the image has loaded.
        thumbnail_request = ImageCache::RequestPtr();
        wxImage img = imageCacheEntry2wxImage(cacheEntry); 

        double iRatio = img.GetWidth() / (double) img.GetHeight();

        wxSize sz;
        // estimate image size
        
        sz = m_smallImgCtrl->GetContainingSizer()->GetSize();
        double sRatio = (double)sz.GetWidth() / sz.GetHeight();
        if (iRatio > sRatio) {
            // image is wider than screen, display landscape
            sz.SetHeight((int) (sz.GetWidth() / iRatio));
        } else {
            // portrait
            sz.SetWidth((int) (sz.GetHeight() * iRatio));
        }
        // Make sure the size is positive:
        // on a small window, m_smallImgCtrl can have 0 width.
        sz.IncTo(wxSize(1,1));
        wxImage scaled = img.Scale(sz.GetWidth(),sz.GetHeight());
        m_smallImgCtrl->SetBitmap(wxBitmap(scaled));
        m_smallImgCtrl->GetParent()->Layout();
        m_smallImgCtrl->Refresh();
    }
}

void ImagesPanel::ReloadCPDetectorSettings()
{
    cpdetector_config.Read(); 
    cpdetector_config.FillControl(m_CPDetectorChoice,true);
    m_CPDetectorChoice->InvalidateBestSize();
    m_CPDetectorChoice->GetParent()->Layout();
    Refresh();
};

void ImagesPanel::OnLensTypeChanged (wxCommandEvent & e)
{
    size_t var = GetSelectedProjection(m_lenstype);
    UIntSet images=m_images_tree->GetSelectedImages();
    if(images.size()>0)
    {
        const SrcPanoImage & img=m_pano->getImage(*(images.begin()));
        double focal_length = SrcPanoImage::calcFocalLength(img.getProjection(),img.getHFOV(),img.getExifCropFactor(),img.getSize());
        std::vector<PanoCommand*> commands;
        commands.push_back(new PT::ChangeImageProjectionCmd(*m_pano, images,(HuginBase::SrcPanoImage::Projection) var));
        commands.push_back(new PT::UpdateFocalLengthCmd(*m_pano, images, focal_length));
        GlobalCmdHist::getInstance().addCommand(
            new PT::CombinedPanoCommand(*m_pano, commands)
        );
    };
};

void ImagesPanel::OnFocalLengthChanged(wxCommandEvent & e)
{
    if (m_pano->getNrOfImages() == 0)
    {
        return;
    };

    wxString text = m_focallength->GetValue();
    if(text.IsEmpty())
    {
        return;
    };
    double val;
    if (!str2double(text, val))
    {
        return;
    }
    //no negative values, no zero input please
    if (val<0.1)
    {
        wxBell();
        return;
    };
    
    UIntSet images=m_images_tree->GetSelectedImages();
    const SrcPanoImage& srcImg=m_pano->getImage(*(images.begin()));
    if(srcImg.getProjection()==SrcPanoImage::FISHEYE_ORTHOGRAPHIC)
    {
        double hfov=srcImg.calcHFOV(srcImg.getProjection(), val, srcImg.getExifCropFactor(), srcImg.getSize());
        if(hfov>190)
        {
            if(wxMessageBox(
                wxString::Format(_("You have given a field of view of %.2f degrees.\n But the orthographic projection is limited to a field of view of 180 degress.\nDo you want still use that high value?"), hfov),
#ifdef __WXMSW__
                _("Hugin"),
#else
                wxT(""),
#endif
                wxICON_EXCLAMATION | wxYES_NO)==wxNO)
            {
                wxTreeEvent dummy;
                OnSelectionChanged(dummy);
                return;
            };
        };
    };
    GlobalCmdHist::getInstance().addCommand(
        new PT::UpdateFocalLengthCmd(*m_pano, images, val)
    );
}

void ImagesPanel::OnCropFactorChanged(wxCommandEvent & e)
{
    if (m_pano->getNrOfImages() == 0)
    {
        return;
    };

    wxString text = m_cropfactor->GetValue();
    if(text.IsEmpty())
    {
        return;
    };
    double val;
    if (!str2double(text, val))
    {
        return;
    }
    //no negative values, no zero input please
    if (val<0.1)
    {
        wxBell();
        return;
    };

    UIntSet images=m_images_tree->GetSelectedImages();
    GlobalCmdHist::getInstance().addCommand(
        new PT::UpdateCropFactorCmd(*m_pano,images,val)
    );
}

void ImagesPanel::OnMinimumOverlapChanged(wxCommandEvent & e)
{
    wxString text = m_overlap->GetValue();
    if(text.IsEmpty())
    {
        return;
    };
    double val;
    if (!str2double(text, val))
    {
        return;
    }
    if(val<=0 || val>1)
    {
        wxMessageBox(_("The minimum overlap has to be greater than 0 and smaller than 1."),
#ifdef _WINDOWS
            _("Hugin"),
#else
            wxT(""),
#endif
            wxOK | wxICON_INFORMATION, this);
        return;
    };
    PanoramaOptions opt=m_pano->getOptions();
    opt.outputStacksMinOverlap=val;
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd(*m_pano,opt)
    );
};

void ImagesPanel::OnMaxEvDiffChanged(wxCommandEvent& e)
{
    wxString text = m_maxEv->GetValue();
    if(text.IsEmpty())
    {
        return;
    };
    double val;
    if (!str2double(text, val))
    {
        return;
    }
    if(val<0)
    {
        wxMessageBox(_("The maximum Ev difference has to be greater than 0."),
#ifdef _WINDOWS
            _("Hugin"),
#else
            wxT(""),
#endif
            wxOK | wxICON_INFORMATION, this);
        return;
    };
    PanoramaOptions opt=m_pano->getOptions();
    opt.outputLayersExposureDiff=val;
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd(*m_pano,opt)
    );
};

void ImagesPanel::FillGroupChoice()
{
    wxChoice* group=XRCCTRL(*this,"images_group_mode", wxChoice);
    size_t sel=group->GetSelection();
    DeleteClientData(group);
    group->Clear();
    int* i=new int;
    *i=ImagesTreeCtrl::GROUP_NONE;
    group->Append(_("None"), i);
    i=new int;
    *i=ImagesTreeCtrl::GROUP_LENS;
    group->Append(_("Lens"), i);
    if(m_guiLevel>GUI_SIMPLE)
    {
        i=new int;
        *i=ImagesTreeCtrl::GROUP_STACK;
        group->Append(_("Stacks"), i);
        if(m_guiLevel==GUI_EXPERT)
        {
            i=new int;
            *i=ImagesTreeCtrl::GROUP_OUTPUTLAYERS;
            group->Append(_("Output layers"), i);
            i=new int;
            *i=ImagesTreeCtrl::GROUP_OUTPUTSTACK;
            group->Append(_("Output stacks"), i);
        };
    };
    if((m_guiLevel==GUI_ADVANCED && sel>2) || (m_guiLevel==GUI_SIMPLE && sel>1))
    {
        sel=0;
    };
    group->SetSelection(sel);
    wxCommandEvent dummy;
    OnGroupModeChanged(dummy);
};

void ImagesPanel::FillOptimizerChoice()
{
    DeleteClientData(m_optChoice);
    m_optChoice->Clear();
    int* i=new int;
    *i=HuginBase::OPT_PAIR;
    m_optChoice->Append(_("Positions (incremental, starting from anchor)"), i);
    i=new int;
    *i=HuginBase::OPT_POSITION;
    m_optChoice->Append(_("Positions (y,p,r)"), i);
    i=new int;
    *i=(HuginBase::OPT_POSITION | HuginBase::OPT_VIEW);
    m_optChoice->Append(_("Positions and View (y,p,r,v)"), i);
    i=new int;
    *i=(HuginBase::OPT_POSITION | HuginBase::OPT_BARREL);
    m_optChoice->Append(_("Positions and Barrel Distortion (y,p,r,b)"), i);
    i=new int;
    *i=(HuginBase::OPT_POSITION | HuginBase::OPT_VIEW | HuginBase::OPT_BARREL);
    m_optChoice->Append(_("Positions, View and Barrel (y,p,r,v,b)"), i);
    i=new int;
    *i=HuginBase::OPT_ALL;
    if(m_guiLevel==GUI_EXPERT)
    {
        m_optChoice->Append(_("Everything without translation"), i);
    }
    else
    {
        m_optChoice->Append(_("Everything"), i);
    };
    if(m_guiLevel==GUI_EXPERT)
    {
        i=new int;
        *i=(HuginBase::OPT_POSITION | HuginBase::OPT_TRANSLATION);
        m_optChoice->Append(_("Positions and Translation (y,p,r,x,y,z)"), i);
        i=new int;
        *i=(HuginBase::OPT_POSITION | HuginBase::OPT_TRANSLATION | HuginBase::OPT_VIEW);
        m_optChoice->Append(_("Positions, Translation and View (y,p,r,x,y,z,v)"), i);
        i=new int;
        *i=(HuginBase::OPT_POSITION | HuginBase::OPT_TRANSLATION | HuginBase::OPT_BARREL);
        m_optChoice->Append(_("Positions, Translation and Barrel (y,p,r,x,y,z,b)"), i);
        i=new int;
        *i=(HuginBase::OPT_POSITION | HuginBase::OPT_TRANSLATION | HuginBase::OPT_VIEW | HuginBase::OPT_BARREL);
        m_optChoice->Append(_("Positions, Translation, View and Barrel (y,p,r,x,y,z,v,b)"), i);
    };
    i=new int;
    *i=0;
    m_optChoice->Append(_("Custom parameters"), i);

    DeleteClientData(m_optPhotoChoice);
    m_optPhotoChoice->Clear();
    i=new int;
    *i=(HuginBase::OPT_EXPOSURE | HuginBase::OPT_VIGNETTING | HuginBase::OPT_RESPONSE);
    m_optPhotoChoice->Append(_("Low dynamic range"), i);
    i=new int;
    *i=(HuginBase::OPT_EXPOSURE | HuginBase::OPT_VIGNETTING | HuginBase::OPT_RESPONSE | HuginBase::OPT_WHITEBALANCE);
    m_optPhotoChoice->Append(_("Low dynamic range, variable white balance"), i);
    if(m_guiLevel>GUI_SIMPLE)
    {
        i=new int;
        *i=(HuginBase::OPT_VIGNETTING | HuginBase::OPT_RESPONSE);
        m_optPhotoChoice->Append(_("High dynamic range, fixed exposure"), i);
        i=new int;
        *i=(HuginBase::OPT_WHITEBALANCE | HuginBase::OPT_VIGNETTING | HuginBase::OPT_RESPONSE);
        m_optPhotoChoice->Append(_("High dynamic range, variable white balance, fixed exposure"), i);
    };
    i=new int;
    *i=0;
    m_optPhotoChoice->Append(_("Custom parameters"), i);
    m_optChoice->GetParent()->Layout();
    Refresh();
};

void ImagesPanel::OnGroupModeChanged(wxCommandEvent & e)
{
    wxChoice* group=XRCCTRL(*this,"images_group_mode", wxChoice);
    ImagesTreeCtrl::GroupMode mode=ImagesTreeCtrl::GroupMode(*static_cast<int*>(group->GetClientData(group->GetSelection())));
    m_images_tree->SetGroupMode(mode);
    XRCCTRL(*this, "images_text_overlap", wxStaticText)->Show(mode==ImagesTreeCtrl::GROUP_OUTPUTSTACK);
    m_overlap->Show(mode==ImagesTreeCtrl::GROUP_OUTPUTSTACK);
    m_overlap->Enable(mode==ImagesTreeCtrl::GROUP_OUTPUTSTACK);
    XRCCTRL(*this, "images_text_maxev", wxStaticText)->Show(mode==ImagesTreeCtrl::GROUP_OUTPUTLAYERS);
    m_maxEv->Show(mode==ImagesTreeCtrl::GROUP_OUTPUTLAYERS);
    m_maxEv->Enable(mode==ImagesTreeCtrl::GROUP_OUTPUTLAYERS);
    Layout();
    Refresh();
};

void ImagesPanel::OnDisplayModeChanged(wxCommandEvent & e)
{
    wxRadioBox* display=XRCCTRL(*this,"images_column_radiobox", wxRadioBox);
    m_images_tree->SetDisplayMode((ImagesTreeCtrl::DisplayMode)display->GetSelection());
};

void ImagesPanel::OnOptimizerSwitchChanged(wxCommandEvent &e)
{
    int optSwitch=*static_cast<int*>(m_optChoice->GetClientData(m_optChoice->GetSelection()));
    if(optSwitch!=m_pano->getOptimizerSwitch())
    {
        GlobalCmdHist::getInstance().addCommand(
            new PT::UpdateOptimizerSwitchCmd(*m_pano,optSwitch)
        );
    };
};

void ImagesPanel::OnPhotometricOptimizerSwitchChanged(wxCommandEvent &e)
{
    int optSwitch=*static_cast<int*>(m_optPhotoChoice->GetClientData(m_optPhotoChoice->GetSelection()));
    if(optSwitch!=m_pano->getPhotometricOptimizerSwitch())
    {
        GlobalCmdHist::getInstance().addCommand(
            new PT::UpdatePhotometricOptimizerSwitchCmd(*m_pano,optSwitch)
        );
    };
};

void ImagesPanel::SetGuiLevel(GuiLevel newGuiLevel)
{
    m_guiLevel=newGuiLevel;
    m_images_tree->SetGuiLevel(newGuiLevel);
    FillGroupChoice();
    FillOptimizerChoice();
    wxStaticText* textlabel=XRCCTRL(*this, "images_mode_text", wxStaticText);
    switch(m_guiLevel)
    {
        case GUI_SIMPLE:
            textlabel->SetLabel(_("Simple interface"));
            break;
        case GUI_ADVANCED:
            textlabel->SetLabel(_("Advanced interface"));
            break;
        case GUI_EXPERT:
            textlabel->SetLabel(_("Expert interface"));
            break;
    };
    textlabel->GetParent()->Layout();
    textlabel->Refresh();
    panoramaChanged(*m_pano);
};

void ImagesPanel::OnOptimizeButton(wxCommandEvent &e)
{
    MainFrame::Get()->OnOptimize(e);
};

void ImagesPanel::OnPhotometricOptimizeButton(wxCommandEvent &e)
{
    MainFrame::Get()->OnPhotometricOptimize(e);
};

IMPLEMENT_DYNAMIC_CLASS(ImagesPanel, wxPanel)

ImagesPanelXmlHandler::ImagesPanelXmlHandler()
                : wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *ImagesPanelXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, ImagesPanel)

    cp->Create(m_parentAsWindow,
                   GetID(),
                   GetPosition(), GetSize(),
                   GetStyle(wxT("style")),
                   GetName());

    SetupWindow( cp);
    return cp;
}

bool ImagesPanelXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("ImagesPanel"));
}

IMPLEMENT_DYNAMIC_CLASS(ImagesPanelXmlHandler, wxXmlResourceHandler)
