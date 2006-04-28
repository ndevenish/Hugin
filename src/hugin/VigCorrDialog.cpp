// -*- c-basic-offset: 4 -*-

/** @file PreferencesDialog.cpp
 *
 *  @brief implementation of VigCorrDialog Class
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
#include "common/wxPlatform.h"

#include <iostream>
#include <fstream>
#include <vigra/basicimage.hxx>
#include <vigra/basicimageview.hxx>

#include <PT/RemappedPanoImage.h>
#include <vigra_ext/VigQuotientEstimator.h>
#include <PT/RandomPointSampler.h>

#include "hugin/config_defaults.h"
#include "hugin/huginApp.h"
#include "hugin/VigCorrDialog.h"
#include "hugin/CommandHistory.h"
#include "hugin/ImageCache.h"
#include "hugin/MyProgressDialog.h"


// validators are working different somehow...
//#define MY_STR_VAL(id, filter) { XRCCTRL(*this, "prefs_" #id, wxTextCtrl)->SetValidator(wxTextValidator(filter, &id)); }
//#define MY_SPIN_VAL(id) {     XRCCTRL(*this, "prefs_" #id, wxSpinCtrl)->SetValidator(wxGenericValidator(&id)); }

#define MY_STR_VAL(id, val) { XRCCTRL(*this, id, wxTextCtrl)->SetValue(val); };

#define MY_G_STR_VAL(id)  XRCCTRL(*this, id, wxTextCtrl)->GetValue()

using namespace PT;
using namespace utils;
using namespace vigra;
using namespace vigra_ext;
using namespace std;



BEGIN_EVENT_TABLE(VigCorrDialog, wxFrame)
    EVT_BUTTON(wxID_OK, VigCorrDialog::OnOk)
    EVT_BUTTON(wxID_APPLY,VigCorrDialog::OnApply)
    EVT_BUTTON(wxID_CANCEL,VigCorrDialog::OnCancel)
    EVT_BUTTON(XRCID("vig_corr_estimate"), VigCorrDialog::OnEstimate)
    EVT_BUTTON(XRCID("vig_corr_flatfield_select"), VigCorrDialog::OnFlatfieldSelect)
END_EVENT_TABLE()


VigCorrDialog::VigCorrDialog(wxWindow *parent, Panorama & pano, unsigned int imgNr)
//    : wxFrame(parent, -1, _("Preferences - hugin"))
    : m_pano(pano), m_imgNr(imgNr)
{
    DEBUG_TRACE("");

    pano.addObserver(this);

    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadFrame(this, parent, wxT("vig_corr_dlg"));

#if __WXMSW__
    wxIcon myIcon(MainFrame::Get()->GetXRCPath() + wxT("data/icon.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(MainFrame::Get()->GetXRCPath() + wxT("data/icon.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);

    // create xrc variables
    m_corrModeRBB = XRCCTRL(*this, "vig_corr_mode_rbbox", wxRadioBox);
    DEBUG_ASSERT(m_corrModeRBB);

    m_corrFlatRB = XRCCTRL(*this, "vig_corr_flatfield_rb", wxRadioButton);
    DEBUG_ASSERT(m_corrFlatRB);
    m_corrPolyRB = XRCCTRL(*this, "vig_corr_poly_rb", wxRadioButton);
    DEBUG_ASSERT(m_corrPolyRB);

    m_flatEdit = XRCCTRL(*this, "vig_corr_flatfile_edit", wxTextCtrl);
    DEBUG_ASSERT(m_flatEdit);

    m_coef0Edit = XRCCTRL(*this, "vig_corr_coef0_edit", wxTextCtrl);
    DEBUG_ASSERT(m_coef0Edit);
    m_coef1Edit = XRCCTRL(*this, "vig_corr_coef1_edit", wxTextCtrl);
    DEBUG_ASSERT(m_coef1Edit);
    m_coef2Edit = XRCCTRL(*this, "vig_corr_coef2_edit", wxTextCtrl);
    DEBUG_ASSERT(m_coef2Edit);
    m_coef3Edit = XRCCTRL(*this, "vig_corr_coef3_edit", wxTextCtrl);
    DEBUG_ASSERT(m_coef3Edit);

    m_coefxEdit = XRCCTRL(*this, "vig_corr_coefx_edit", wxTextCtrl);
    DEBUG_ASSERT(m_coefxEdit);
    m_coefyEdit = XRCCTRL(*this, "vig_corr_coefy_edit", wxTextCtrl);
    DEBUG_ASSERT(m_coefxEdit);

    m_plot = new Plot2DWindow( this, -1, wxPoint(0,0), wxSize(500,200));
    wxXmlResource::Get()->AttachUnknownControl (
               wxT("vig_corr_plot_poly"),
               m_plot);
//    m_plot->SetAxis(0,1.01, 0,1.01);

    // update display with values from panorama
    UpdateDisplayData();

#if __WXMSW__
    // wxFrame does have a strange background color on Windows, copy color from a child widget
    this->SetBackgroundColour(XRCCTRL(*this, "vig_corr_mode_rbbox", wxRadioBox)->GetBackgroundColour());
#endif

//    RestoreFramePosition(this, wxT("VigCorrDialog"));
}


VigCorrDialog::~VigCorrDialog()
{
    DEBUG_TRACE("begin dtor");
//    SaveFramePosition(this, wxT("VigCorrDialog"));
    m_pano.removeObserver(this);

    DEBUG_TRACE("end dtor");
}


void VigCorrDialog::OnApply(wxCommandEvent & e)
{
    UpdatePanorama();
}


void VigCorrDialog::OnOk(wxCommandEvent & e)
{
    if (UpdatePanorama()) {
        Close();
    }
}

void VigCorrDialog::OnCancel(wxCommandEvent & e)
{
    Close();
}

void VigCorrDialog::OnFlatfieldSelect(wxCommandEvent & e)
{
    wxString wildcard (_("All Image files|*.jpg;*.JPG;*.tif;*.TIF;*.tiff;*.TIFF;*.png;*.PNG;*.bmp;*.BMP;*.gif;*.GIF;*.pnm;*.PNM;*.sun;*.viff|JPEG files (*.jpg)|*.jpg;*.JPG|All files (*)|*"));
    wxFileDialog dlg(this,_("Select flatfield image"),
                     wxConfigBase::Get()->Read(wxT("flatfieldPath"),wxT("")), wxT(""),
                     wildcard,
                     wxOPEN, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK) {
        XRCCTRL(*this, "vig_corr_flatfile_edit", wxTextCtrl)->SetValue(
		dlg.GetPath());
        wxConfig::Get()->Write(wxT("flatfieldPath"), dlg.GetDirectory());
    }
}


void VigCorrDialog::panoramaChanged(Panorama &pano)
{
    UpdateDisplayData();
}


void VigCorrDialog::UpdateDisplayData()
{
    DEBUG_DEBUG("Updating display data");
    // get current Lens.
    if (m_imgNr >= m_pano.getNrOfImages()) {
        Close();
        return;
    }

    const PanoImage & img = m_pano.getImage(m_imgNr);
    const PT::ImageOptions & iopts = img.getOptions();

    if (iopts.m_vigCorrMode  == 0) {
        m_corrModeRBB->SetSelection(0);
    } else if (iopts.m_vigCorrMode & ImageOptions::VIGCORR_DIV) {
        m_corrModeRBB->SetSelection(2);
    } else {
        m_corrModeRBB->SetSelection(1);
    }

    if (iopts.m_vigCorrMode & ImageOptions::VIGCORR_FLATFIELD) {
        m_corrFlatRB->SetValue(true);
        m_corrPolyRB->SetValue(false);
    } else {
        m_corrFlatRB->SetValue(false);
        m_corrPolyRB->SetValue(true);
    }

    // update the coefficients
    unsigned int ndigits = 3;
    std::vector<double> coeff(4);
    coeff[0] = const_map_get(m_pano.getImageVariables(m_imgNr),"Va").getValue();
    coeff[1] = const_map_get(m_pano.getImageVariables(m_imgNr),"Vb").getValue();
    coeff[2] = const_map_get(m_pano.getImageVariables(m_imgNr),"Vc").getValue();
    coeff[3] = const_map_get(m_pano.getImageVariables(m_imgNr),"Vd").getValue();

    m_coef0Edit->SetValue(doubleTowxString(coeff[0],ndigits));
    m_coef1Edit->SetValue(doubleTowxString(coeff[1],ndigits));
    m_coef2Edit->SetValue(doubleTowxString(coeff[2],ndigits));
    m_coef3Edit->SetValue(doubleTowxString(coeff[3],ndigits));
    m_coefxEdit->SetValue(doubleTowxString(const_map_get(m_pano.getImageVariables(m_imgNr),"Vx").getValue(),1));
    m_coefyEdit->SetValue(doubleTowxString(const_map_get(m_pano.getImageVariables(m_imgNr),"Vy").getValue(),1));
    
    // update the flatfield filename
    m_flatEdit->SetValue(wxString (iopts.m_flatfield.c_str(), *wxConvCurrent));

    // update vignetting curve
    VigPlotCurve f(coeff);
    m_plot->Plot(f,0,1);
    if (iopts.m_vigCorrMode & ImageOptions::VIGCORR_DIV) {
        m_plot->SetAxis(0.0, 1, 0.0, 1.2);
    } else {
        m_plot->AutoSizeAxis();
    }
}

void VigCorrDialog::OnEstimate(wxCommandEvent & e)
{
    bool uniformRadiusSample=true;
    int nPoints = wxConfigBase::Get()->Read(wxT("VigCorrDialog/nRandomPoints"),2000l);

    // get parameters for estimation.
    nPoints = wxGetNumberFromUser(_("The vignetting curve is determined by using gray values in the overlaping areas.\nTo speed up the computation, only a random subset of points is used."),
                                      _("Number of points per overlap"),
                                      _("Estimate vignetting curve"), nPoints, 0, INT_MAX,
                                      this);
    wxConfigBase::Get()->Write(wxT("VigCorrDialog/nRandomPoints"),nPoints);

    wxString deltaStr = wxConfigBase::Get()->Read(wxT("VigCorrDialog/ransacTol"),wxT("0.1"));
    deltaStr = wxGetTextFromUser(_("Ransac tolerance parameter.\nShould be around 0.1.\n Smaller numbers might lead to better results, if the images are registered very well,\n and the same camera settings (exposure, white balance) were used."), _("Estimate vignetting curve"),
                                          deltaStr, this);

    double ransacTol=0.1;
    str2double(deltaStr, ransacTol);
    wxConfigBase::Get()->Write(wxT("VigCorrDialog/ransacTol"),wxString::Format(wxT("%.2f"),ransacTol));


    typedef vigra::BImage ImageType;

    // define for the interpolated image that we are going to use.
    typedef vigra_ext::ImageMaskInterpolator<ImageType::const_traverser, ImageType::ConstAccessor,
                                             ImageType::const_traverser, ImageType::ConstAccessor, interp_spline36> InterpolImg;

    // estimate good width for pano
    PanoramaOptions opts = m_pano.getOptions();
    PT::PanoImage img = m_pano.getImage(m_imgNr);
    PT::ImageOptions iopt = img.getOptions();
    // get hfov
    double v = const_map_get(m_pano.getImageVariables(m_imgNr),"v").getValue();
    wxImage * src = ImageCache::getInstance().getSmallImage(img.getFilename().c_str());
    if (!src->Ok()) {
        throw std::runtime_error("could not retrieve small source image for vignetting optimisation");
    }

    double scale = calcOptimalPanoScale(m_pano.getSrcImage(m_imgNr), opts);

    opts.setWidth(roundi(opts.getWidth()*scale));

    // prepare images for random point extraction
    std::vector<BImage *> grayImgs;
    std::vector<BImage *> maskImgs;
    std::vector<FImage *> lapImgs;
    std::vector<InterpolImg> srcImgs;
    std::vector<SrcPanoImage> srcDescr;

    MyProgressDialog progress(_("Vignetting function estimation"), wxT("\n\n\n\n"), this);
    progress.pushTask(ProgressTask("Optimize vignetting poly", "high pass filtering input images", 1.0/3));

    unsigned lensNr = m_pano.getImage(m_imgNr).getLensNr();
    UIntSet imgs;
    bool useActive = wxConfigBase::Get()->Read(wxT("/General/UseOnlySelectedImages"),HUGIN_USE_SELECTED_IMAGES) != 0;
    UIntSet activeImgs;
    if (useActive)
    {
        // use only selected images.
        activeImgs = m_pano.getActiveImages();
    }
    // select all images of current lens
    for (unsigned i=0; i < m_pano.getNrOfImages(); i++) {
        // skip images that are not active.
        if (useActive && (!set_contains(activeImgs,i))) {
            continue;
        }
        if (m_pano.getImage(i).getLensNr() == lensNr) {

            SrcPanoImage src = m_pano.getSrcImage(i);
            wxImage * srcImgWX = ImageCache::getInstance().getSmallImage(src.getFilename().c_str());
            if (!srcImgWX->Ok()) {
                throw std::runtime_error("could not retrieve small source image for vignetting optimisation");
            }
            // image view
            BasicImageView<RGBValue<unsigned char> > srcImgInCache((RGBValue<unsigned char> *)srcImgWX->GetData(),
                                     srcImgWX->GetWidth(),
                                     srcImgWX->GetHeight());

            src.resize(srcImgInCache.size());
            BImage * grayImg = new BImage(srcImgInCache.size());
            // change to grayscale
            vigra::copyImage(vigra::srcImageRange(srcImgInCache, vigra::RGBToGrayAccessor<RGBValue<unsigned char> >()),
                             vigra::destImage(*grayImg));
            BImage *maskImg = new BImage(srcImgInCache.size().x,srcImgInCache.size().y , 255);

            // TODO need to do gamma correction here!
            if (src.getGamma() != 1.0) {
                vigra_ext::applyGammaCorrection(srcImageRange(*grayImg), destImage(*grayImg),
                                                src.getGamma(), 255);
            }
            // TODO: cut out the masked parts.
//            applyMask(src, vigra::destImageRange(*maskImg));

            vigra_ext::interp_spline36 interp;
            // create interpolating accessor to images.
            srcImgs.push_back(InterpolImg(vigra::srcImageRange(*grayImg),
                              vigra::srcImage(*maskImg), interp,
                              false) );
            grayImgs.push_back(grayImg);
            maskImgs.push_back(maskImg);
            srcDescr.push_back(src);
            if (uniformRadiusSample) {
                vigra::FImage * lap = new vigra::FImage(grayImg->size());
                laplacianOfGaussian(srcImageRange(*grayImg), destImage(*lap), 1);
                lapImgs.push_back(lap);
            }

        }
    }
    if (grayImgs.size() < 2) {
        wxMessageBox(_("Not enought images selected. Please ensure that more than two images are associated with the current lens."), _("Cannot estimate vignetting"), wxOK | wxICON_HAND);
        return;
    }
    // advance progress
    progress.increase();
    progress.setMessage("remapping");
    progress.pushTask(utils::ProgressTask("Extracting overlapping points", "", 1.0/(grayImgs.size())));

    std::vector<vigra_ext::PointPair> points;

    unsigned nBadPoints=0;
    unsigned nGoodPoints=0;
    if (uniformRadiusSample) {
        std::vector<std::multimap<double, vigra_ext::PointPair> > radiusHist;

        sampleAllPanoPoints(srcImgs,
                            lapImgs, srcDescr,
                            opts,
                            nPoints, 10, 250,
                            radiusHist,
                            nGoodPoints,
                            nBadPoints,
                            progress);
        // dump histogram bins to disk.
        /*
        {
            for (unsigned i=0; i < radiusHist.size(); i++) {
                ostringstream fn;
                fn << "huginRandPnt_" << i;
                ofstream of(fn.str().c_str());
                for (std::multimap<double, int>::const_iterator it= radiusHist[i].begin();
                     it != radiusHist[i].end(); ++it) 
                {
                    of << (*it).first << " " 
                            << allPoints[(*it).second].i1 << " "
                            << allPoints[(*it).second].r1 << " "
                            << allPoints[(*it).second].i2 << " "
                            << allPoints[(*it).second].r2 << endl;
                }
            }
        }
        */
        progress.setMessage("selecting points in uniform areas");
        // select points with low laplacian of gaussian values.
        sampleRadiusUniform(radiusHist, nPoints, points);
        progress.popTask();
    } else {
        // extract random points.
        sampleRandomPanoPoints(srcImgs,
                               srcDescr,
                               opts,
                               nPoints,
                               10,
                               250,
                               points,
                               nBadPoints);
        nGoodPoints=points.size();
    }
    // DEBUG: write random points to disk
    /*
    {
        ofstream of("hugin_random_points");
        for (std::vector<PointPair>::iterator it = points.begin(); it != points.end(); ++it) {
            of << (*it).r1 << " " << (*it).i1 << " " << (*it).r2 << " " << (*it).i2 << std::endl;
        }
    }
    */

    progress.setMessage("estimating vignetting polynom");

    // extract vignetting coefficients..
    std::vector<double> vigCoeff(3);
    vigCoeff[0] = 0;
    vigCoeff[1] = 0;
    vigCoeff[2] = 0;

    VigQuotientEstimateResult res = 
    optimizeVignettingQuotient(points, ransacTol, vigCoeff);

    for (unsigned i=0; i < grayImgs.size(); i++) {
        delete grayImgs[i];
        delete maskImgs[i];
        if (uniformRadiusSample) {
            delete lapImgs[i];
        }
    }
    progress.increase();

    unsigned int ndigits = 5;
    m_coef0Edit->SetValue(doubleTowxString(1,ndigits));
    m_coef1Edit->SetValue(doubleTowxString(vigCoeff[0],ndigits));
    m_coef2Edit->SetValue(doubleTowxString(vigCoeff[1],ndigits));
    m_coef3Edit->SetValue(doubleTowxString(vigCoeff[2],ndigits));

    m_corrModeRBB->SetSelection(2);
    m_corrFlatRB->SetValue(false);
    m_corrPolyRB->SetValue(true);

    std::vector<double> coeff(4);
    coeff[0] = 1;
    for (int i=0; i < 3; i++) 
        coeff[i+1] = vigCoeff[i];
    // calculate vignetting curve.
    VigPlotCurve f(coeff);
    m_plot->Plot(f, 0, 1);
    m_plot->SetAxis(0, 1, 0, 1.2);

    // display information about the estimation process:
    double inconsistentPoints = (double) nBadPoints/(nGoodPoints+nBadPoints);
    if (inconsistentPoints > 0.5) {
        wxMessageBox(wxString::Format(_("Vignetting curve estimation results:\n\nWARNING: %.1f%% of all points were inconsistent.\n\nPlease check if the images fit well and the same exposure and white balance settings have been used for all images.\n\nThe recovered vignetting curve might be inaccurate."),
                                       inconsistentPoints*100),
                                       _("Warning"), wxOK | wxICON_HAND);
    }
    wxMessageBox(wxString::Format(_("Vignetting curve estimation results:\n%.1f%% inconsistent point pairs found, used %.1f%% of %d pairs during final estimation.\nBrightness error: %.2f (gray value RMSE)"),
                                  inconsistentPoints*100,
                                  ((double)res.nUsedPoints/(double)points.size()) * 100,
                                  points.size(),
                                  res.brightnessRMSE),
                 _("Vignetting curve estimation"), wxOK | wxICON_INFORMATION); 
}

