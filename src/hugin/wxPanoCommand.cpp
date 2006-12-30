// -*- c-basic-offset: 4 -*-

/** @file PanorCommand.cpp
 *
 *  @brief implementation of some PanoCommands
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

#include <hugin/ImageCache.h>
#include <hugin/wxPanoCommand.h>

#include <vigra/cornerdetection.hxx>
#include <vigra/localminmax.hxx>



using namespace std;
using namespace vigra;
using namespace utils;

namespace PT {

void wxAddCtrlPointGridCmd::execute()
{
    PanoCommand::execute();


    const PanoImage & i1 = pano.getImage(img1);
    const PanoImage & i2 = pano.getImage(img1);

    // run both images through the harris corner detector
    const vigra::BImage & leftImg = ImageCache::getInstance().getPyramidImage(
        i1.getFilename(),1);

    BImage leftCorners(leftImg.size());
    FImage leftCornerResponse(leftImg.size());

    // empty corner image
    leftCorners.init(0);

    DEBUG_DEBUG("running corner detector threshold: " << cornerThreshold << "  scale: " << scale );

    // find corner response at scale scale
    vigra::cornerResponseFunction(srcImageRange(leftImg),
        destImage(leftCornerResponse),
        scale);

    //    saveScaledImage(leftCornerResponse,"corner_response.png");
    DEBUG_DEBUG("finding local maxima");
    // find local maxima of corner response, mark with 1
    vigra::localMaxima(srcImageRange(leftCornerResponse), destImage(leftCorners), 255);

//    exportImage(srcImageRange(leftCorners), vigra::ImageExportInfo("c:/corner_response_maxima.png"));

    DEBUG_DEBUG("thresholding corner response");
    // threshold corner response to keep only strong corners (above 400.0)
    transformImage(srcImageRange(leftCornerResponse), destImage(leftCornerResponse),
        vigra::Threshold<double, double>(
        cornerThreshold, DBL_MAX, 0.0, 1.0));

    vigra::combineTwoImages(srcImageRange(leftCorners), srcImage(leftCornerResponse),
        destImage(leftCorners), std::multiplies<float>());

//    exportImage(srcImageRange(leftCorners), vigra::ImageExportInfo("c:/corner_response_threshold.png"));

    // create transform from img1 -> sphere
    PTools::Transform img1ToSphere;
    PTools::Transform sphereToImg2;

    PanoramaOptions opts;
    opts.setProjection(PanoramaOptions::EQUIRECTANGULAR);
    opts.setHFOV(360);
    opts.setWidth(360);
    opts.setVFOV(180);

    img1ToSphere.createInvTransform(pano, img1, opts);
    sphereToImg2.createTransform(pano, img2, opts);


    int border = 5;
    double sphx, sphy;
    double img2x, img2y;
    // sample grid on img1 and try to add ctrl points
    for (unsigned int x=0; x < i1.getWidth(); x += 2 ) {
        for (unsigned int y=0; y < i1.getHeight(); y +=2 ) {
            if (leftCorners(x/2,y/2) > 0) {
                img1ToSphere.transformImgCoord(sphx, sphy, x, y);
                sphereToImg2.transformImgCoord(img2x, img2y, sphx, sphy);
                // check if it is inside..
                if (   img2x > border && img2x < i2.getWidth() - border
                    && img2y > border && img2y < i2.getHeight() - border )
                {
                    // add control point
                    ControlPoint p(img1,x, y, img2, img2x, img2y);
                    pano.addCtrlPoint(p);
                }
            }
        }
    }
    pano.changeFinished();
}


void wxAddImagesCmd::execute()
{
    PanoCommand::execute();

    // check if the files should be sorted by date
    long sort = wxConfigBase::Get()->Read(wxT("General/SortNewImgOnAdd"), HUGIN_GUI_SORT_NEW_IMG_ON_ADD);

    switch (sort) {
        case 1:
                // sort by filename
            std::sort(files.begin(), files.end());
            break;
        case 2:
                // sort by date
            std::sort(files.begin(), files.end(), FileIsNewer());
            break;
        default:
                // no or unknown sort method
            break;
    }

    std::vector<std::string>::const_iterator it;


    double cropFactor = 0;
    double focalLength = 0;

    Lens lens;
    VariableMap vars;
    fillVariableMap(vars);
    ImageOptions imgopts;
    SrcPanoImage srcImg;

    // load additional images...
    for (it = files.begin(); it != files.end(); ++it) {
        const std::string &filename = *it;

        // try to read settings automatically.
        srcImg.setFilename(filename);
        int assumeSimilar = wxConfigBase::Get()->Read(wxT("/LensDefaults/AssumeSimilar"), HUGIN_LENS_ASSUME_SIMILAR);
        if (!assumeSimilar) {
            focalLength = 0;
            cropFactor = 0;
        }
        bool ok = initImageFromFile(srcImg, focalLength, cropFactor);
#if 0
        if (! ok) {
                 // search for image with matching size and exif data
                for (unsigned int i=0; i < pano.getNrOfImages(); i++) {
                    SrcPanoImage other = pano.getSrcImage(i);
                    if ( other.getSize() == srcImg.getSize() )
                        /*  this exif data is currently not saved in the Panorama object and the .pto files.
                            do not check it.
                         &&
                         other.getExifModel() == srcImg.getExifModel() &&
                         other.getExifMake()  == srcImg.getExifMake() &&
                         other.getExifFocalLength() == srcImg.getExifFocalLength() &&
                         other.getExifCropFactor() == srcImg.getExifCropFactor()
                       )
                        */
                    {
                        srcImg = pano.getSrcImage(i);
                        srcImg.setFilename(filename);
                        ok = true;
                        break;
                    }
                }
            }
        }
