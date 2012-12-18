// -*- c-basic-offset: 4 -*-

/** @file OptimizePhotometricPanel.cpp
 *
 *  @brief implementation of OptimizePhotometricPanel
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 */

/*  This program is free software; you can redistribute it and/or
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

#include <PT/RandomPointSampler.h>
#include <PT/PhotometricOptimizer.h>
#include <PT/PTOptimise.h>

#include "hugin/OptimizePhotometricPanel.h"
#include "hugin/CommandHistory.h"
#include "hugin/MainFrame.h"
#include "base_wx/MyProgressDialog.h"
#include "hugin/config_defaults.h"
#include "base_wx/wxImageCache.h"
#include "hugin/ImagesTree.h"
#include "hugin_base/panodata/OptimizerSwitches.h"
#include "hugin/PanoOperation.h"

using namespace std;
using namespace PT;
using namespace PTools;
using namespace hugin_utils;
using namespace vigra;
using namespace vigra_ext;

//============================================================================
//============================================================================
//============================================================================

BEGIN_EVENT_TABLE(OptimizePhotometricPanel, wxPanel)
    EVT_CLOSE(OptimizePhotometricPanel::OnClose)
    EVT_BUTTON(XRCID("optimize_photo_panel_optimize"), OptimizePhotometricPanel::OnOptimizeButton)
    EVT_BUTTON(XRCID("optimize_photo_panel_reset"), OptimizePhotometricPanel::OnReset)
END_EVENT_TABLE()

OptimizePhotometricPanel::OptimizePhotometricPanel() : m_pano(0)
{
};

bool OptimizePhotometricPanel::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                      long style, const wxString& name)
{
    DEBUG_TRACE("");
    if (! wxPanel::Create(parent, id, pos, size, style, name))
    {
        return false;
    }

    wxXmlResource::Get()->LoadPanel(this, wxT("optimize_photo_panel"));
    wxPanel * panel = XRCCTRL(*this, "optimize_photo_panel", wxPanel);

    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
    topsizer->Add(panel, 1, wxEXPAND, 0);
    SetSizer(topsizer);

    m_only_active_images_cb = XRCCTRL(*this, "optimize_photo_panel_only_active_images", wxCheckBox);
    DEBUG_ASSERT(m_only_active_images_cb);
    m_only_active_images_cb->SetValue(wxConfigBase::Get()->Read(wxT("/OptimizeOptimizePhotometricPanelPanel/OnlyActiveImages"),1l) != 0);

    m_images_tree = XRCCTRL(*this, "optimize_photo_panel_images", ImagesTreeCtrl);
    DEBUG_ASSERT(m_images_tree);
    m_lens_tree = XRCCTRL(*this, "optimize_photo_panel_lens", ImagesTreeCtrl);
    DEBUG_ASSERT(m_lens_tree);

    XRCCTRL(*this, "optimize_photo_panel_splitter", wxSplitterWindow)->SetSashGravity(0.66);

    wxString text(_("Any variables below which are bold and underlined will be optimized."));
    text.Append(wxT(" "));
#if defined __WXMAC__ || defined __WXOSX_COCOA__
    text.Append(_("Use command + left mouse click to toggle state of variables."));
#else
    text.Append(_("Use control + left mouse click to toggle state of variables."));
#endif
    text.Append(wxT(" "));
    text.Append(_("Variables which shown in normal font will act as references or anchors."));
    XRCCTRL(*this, "optimize_photo_panel_information_text", wxStaticText)->SetLabel(text);

    return true;
}

void OptimizePhotometricPanel::Init(Panorama * panorama)
{
    m_pano = panorama;
    // observe the panorama
    m_pano->addObserver(this);
    m_images_tree->Init(m_pano);
    m_images_tree->SetOptimizerMode();
    m_images_tree->SetDisplayMode(ImagesTreeCtrl::DISPLAY_PHOTOMETRICS_IMAGES);

    m_lens_tree->Init(m_pano);
    m_lens_tree->SetOptimizerMode();
    m_lens_tree->SetGroupMode(ImagesTreeCtrl::GROUP_LENS);
    m_lens_tree->SetDisplayMode(ImagesTreeCtrl::DISPLAY_PHOTOMETRICS_LENSES);
    
}

void OptimizePhotometricPanel::SetGuiLevel(GuiLevel newGuiLevel)
{
    m_images_tree->SetGuiLevel(newGuiLevel);
    m_lens_tree->SetGuiLevel(newGuiLevel);
};

OptimizePhotometricPanel::~OptimizePhotometricPanel()
{
    DEBUG_TRACE("dtor, writing config");
    wxConfigBase::Get()->Write(wxT("/OptimizePhotometricPanel/OnlyActiveImages"),m_only_active_images_cb->IsChecked() ? 1l : 0l);

    m_pano->removeObserver(this);
    DEBUG_TRACE("dtor end");
}

void OptimizePhotometricPanel::OnOptimizeButton(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    // run optimizer

    UIntSet imgs;
    if (m_only_active_images_cb->IsChecked() || m_pano->getPhotometricOptimizerSwitch()!=0)
    {
        // use only selected images.
        imgs = m_pano->getActiveImages();
        if (imgs.size() < 2)
        {
            wxMessageBox(_("The project does not contain any active images.\nPlease activate at least 2 images in the (fast) preview window.\nOptimization canceled."),
#ifdef _WINDOWS
                _("Hugin"),
#else
                wxT(""),
#endif
                wxICON_ERROR | wxOK);
            return;
        } 
    }
    else
    {
        fill_set(imgs, 0, m_pano->getNrOfImages()-1);
    }
    runOptimizer(imgs);
}

void OptimizePhotometricPanel::panoramaChanged(PT::Panorama & pano)
{
    m_images_tree->Enable(m_pano->getPhotometricOptimizerSwitch()==0);
    m_lens_tree->Enable(m_pano->getPhotometricOptimizerSwitch()==0);
}

void OptimizePhotometricPanel::panoramaImagesChanged(PT::Panorama &pano,
                                          const PT::UIntSet & imgNr)
{
    XRCCTRL(*this, "optimize_photo_panel_optimize", wxButton)->Enable(pano.getNrOfImages()>1);
    XRCCTRL(*this, "optimize_photo_panel_reset", wxButton)->Enable(pano.getNrOfImages()>0);    
}

void OptimizePhotometricPanel::runOptimizer(const UIntSet & imgs)
{
    DEBUG_TRACE("");
    int mode = m_pano->getPhotometricOptimizerSwitch();

    // check if vignetting and response are linked, display a warning if they are not
    // The variables to check:
    const HuginBase::ImageVariableGroup::ImageVariableEnum vars[] = {
            HuginBase::ImageVariableGroup::IVE_EMoRParams,
            HuginBase::ImageVariableGroup::IVE_ResponseType,
            HuginBase::ImageVariableGroup::IVE_VigCorrMode,
            HuginBase::ImageVariableGroup::IVE_RadialVigCorrCoeff,
            HuginBase::ImageVariableGroup::IVE_RadialVigCorrCenterShift
        };
    // keep a list of commands needed to fix it:
    std::vector<PT::PanoCommand *> commands;
    HuginBase::ConstStandardImageVariableGroups variable_groups(*m_pano);
    HuginBase::ConstImageVariableGroup & lenses = variable_groups.getLenses();
    for (size_t i = 0; i < lenses.getNumberOfParts(); i++)
    {
        std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> links_needed;
        links_needed.clear();
        for (int v = 0; v < 5; v++)
        {
            if (!lenses.getVarLinkedInPart(vars[v], i))
            {
                links_needed.insert(vars[v]);
            }
        };
        if (!links_needed.empty())
        {
            commands.push_back(new PT::LinkLensVarsCmd(*m_pano, i, links_needed));
        }
    }
    // if the list of commands is empty, all is good and we don't need a warning.
    if (!commands.empty()) {
        int ok = wxMessageBox(_("The same vignetting and response parameters should\nbe applied for all images of a lens.\nCurrently each image can have different parameters.\nLink parameters?"), _("Link parameters"), wxYES_NO | wxICON_INFORMATION);
        if (ok == wxYES)
        {
            // perform all the commands we stocked up earilier.
            for (std::vector<PT::PanoCommand *>::iterator it = commands.begin(); it != commands.end(); it++)
            {
                GlobalCmdHist::getInstance().addCommand(*it);
            }
        }
        else
        {
            // free all the commands, the user doesn't want them used.
            for (std::vector<PT::PanoCommand *>::iterator it = commands.begin(); it != commands.end(); it++)
            {
                delete *it;
            }
        }
    }

    Panorama optPano = m_pano->getSubset(imgs);
    PanoramaOptions opts = optPano.getOptions();

    OptimizeVector optvars;
    if(mode==0)
    {
        optvars = optPano.getOptimizeVector();
        bool valid=false;
        for(unsigned int i=0;i<optvars.size() && !valid;i++)
        {
            if(set_contains(optvars[i], "Eev") || set_contains(optvars[i], "Er") || set_contains(optvars[i], "Eb") ||
                set_contains(optvars[i], "Vb") || set_contains(optvars[i], "Vx") || set_contains(optvars[i], "Ra"))
            {
                valid=true;
            };
        };
        if(!valid)
        {
            wxMessageBox(_("You selected no parameters to optimize.\nTherefore optimization will be canceled."), _("Exposure optimization"), wxOK | wxICON_INFORMATION);
            return;
        };
    };

    double error = 0;
    {
        std::vector<vigra_ext::PointPairRGB> points;
        long nPoints = 200;
        wxConfigBase::Get()->Read(wxT("/OptimizePhotometric/nRandomPointsPerImage"), &nPoints , HUGIN_PHOTOMETRIC_OPTIMIZER_NRPOINTS);

        ProgressReporterDialog progress(5.0, _("Photometric alignment"), _("Loading images"));
        progress.Show();

        nPoints = nPoints * optPano.getNrOfImages();
        // get the small images
        std::vector<vigra::FRGBImage *> srcImgs;
        for (size_t i=0; i < optPano.getNrOfImages(); i++)
        {
            ImageCache::EntryPtr e = ImageCache::getInstance().getSmallImage(optPano.getImage(i).getFilename());
            vigra::FRGBImage * img = new FRGBImage;
            if (!e)
            {
                wxMessageBox(_("Error: could not load all images"), _("Error"));
                return;
            }
            if (e->image8 && e->image8->width() > 0)
            {
                reduceToNextLevel(*(e->image8), *img);
                transformImage(vigra::srcImageRange(*img), vigra::destImage(*img),
                                vigra::functor::Arg1()/vigra::functor::Param(255.0));
            }
            else
            {
                if (e->image16 && e->image16->width() > 0)
                {
                    reduceToNextLevel(*(e->image16), *img);
                    transformImage(vigra::srcImageRange(*img), vigra::destImage(*img),
                                   vigra::functor::Arg1()/vigra::functor::Param(65535.0));
                }
                else
                {
                    reduceToNextLevel(*(e->imageFloat), *img);
                }
            };
            srcImgs.push_back(img);
        }
        bool randomPoints = true;
        extractPoints(optPano, srcImgs, nPoints, randomPoints, progress, points);

        if (points.size() == 0)
        {
            wxMessageBox(_("Error: no overlapping points found, Photometric optimization aborted"), _("Error"));
            return;
        }

        try
        {
            if (mode != 0)
            {
                // run automatic optimisation
                // ensure that we have a valid anchor.
                PanoramaOptions opts = optPano.getOptions();
                if (opts.colorReferenceImage >= optPano.getNrOfImages())
                {
                    opts.colorReferenceImage = 0;
                    optPano.setOptions(opts);
                }
                PhotometricOptimizeMode optMode;
                switch(mode)
                {
                    case (HuginBase::OPT_EXPOSURE | HuginBase::OPT_VIGNETTING | HuginBase::OPT_RESPONSE):
                        optMode=OPT_PHOTOMETRIC_LDR;
                        break;
                    case (HuginBase::OPT_EXPOSURE | HuginBase::OPT_VIGNETTING | HuginBase::OPT_RESPONSE | HuginBase::OPT_WHITEBALANCE):
                        optMode=OPT_PHOTOMETRIC_LDR_WB;
                        break;
                    case (HuginBase::OPT_VIGNETTING | HuginBase::OPT_RESPONSE):
                        optMode=OPT_PHOTOMETRIC_HDR;
                        break;
                    case (HuginBase::OPT_WHITEBALANCE | HuginBase::OPT_VIGNETTING | HuginBase::OPT_RESPONSE):
                        optMode=OPT_PHOTOMETRIC_HDR_WB;
                        break;
                    default:
                        //invalid combination
                        return;
                };
                smartOptimizePhotometric(optPano, optMode, points, progress, error);
            }
            else
            {
                // optimize selected parameters
                optimizePhotometric(optPano, optvars, points, progress, error);
            }
        }
        catch (std::exception & error)
        {
            wxMessageBox(_("Internal error during photometric optimization:\n") + wxString(error.what(), wxConvLocal), _("Internal error"));
        }
    }
    wxYield();

    // display information about the estimation process:
    int ret = wxMessageBox(wxString::Format(_("Photometric optimization results:\nAverage difference (RMSE) between overlapping pixels: %.2f gray values (0..255)\n\nApply results?"), error*255),
                           _("Photometric optimization finished"), wxYES_NO | wxICON_INFORMATION,this);

    if (ret == wxYES)
    {
        DEBUG_DEBUG("Applying vignetting corr");
        // TODO: merge into a single update command
        const VariableMapVector & vars = optPano.getVariables();
        GlobalCmdHist::getInstance().addCommand(
                new PT::UpdateImagesVariablesCmd(*m_pano, imgs, vars)
                                               );
        //now update panorama exposure value
        PanoramaOptions opts = m_pano->getOptions();
        opts.outputExposureValue = calcMeanExposure(*m_pano);
        GlobalCmdHist::getInstance().addCommand(
                new PT::SetPanoOptionsCmd(*m_pano, opts)
                                               );
    }
}

void OptimizePhotometricPanel::OnClose(wxCloseEvent& event)
{
    DEBUG_TRACE("OnClose");
    // do not close, just hide if we're not forced
    if (event.CanVeto())
    {
        event.Veto();
        Hide();
        DEBUG_DEBUG("Hiding");
    }
    else
    {
        DEBUG_DEBUG("Closing");
        Destroy();
    }
}

void OptimizePhotometricPanel::OnReset(wxCommandEvent& e)
{
    PanoOperation::ResetOperation op(PanoOperation::ResetOperation::RESET_DIALOG_PHOTOMETRICS);
    PT::PanoCommand* cmd=op.GetCommand(this,*m_pano, m_images_tree->GetSelectedImages());
    if(cmd!=NULL)
    {
        GlobalCmdHist::getInstance().addCommand(cmd);
    };
};

IMPLEMENT_DYNAMIC_CLASS(OptimizePhotometricPanel, wxPanel)

OptimizePhotometricPanelXmlHandler::OptimizePhotometricPanelXmlHandler()
                : wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *OptimizePhotometricPanelXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, OptimizePhotometricPanel)

    cp->Create(m_parentAsWindow,
                   GetID(),
                   GetPosition(), GetSize(),
                   GetStyle(wxT("style")),
                   GetName());

    SetupWindow( cp);

    return cp;
}

bool OptimizePhotometricPanelXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("OptimizePhotometricPanel"));
}

IMPLEMENT_DYNAMIC_CLASS(OptimizePhotometricPanelXmlHandler, wxXmlResourceHandler)