#if 0
void VigCorrDialog::OnEstimateOld(wxCommandEvent & e)
{
    // need to get the remapped panorama image...

    typedef vigra::BImage ImageType;
    typedef RemappedPanoImage<vigra::BImage, vigra::BImage> RImg;
    typedef ROIImage<ImageType, vigra::BImage> RoiImg;

    unsigned lensNr = m_pano.getImage(m_imgNr).getLensNr();
    UIntSet imgs;
    // select all images of current lens
    for (unsigned i=0; i < m_pano.getNrOfImages(); i++) {
        if (m_pano.getImage(i).getLensNr() == lensNr) {

            SrcPanoImage src = pano.getSrcImage(i);

            wxImage * srcImgWX = ImageCache::getInstance().getSmallImage(src.getFilename().c_str());
            if (!src->Ok()) {
                throw std::runtime_error("could not retrieve small source image for vignetting optimisation");
            }
            // image view
            BasicImageView<RGBValue<unsigned char> > srcImgInCache((RGBValue<unsigned char> *)src->GetData(),
                                     srcImgWX->GetWidth(),
                                     srcImgWX->GetHeight());

            srcImgDescr.resize(srcImgInCache.size());
            BImage srcImg(srcImgInCache.size());
            
            // change to grayscale
            vigra::copyImage(vigra::srcImageRange(srcImgInCache, vigra::RGBToGrayAccessor<RGBValue<unsigned char> >()),
                             vigra::destImage(srcImg));

        SrcPanoImage srcPanoImg = m_pano.getSrcImage(*it);
        // adjust distortion parameters for small preview image
        srcPanoImg.resize(srcImg.size());
            imgs.insert(i);
        }
    }

    // empty ROI
    vigra::Rect2D panoROI;

    PanoramaOptions opts = m_pano.getOptions();
    PT::PanoImage img = m_pano.getImage(m_imgNr);
    PT::ImageOptions iopt = img.getOptions();
        // get hfov
    double v = const_map_get(m_pano.getImageVariables(m_imgNr),"v").getValue();
    wxImage * src = ImageCache::getInstance().getSmallImage(img.getFilename().c_str());
    if (!src->Ok()) {
        throw std::runtime_error("could not retrieve small source image for vignetting optimisation");
    }

    unsigned panoWidth = calcOptimalPanoWidth(opts, img, v,
                                              m_pano.getLens(img.getLensNr()).getProjection(),
                                              vigra::Size2D(src->GetWidth(), src->GetHeight()) );

    opts.setWidth(panoWidth);
    // remap images
    std::vector<RoiImg *> remapped;
    std::vector<FImage>  imgXCoord(imgs.size());
    std::vector<FImage>  imgYCoord(imgs.size());
    std::vector<Size2D>  sizes(imgs.size());
    std::vector<FDiff2D> centers(imgs.size());

    MyProgressDialog progress(_("Vignetting function estimation"), wxT("\n\n\n\n\n\n\n\n"), this);
    progress.pushTask(ProgressTask("Optimize vignetting poly", "remapping", 1.0/(imgs.size()+1)));

    Size2D srcSize;
    int idx=0;
    // hmm, need to estimate a suitable panorama size.
    for (UIntSet::iterator it=imgs.begin(); it != imgs.end(); ++it) {

        PanoImage panoImg = m_pano.getImage(*it);
        ImageOptions iopts = img.getOptions();
        // disable vignetting correction.
        iopts.m_vigCorrMode = ImageOptions::VIGCORR_NONE;
        panoImg.setOptions(iopts);

        RImg * remappedImg = new RImg;
        // get source images.
        BImage srcFlat;  // only a dummy image.
        BImage srcMask; // only a dummy image
        wxImage * src = ImageCache::getInstance().getSmallImage(panoImg.getFilename().c_str());
        if (!src->Ok()) {
            throw std::runtime_error("could not retrieve small source image for vignetting optimisation");
        }
        // image view
        BasicImageView<RGBValue<unsigned char> > srcImgInCache((RGBValue<unsigned char> *)src->GetData(),
                                     src->GetWidth(),
                                     src->GetHeight());
        BImage srcImg(srcImgInCache.size());
        // change to grayscale
        vigra::copyImage(vigra::srcImageRange(srcImgInCache, vigra::RGBToGrayAccessor<RGBValue<unsigned char> >()),
                         vigra::destImage(srcImg));

        SrcPanoImage srcPanoImg = m_pano.getSrcImage(*it);
        // adjust distortion parameters for small preview image
        srcPanoImg.resize(srcImg.size());

        // remap the image
        remapImage(srcImg,
                   srcMask,
                   srcFlat,
                   srcPanoImg,
                   opts.getDestImage(),
                   opts.interpolator,
                   *remappedImg,
                   progress);

        // create X and Y Coordinates
        remapped.push_back(remappedImg);
        remappedImg->calcSrcCoordImgs(imgXCoord[idx], imgYCoord[idx]);

        sizes[idx] = srcImg.size();
        centers[idx] = srcPanoImg.getRadialVigCorrCenter();

        idx++;
    }

    // get parameters for estimation.
    int nPoints = wxGetNumberFromUser(_("The vignetting curve is determined by using gray values in the overlaping areas.\nTo speed up the computation, only a random subset of points is used."), _("Number of points per overlap"), _("Estimate vignetting curve"), 10000,0, INT_MAX);

    wxString deltaStr = wxGetTextFromUser(_("Ransac tolerance parameter.\nShould be around 0.1.\n Smaller numbers might lead to better results, if the images are registered very well,\n and the same camera settings (exposure, white balance) were used."), _("Estimate vignetting curve"), _("0.1"));

    double ransacTol=0.1;
    str2double(deltaStr, ransacTol);

    // extract vignetting coefficients..
    boost::numeric::ublas::vector<double> vigCoeff(3);
    vigCoeff[0] = 0;
    vigCoeff[1] = 0;
    vigCoeff[2] = 0;

    VigQuotientEstimateResult res = 
    optimizeVignettingQuotient(remapped,
                               imgXCoord,
                               imgYCoord,
                               sizes,
                               centers,
                               vigCoeff,
                               nPoints,
                               ransacTol
                              );

    for (UIntSet::iterator it=imgs.begin(); it != imgs.end(); ++it) {
        delete remapped[*it];
    }

    unsigned int ndigits = 5;
    m_coef0Edit->SetValue(doubleTowxString(1,ndigits));
    m_coef1Edit->SetValue(doubleTowxString(vigCoeff[0],ndigits));
    m_coef2Edit->SetValue(doubleTowxString(vigCoeff[1],ndigits));
    m_coef3Edit->SetValue(doubleTowxString(vigCoeff[2],ndigits));

    std::vector<double> coeff(4);
    coeff[0] = 1;
    for (int i=0; i < 3; i++) 
        coeff[i+1] = vigCoeff[i];
    // calculate vignetting curve.
    VigPlotCurve f(coeff);
    m_plot->Plot(f, 0, 1.01);

    // display information about the estimation process:
    wxMessageBox(wxString::Format(_("Vignetting curve estimation results:\n %d good Points, %d bad points, used %.1f%% of good points during final estimation.\n Residual after estimation: %.5f"),
                  res.nGoodPoints, res.nBadPoints,
                  ((double)res.nUsedPoints/(double)res.nGoodPoints) * 100,
                   res.residual),
               _("Vignetting curve estimation"), wxOK | wxICON_INFORMATION); 
}
#endif

