// -*- c-basic-offset: 4 -*-

/** @file OptimizePhotometricPanel.cpp
 *
 *  @brief implementation of OptimizePhotometricPanel
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
#include <PT/RandomPointSampler.h>
#include <PT/PhotometricOptimizer.h>
#include <PT/PTOptimise.h>

#include "hugin/OptimizePhotometricPanel.h"
#include "hugin/CommandHistory.h"
#include "hugin/MainFrame.h"
#include "base_wx/MyProgressDialog.h"
#include "base_wx/PTWXDlg.h"
#include "hugin/config_defaults.h"
#include "base_wx/ImageCache.h"

using namespace std;
using namespace PT;
using namespace PTools;
using namespace utils;
using namespace vigra;
using namespace vigra_ext;

//============================================================================
//============================================================================
//============================================================================

BEGIN_EVENT_TABLE(OptimizePhotometricPanel, wxPanel)
    EVT_CLOSE(OptimizePhotometricPanel::OnClose)
    EVT_BUTTON(XRCID("optimize_photo_frame_optimize"), OptimizePhotometricPanel::OnOptimizeButton)
    EVT_BUTTON(XRCID("opt_exp_select"), OptimizePhotometricPanel::OnSelExposure)
    EVT_BUTTON(XRCID("opt_exp_clear"), OptimizePhotometricPanel::OnDelExposure)
    EVT_BUTTON(XRCID("opt_wb_select"), OptimizePhotometricPanel::OnSelWB)
    EVT_BUTTON(XRCID("opt_wb_clear"), OptimizePhotometricPanel::OnDelWB)
    EVT_CHOICE(XRCID("optimize_photo_panel_mode"), OptimizePhotometricPanel::OnChangeMode)
END_EVENT_TABLE()

// local optimize definition. need to be in sync with the xrc file
enum OptimizeMode { OPT_LDR=0, OPT_LDR_WB, OPT_HDR, OPT_HDR_WB, OPT_CUSTOM,
                    OPT_END_MARKER};


OptimizePhotometricPanel::OptimizePhotometricPanel()
    : m_pano(0)
{
}

bool OptimizePhotometricPanel::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                      long style, const wxString& name)
{
    DEBUG_TRACE("");
    if (! wxPanel::Create(parent, id, pos, size, style, name)) {
        return false;
    }

    wxXmlResource::Get()->LoadPanel(this, wxT("optimize_photo_panel"));
    wxPanel * panel = XRCCTRL(*this, "optimize_photo_panel", wxPanel);

    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
    topsizer->Add(panel, 1, wxEXPAND, 0);
    SetSizer(topsizer);

#ifdef DEBUG
    SetBackgroundColour(wxTheColourDatabase->Find(wxT("RED")));
    panel->SetBackgroundColour(wxTheColourDatabase->Find(wxT("BLUE")));
#endif

    m_only_active_images_cb = XRCCTRL(*this, "optimize_photo_only_active_images", wxCheckBox);
    DEBUG_ASSERT(m_only_active_images_cb);
    m_only_active_images_cb->SetValue(wxConfigBase::Get()->Read(wxT("/OptimizeOptimizePhotometricPanelPanel/OnlyActiveImages"),1l));

    m_vig_list = XRCCTRL(*this, "optimize_photo_vig_list", wxCheckListBox);
    DEBUG_ASSERT(m_vig_list);
    m_vigc_list = XRCCTRL(*this, "optimize_photo_vigc_list", wxCheckListBox);
    DEBUG_ASSERT(m_vigc_list);
    m_exp_list = XRCCTRL(*this, "optimize_photo_exp_list", wxCheckListBox);
    DEBUG_ASSERT(m_exp_list);
    m_wb_list = XRCCTRL(*this, "optimize_photo_wb_list", wxCheckListBox);
    DEBUG_ASSERT(m_wb_list);
    m_resp_list = XRCCTRL(*this, "optimize_photo_resp_list", wxCheckListBox);
    DEBUG_ASSERT(m_resp_list);

    m_mode_cb = XRCCTRL(*this, "optimize_photo_panel_mode", wxChoice);
    DEBUG_ASSERT(m_mode_cb);

    m_opt_ctrls = XRCCTRL(*this, "optimize_photo_controls_panel", wxScrolledWindow);
    DEBUG_ASSERT(m_opt_ctrls);
    m_opt_ctrls->SetSizeHints(20, 20);
    m_opt_ctrls->FitInside();
    m_opt_ctrls->SetScrollRate(10, 10);

    // disable the optimize panel controls by default
    XRCCTRL(*this, "optimize_photo_frame_optimize", wxButton)->Disable();
    m_mode_cb->Disable();


    return true;
}

void OptimizePhotometricPanel::Init(Panorama * panorama)
{
    m_pano = panorama;
    // observe the panorama
    m_pano->addObserver(this);

    wxCommandEvent dummy;
    dummy.SetInt(m_mode_cb->GetSelection());
    OnChangeMode(dummy);
}

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
    // take the OptimizeVector from somewhere.

    //OptimizeVector optvars = getOptimizeVector();
    //m_pano->setOptimizeVector(optvars);


    UIntSet imgs;
    if (m_only_active_images_cb->IsChecked()) {
        // use only selected images.
        imgs = m_pano->getActiveImages();
    } else {
        for (unsigned int i = 0 ; i < m_pano->getNrOfImages(); i++) {
                imgs.insert(i);
        }
    }
    runOptimizer(imgs);
}

void OptimizePhotometricPanel::OnSelExposure(wxCommandEvent & e)
{
    SetCheckMark(m_exp_list,true, m_pano->getOptions().colorReferenceImage);
}

void OptimizePhotometricPanel::OnDelExposure(wxCommandEvent & e)
{
    SetCheckMark(m_exp_list,false, m_pano->getOptions().colorReferenceImage);
}

void OptimizePhotometricPanel::OnSelWB(wxCommandEvent & e)
{
    SetCheckMark(m_wb_list,true, m_pano->getOptions().colorReferenceImage); 
}

void OptimizePhotometricPanel::OnDelWB(wxCommandEvent & e)
{
    SetCheckMark(m_wb_list,false, m_pano->getOptions().colorReferenceImage);
}

void OptimizePhotometricPanel::SetCheckMark(wxCheckListBox * l, int check, int anchor)
{
    // TODO: (un)check all linked variables as well!
    int n = l->GetCount();
    for (int i=0; i < n; i++) {
        if (check) {
            l->Check(i, i != anchor);
        } else {
            l->Check(i, 0);
        }
    }
}


OptimizeVector OptimizePhotometricPanel::getOptimizeVector()
{

    int nrLI = m_exp_list->GetCount();
    assert(nrLI >=0);
    unsigned int nr = (unsigned int) nrLI;
    unsigned int nImages = m_pano->getNrOfImages();
    assert(nr == nImages);

    OptimizeVector optvars;

    for (unsigned int i=0; i < nImages; i++) {

        set<string> imgopt;
        // lens variables
        unsigned int lensNr = m_pano->getImage(i).getLensNr();

        if (m_vig_list->IsChecked(lensNr)) {
            imgopt.insert("Vb");
            imgopt.insert("Vc");
            imgopt.insert("Vd");
        }
        if (m_vigc_list->IsChecked(lensNr)) {
            imgopt.insert("Vx");
            imgopt.insert("Vy");
        }
        if (m_resp_list->IsChecked(lensNr)) {
            imgopt.insert("Ra");
            imgopt.insert("Rb");
            imgopt.insert("Rc");
            imgopt.insert("Rd");
            imgopt.insert("Re");
        }

        // image variables
        if (m_exp_list->IsChecked(i)) {
            imgopt.insert("Eev");
        }
        if (m_wb_list->IsChecked(i)) {
            imgopt.insert("Er");
            imgopt.insert("Eb");
        }
        optvars.push_back(imgopt);
    }

    return optvars;
}

void OptimizePhotometricPanel::panoramaChanged(PT::Panorama & pano)
{
	DEBUG_TRACE("");
    // update accordingly to the choosen mode
//    wxCommandEvent dummy;
//    OnChangeMode(dummy);
}

void OptimizePhotometricPanel::panoramaImagesChanged(PT::Panorama &pano,
                                          const PT::UIntSet & imgNr)
{
    DEBUG_TRACE("nr of changed images: " << imgNr.size());
    if (pano.getNrOfImages() <= 1)
    {
        XRCCTRL(*this, "optimize_photo_frame_optimize", wxButton)->Disable();
	m_mode_cb->Disable();
    } else {
        XRCCTRL(*this, "optimize_photo_frame_optimize", wxButton)->Enable();
        m_mode_cb->Enable();
    }
    // update lens values
    int nrLensList = m_vig_list->GetCount();
    assert(nrLensList >=0);
    unsigned int nr = (unsigned int) nrLensList;
    unsigned int nLens = m_pano->getNrOfLenses();
    while (nr < nLens) {
        // add checkboxes.
        m_vig_list->Append(wxString::Format(wxT("%d"),nr));
        m_vigc_list->Append(wxString::Format(wxT("%d"),nr));
        m_resp_list->Append(wxString::Format(wxT("%d"),nr));
        nr++;
    }

    while (nr > nLens) {
        if (nr == 0)
            break;
        m_vig_list->Delete(nr-1);
        m_vigc_list->Delete(nr-1);
        m_resp_list->Delete(nr-1);
        nr--;
    }

    // add/remove items
    int nrLI = m_exp_list->GetCount();
    assert(nrLI >=0);
    nr = (unsigned int) nrLI;
    unsigned int nImages = m_pano->getNrOfImages();
    while (nr < nImages) {
        // add checkboxes.
        m_exp_list->Append(wxString::Format(wxT("%d"),nr));
        m_wb_list->Append(wxString::Format(wxT("%d"),nr));
        nr++;
    }

    while (nr > nImages) {
        if (nr == 0)
            break;
        m_exp_list->Delete(nr-1);
        m_wb_list->Delete(nr-1);
        nr--;
    }

    // display values of the variables.
    UIntSet::const_iterator it;
    for (it = imgNr.begin(); it != imgNr.end(); it++) {
        DEBUG_DEBUG("setting checkmarks for image " << *it)
        const VariableMap & vars = pano.getImageVariables(*it);
        // keep selections
        bool sel = m_exp_list->IsChecked(*it);
        m_exp_list->SetString(*it, wxString::Format(wxT("%d (%.3f)"),
                                *it, const_map_get(vars,"Eev").getValue()));
        m_exp_list->Check(*it,sel);

        sel = m_wb_list->IsChecked(*it);
        m_wb_list->SetString(*it, wxString::Format(wxT("%d (%.3f, %.3f)"),
                                *it, const_map_get(vars,"Er").getValue(),
                                const_map_get(vars,"Eb").getValue()));
        m_wb_list->Check(*it,sel);
    }

    // display lens values if they are linked
    for (unsigned int i=0; i < nLens; i++) {
        const Lens & lens = pano.getLens(i);
        const LensVariable & vb = const_map_get(lens.variables,"Vb");
        const LensVariable & vc = const_map_get(lens.variables,"Vc");
        const LensVariable & vd = const_map_get(lens.variables,"Vd");
        bool sel = m_vig_list->IsChecked(i);
        if (vb.isLinked()) {
            m_vig_list->SetString(i,wxString::Format(wxT("%d (%.1f, %.1f, %.1f)"),i, vb.getValue(),
                                vc.getValue(), vd.getValue()));
        } else {
            m_vig_list->SetString(i,wxString::Format(wxT("%d"),i));
        }
        m_vig_list->Check(i,sel);

        sel = m_vigc_list->IsChecked(i);
        const LensVariable & vx = const_map_get(lens.variables,"Vx");
        const LensVariable & vy = const_map_get(lens.variables,"Vy");
        if (vx.isLinked()) {
            m_vigc_list->SetString(i,wxString::Format(wxT("%d (%.1f, %.1f)"),i,
                                   vx.getValue(), vy.getValue()));
        } else {
            m_vigc_list->SetString(i,wxString::Format(wxT("%d"), i));
        }
        m_vigc_list->Check(i,sel);

        sel = m_resp_list->IsChecked(i);
        const LensVariable & ra = const_map_get(lens.variables,"Ra");
        const LensVariable & rb = const_map_get(lens.variables,"Rb");
        const LensVariable & rc = const_map_get(lens.variables,"Rc");
        const LensVariable & rd = const_map_get(lens.variables,"Rd");
        const LensVariable & re = const_map_get(lens.variables,"Re");
        if (ra.isLinked()) {
            m_resp_list->SetString(i,wxString::Format(wxT("%d (%.2f, %.2f, %.2f, %.2f, %.2f)"),i, 
                                   ra.getValue(), rb.getValue(), rc.getValue(),
                                   rd.getValue(), re.getValue()));
        } else {
            m_resp_list->SetString(i,wxString::Format(wxT("%d"),i));
        }
        m_resp_list->Check(i,sel);
    }

    // update automatic checkmarks
    wxCommandEvent dummy;
    dummy.SetInt(m_mode_cb->GetSelection());
    OnChangeMode(dummy);

}

void OptimizePhotometricPanel::setOptimizeVector(const OptimizeVector & optvec)
{
    DEBUG_ASSERT((int)optvec.size() == (int)m_exp_list->GetCount());

    for (int i=0; i < (int) m_pano->getNrOfLenses(); i++) {
        m_vig_list->Check(i,false);
        m_vigc_list->Check(i,false);
        m_resp_list->Check(i,false);
    }

    unsigned int nImages = optvec.size();
    for (unsigned int i=0; i < nImages; i++) {
        m_exp_list->Check(i,false);
        m_wb_list->Check(i,false);
        unsigned int lensNr = m_pano->getImage(i).getLensNr();

        for(set<string>::const_iterator it = optvec[i].begin();
            it != optvec[i].end(); ++it)
        {
            if (*it == "Eev") {
                m_exp_list->Check(i);
            }
            if (*it == "Er") {
                m_wb_list->Check(i);
            }

            if (*it == "Ra") {
                m_resp_list->Check(lensNr);
            }
            if (*it == "Vb") {
                m_vig_list->Check(lensNr);
            }
            if (*it == "Vx") {
                m_vigc_list->Check(lensNr);
            }
        }
    }
}

void OptimizePhotometricPanel::runOptimizer(const UIntSet & imgs)
{
    DEBUG_TRACE("");
    int mode = m_mode_cb->GetSelection();

    // first, check if vignetting and response coefficients are linked
    bool linked = true;
    std::vector<LensVarMap> lensvars;
    // check if vignetting and response are linked, display a warning if they are not

    const char * varnames[] = {"Va", "Vb", "Vc", "Vd", "Vx", "Vy",
                         "Ra", "Rb", "Rc", "Rd", "Re",  0};

    UIntSet lenses;
    for (size_t i = 0; i < m_pano->getNrOfLenses(); i++) {
        const Lens & l = m_pano->getLens(i);
        LensVarMap varmap;

        for (const char ** v = varnames; *v; v++) {
            LensVariable var = const_map_get(l.variables, *v);
            if (!var.isLinked()) {
                var.setLinked();
                varmap.insert(make_pair(var.getName(), var));
                linked = false;
            }
        }
        lensvars.push_back(varmap);
        lenses.insert(i);
    }
    if (!linked) {
        int ok = wxMessageBox(_("The same vignetting and response parameters should\nbe applied for all images of a lens.\nCurrently each image can have different parameters.\nLink parameters?"), _("Link parameters"), wxYES_NO | wxICON_INFORMATION);
        if (ok == wxYES) {
            GlobalCmdHist::getInstance().addCommand(
                    new PT::SetLensVariablesCmd(*m_pano, lenses, lensvars)
                                                   );
        }
    }

    Panorama optPano = m_pano->getSubset(imgs);
    PanoramaOptions opts = optPano.getOptions();


    std::vector<vigra_ext::PointPairRGB> m_points;
    // extract points only if not done previously

    long nPoints = 200;
    wxConfigBase::Get()->Read(wxT("/OptimizePhotometric/nRandomPointsPerImage"), & nPoints);
    // get parameters for estimation.
    nPoints = wxGetNumberFromUser(_("The vignetting and exposure correction is determined by analysing color values in the overlapping areas.\nTo speed up the computation, only a random subset of points is used."),
                                    _("Number of points per image"),
                                    _("Photometric optimization"), nPoints, 0, 32000,
                                    this);
    if (nPoints < 0) {
        return;
    }
    wxConfigBase::Get()->Write(wxT("/OptimizePhotometric/nRandomPointsPerImage"),nPoints);

    ProgressReporterDialog progress(5.0, _("Photometric alignment"), wxT("Loading images"));

    progress.Show();

    nPoints = nPoints * optPano.getNrOfImages();
    // get the small images
    std::vector<vigra::FRGBImage *> srcImgs;
    for (size_t i=0; i < optPano.getNrOfImages(); i++) {
        ImageCache::EntryPtr e = ImageCache::getInstance().getSmallImage(optPano.getImage(i).getFilename());
        vigra::FRGBImage * img = new FRGBImage;
        if (!e) {
            wxMessageBox(_("Error: could not load all images"), _("Error"));
            return;
        }
        if (e->image8 && e->image8->width() > 0) {
            reduceToNextLevel(*(e->image8), *img);
            transformImage(vigra::srcImageRange(*img), vigra::destImage(*img),
                            vigra::functor::Arg1()/vigra::functor::Param(255.0));
        } else if (e->image16 && e->image16->width() > 0) {
            reduceToNextLevel(*(e->image16), *img);
            transformImage(vigra::srcImageRange(*img), vigra::destImage(*img),
                            vigra::functor::Arg1()/vigra::functor::Param(65535.0));
        } else {
            reduceToNextLevel(*(e->imageFloat), *img);
        }
        srcImgs.push_back(img);
    }
    bool randomPoints = true;
    extractPoints(optPano, srcImgs, nPoints, randomPoints, progress, m_points);

    if (m_points.size() == 0) {
        wxMessageBox(_("Error: no overlapping points found, Photometric optimization aborted"), _("Error"));
        return;
    }

    double error = 0;
    try {
        //wxBusyCursor busyc;
        if (mode != OPT_CUSTOM) {
            // run automatic optimisation
            // ensure that we have a valid anchor.
            PanoramaOptions opts = optPano.getOptions();
            if (opts.colorReferenceImage >= optPano.getNrOfImages()) {
                opts.colorReferenceImage = 0;
                optPano.setOptions(opts);
            }
            smartOptimizePhotometric(optPano, PhotometricOptimizeMode(mode),
                                    m_points, progress, error);
        } else {
            OptimizeVector optvars = getOptimizeVector();
            if (optPano.getNrOfImages() != m_pano->getNrOfImages()) {
                OptimizeVector o = optvars;
                optvars.clear();
                for (UIntSet::const_iterator it = imgs.begin();
                     it != imgs.end(); ++it)
                {
                    optvars.push_back(o[*it]);
                }
            }
            // optimize selected parameters
            optimizePhotometric(optPano, optvars,
                                m_points, progress, error);
        }
    } catch (std::exception & error) {
        wxMessageBox(_("Internal error during photometric optimization:\n") + wxString(error.what(), wxConvLocal), _("Internal error"));
    }

    progress.Close();

    // display information about the estimation process:
    int ret = wxMessageBox(wxString::Format(_("Photometric optimization results:\nAverage difference (RMSE) between overlapping pixels: %.2f gray values (0..255)\n\nApply results?"), error*255),
                           _("Photometric optimization finished"), wxYES_NO | wxICON_INFORMATION);

    if (ret == wxYES) {
        DEBUG_DEBUG("Applying vignetting corr");
        PanoramaOptions opts = m_pano->getOptions();
        opts.outputExposureValue = calcMeanExposure(*m_pano);
        // TODO: merge into a single update command
        GlobalCmdHist::getInstance().addCommand(
                new PT::SetPanoOptionsCmd(*m_pano, opts)
                                               );

        const VariableMapVector & vars = optPano.getVariables();
        GlobalCmdHist::getInstance().addCommand(
                new PT::UpdateImagesVariablesCmd(*m_pano, imgs, vars)
                                               );
    }
}

void OptimizePhotometricPanel::OnClose(wxCloseEvent& event)
{
    DEBUG_TRACE("OnClose");
    // do not close, just hide if we're not forced
    if (event.CanVeto()) {
        event.Veto();
        Hide();
        DEBUG_DEBUG("Hiding");
    } else {
        DEBUG_DEBUG("Closing");
        Destroy();
    }
}

void OptimizePhotometricPanel::OnChangeMode(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    int mode = m_mode_cb->GetSelection();
    DEBUG_ASSERT(mode >= 0 && mode < OPT_END_MARKER);
    if (m_pano->getNrOfImages() == 0)
    {
        XRCCTRL(*this, "opt_exp_select", wxButton)->Disable();
        XRCCTRL(*this, "opt_wb_select", wxButton)->Disable();
        XRCCTRL(*this, "opt_exp_clear", wxButton)->Disable();
        XRCCTRL(*this, "opt_wb_clear", wxButton)->Disable();
    } else {
        bool enabled = mode == OPT_CUSTOM;
        unsigned int refImg = m_pano->getOptions().colorReferenceImage;
        switch (mode) {
        case OPT_LDR:
            setOptimizeVector(createOptVars(*m_pano, OPT_VIG | OPT_EXP | OPT_RESP, refImg));
            break;
        case OPT_LDR_WB:
            setOptimizeVector(createOptVars(*m_pano, OPT_VIG | OPT_EXP | OPT_RESP | OPT_WB, refImg));
            break;
        case OPT_HDR:
            setOptimizeVector(createOptVars(*m_pano, OPT_VIG | OPT_RESP, refImg));
            break;
        case OPT_HDR_WB:
            setOptimizeVector(createOptVars(*m_pano, OPT_VIG | OPT_RESP | OPT_WB, refImg));
            break;
        case OPT_CUSTOM:
            break;
        }
        XRCCTRL(*this, "opt_exp_select", wxButton)->Enable(enabled);
        XRCCTRL(*this, "opt_wb_select", wxButton)->Enable(enabled);
        XRCCTRL(*this, "opt_exp_clear", wxButton)->Enable(enabled);
        XRCCTRL(*this, "opt_wb_clear", wxButton)->Enable(enabled);
        m_vig_list->Enable(enabled);
        m_vigc_list->Enable(enabled);
        m_exp_list->Enable(enabled);
        m_wb_list->Enable(enabled);
        m_resp_list->Enable(enabled);
    }
}

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