#endif
        // if no similar image found, ask user
        if (! ok) {
            getLensDataFromUser(srcImg, focalLength, cropFactor);
        }

        if( srcImg.getSize().x == 0) {
            // if image size is invalid, do not add image.
            pano.changeFinished();
            wxLogError(_("Could not read image size"));
            return;
        }

        int matchingLensNr=-1;
        // FIXME: check if the exif information
        // indicates other camera parameters
        for (unsigned int i=0; i < pano.getNrOfImages(); i++) {
            SrcPanoImage other = pano.getSrcImage(i);
            if (abs(other.getHFOV () - srcImg.getHFOV()) < 1
                && other.getSize() == srcImg.getSize()
                /*
                && other.getExifModel() == srcImg.getExifModel()
                && other.getExifMake()  == srcImg.getExifMake() */
               )
            {
                matchingLensNr = pano.getImage(i).getLensNr();
                break;
            }
        }

        if (matchingLensNr == -1) {
            // create and add new lens
            Lens lens;
            matchingLensNr = pano.addLens(lens);
        }
        PanoImage img(filename, srcImg.getSize().x, srcImg.getSize().y, (unsigned int) matchingLensNr);
        int i = pano.addImage(img, vars);
        pano.setSrcImage(i, srcImg);
    }
    pano.changeFinished();
}


void wxLoadPTProjectCmd::execute()
{
    PanoCommand::execute();
    PanoramaMemento newPano;
    if (newPano.loadPTScript(in,prefix)) {
        pano.setMemento(newPano);
        unsigned int nImg = pano.getNrOfImages();
        wxString basedir;
        double focalLength=0;
        double cropFactor=0;
        for (unsigned int i = 0; i < nImg; i++) {
            wxFileName fname(wxString (pano.getImage(i).getFilename().c_str(), *wxConvCurrent));
            while (! fname.FileExists()){
                        // Is file in the new path
                if (basedir != wxT("")) {
                    DEBUG_DEBUG("Old filename: " << pano.getImage(i).getFilename());
                    std::string fn = utils::stripPath(pano.getImage(i).getFilename());
                    DEBUG_DEBUG("Old filename, without path): " << fn);
                    wxString newname(fn.c_str(), *wxConvCurrent);
                            // GetFullName does only work with local paths (not compatible across platforms)
//                            wxString newname = fname.GetFullName();
                    fname.AssignDir(basedir);
                    fname.SetFullName(newname);
                    DEBUG_TRACE("filename with new path: " << fname.GetFullPath().mb_str());
                    if (fname.FileExists()) {
                        pano.setImageFilename(i, (const char *)fname.GetFullPath().mb_str());
                        DEBUG_TRACE("New filename set: " << fname.GetFullPath().mb_str());
                                // TODO - set pano dirty flag so that new paths are saved
                        continue;
                    }
                }

                wxMessageBox(wxString::Format(_("Image file not found:\n%s\nPlease select correct image"), fname.GetFullPath().c_str()), _("Image file not found"));

                if (basedir == wxT("")) {
                    basedir = fname.GetPath();
                }
                        // open file dialog
                wxString wildcard (_("All Image files|*.jpg;*.JPG;*.tif;*.TIF;*.tiff;*.TIFF;*.png;*.PNG;*.bmp;*.BMP;*.gif;*.GIF;*.pnm;*.PNM;*.sun;*.viff;*.hdr|JPEG files (*.jpg)|*.jpg;*.JPG|All files (*)|*"));
                wxFileDialog dlg(MainFrame::Get(), _("Add images"),
                                 basedir, fname.GetFullName(),
                                 wildcard, wxOPEN, wxDefaultPosition);
                if (dlg.ShowModal() == wxID_OK) {
                    pano.setImageFilename(i, (const char *)dlg.GetPath().mb_str());
                            // save used path
                    basedir = dlg.GetDirectory();
                    DEBUG_INFO("basedir is: " << basedir.mb_str());
                } else {
                    PanoramaMemento emptyPano;
                    pano.setMemento(emptyPano);
                            // set an empty panorama
                    pano.changeFinished();
                    return;
                }
                fname.Assign(dlg.GetPath());
            }
            // check if image size is correct
            SrcPanoImage srcImg = pano.getSrcImage(i);
            //
            vigra::ImageImportInfo imginfo(srcImg.getFilename().c_str());
            if (srcImg.getSize() != imginfo.size()) {
                // adjust size properly.
                srcImg.resize(imginfo.size());
                pano.setSrcImage(i, srcImg);
            }
            // check if script contains invalid HFOV
            unsigned lNr = pano.getImage(i).getLensNr();
            Lens cLens = pano.getLens(lNr);
            double hfov = const_map_get(pano.getVariables()[i], "v").getValue();
            if (cLens.getProjection() == Lens::RECTILINEAR
                && hfov >= 180)
            {
                // something is wrong here, try to read from exif data
                bool ok = initImageFromFile(srcImg, focalLength, cropFactor);
                if (! ok) {
                    getLensDataFromUser(srcImg, focalLength, cropFactor);
                }
                pano.setSrcImage(i, srcImg);
            }
        }
    } else {
        DEBUG_ERROR("could not load panotools script");
    }
    pano.changeFinished();
}




} // namespace