bool VigCorrDialog::UpdatePanorama()
{
    // always apply to all images with the same lens.

    unsigned int mode(ImageOptions::VIGCORR_NONE);
    int moderbb = m_corrModeRBB->GetSelection();
    if (moderbb != 0) {
        if (moderbb == 2) {
            mode |= ImageOptions::VIGCORR_DIV;
        }
        if (m_corrPolyRB->GetValue()) {
            mode |= ImageOptions::VIGCORR_RADIAL;
        } else if(m_corrFlatRB->GetValue()) {
            mode |= ImageOptions::VIGCORR_FLATFIELD;
        }
    }

    if ( (mode & ImageOptions::VIGCORR_FLATFIELD) &&  !wxFileExists(m_flatEdit->GetValue())) {
        wxMessageBox(_("Error: could not find flatfile image file."), _("File not found"));
        return false;
    }
    std::string flat(m_flatEdit->GetValue().mb_str());

    std::vector<double> coeff(6);
    if (!str2double(m_coef0Edit->GetValue(), coeff[0]))  return false;
    if (!str2double(m_coef1Edit->GetValue(), coeff[1]))  return false;
    if (!str2double(m_coef2Edit->GetValue(), coeff[2]))  return false;
    if (!str2double(m_coef3Edit->GetValue(), coeff[3]))  return false;
    if (!str2double(m_coefxEdit->GetValue(), coeff[4]))  return false;
    if (!str2double(m_coefyEdit->GetValue(), coeff[5]))  return false;

    GlobalCmdHist::getInstance().addCommand(
            new SetVigCorrCmd(m_pano, m_pano.getImage(m_imgNr).getLensNr(),
                              mode, coeff, flat) );
    return true;
}
