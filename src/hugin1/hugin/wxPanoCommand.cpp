// -*- c-basic-offset: 4 -*-

/** @file wxPanoCommand.cpp
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
#include "base_wx/wxPlatform.h"

#include <base_wx/wxImageCache.h>
#include <base_wx/platform.h>
#include <hugin/wxPanoCommand.h>
#include <hugin/MainFrame.h>
#include <panodata/OptimizerSwitches.h>

#include <vigra/cornerdetection.hxx>
#include <vigra/localminmax.hxx>
#include <panodata/StandardImageVariableGroups.h>

#include <hugin_utils/alphanum.h>

#ifdef HUGIN_HSI
#include "hugin_script_interface/hpi.h"
#endif

using namespace std;
using namespace vigra;
using namespace hugin_utils;

namespace PT {

bool wxAddCtrlPointGridCmd::processPanorama(Panorama& pano)
{
    // TODO: rewrite, and use same function for modular image creation.
#if 1
    const SrcPanoImage & i1 = pano.getImage(img1);
    const SrcPanoImage & i2 = pano.getImage(img1);

    // run both images through the harris corner detector
    ImageCache::EntryPtr eptr = ImageCache::getInstance().getSmallImage(i1.getFilename());

    vigra::BImage leftImg(eptr->get8BitImage()->size());

    vigra::GreenAccessor<vigra::RGBValue<vigra::UInt8> > ga;
    vigra::copyImage(srcImageRange(*(eptr->get8BitImage()), ga ),
                     destImage(leftImg));

    double scale = i1.getSize().width() / (double) leftImg.width();

    //const vigra::BImage & leftImg = ImageCache::getInstance().getPyramidImage(
    //    i1.getFilename(),1);

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
    // need to scale the images.
    // sample grid on img1 and try to add ctrl points
    for (unsigned int x=0; x < (unsigned int)leftImg.width(); x++ ) {
        for (unsigned int y=0; y < (unsigned int)leftImg.height(); y++) {
            if (leftCorners(x,y) > 0) {
                img1ToSphere.transformImgCoord(sphx, sphy, scale*x, scale*y);
                sphereToImg2.transformImgCoord(img2x, img2y, sphx, sphy);
                // check if it is inside..
                if (   img2x > border && img2x < i2.getWidth() - border
                    && img2y > border && img2y < i2.getHeight() - border )
                {
                    // add control point
                    ControlPoint p(img1, scale*x, scale*y, img2, img2x, img2y);
                    pano.addCtrlPoint(p);
                }
            }
        }
    }
#endif
    return true;
}


bool wxAddImagesCmd::processPanorama(Panorama& pano)
{
    // check if the files should be sorted by date
    long sort = wxConfigBase::Get()->Read(wxT("General/SortNewImgOnAdd"), HUGIN_GUI_SORT_NEW_IMG_ON_ADD);

    switch (sort) {
        case 1:
                // sort by filename
            std::sort(files.begin(), files.end(), doj::alphanum_less());
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
    SrcPanoImage srcImg;
    HuginBase::StandardImageVariableGroups variable_groups(pano);
    HuginBase::ImageVariableGroup & lenses = variable_groups.getLenses();

    // load additional images...
    for (it = files.begin(); it != files.end(); ++it) {
        bool added = false;
        const std::string &filename = *it;
        wxString fname(filename.c_str(), HUGIN_CONV_FILENAME);

        int assumeSimilar = wxConfigBase::Get()->Read(wxT("/LensDefaults/AssumeSimilar"), HUGIN_LENS_ASSUME_SIMILAR);
        if (!assumeSimilar) {
            focalLength = 0;
            cropFactor = 0;
        }

        // try to read settings automatically.
        srcImg.setFilename(filename);
        srcImg.setWhiteBalanceRed(1);
        srcImg.setWhiteBalanceBlue(1);
        bool ok = srcImg.readEXIF(focalLength, cropFactor, true, true);
        if(cropFactor<=0)
        {
            srcImg.readCropfactorFromDB();
            ok=(srcImg.getExifFocalLength()>0 && srcImg.getExifCropFactor()>0);
        };
        if (srcImg.getSize().x == 0 || srcImg.getSize().y == 0) {
            wxMessageBox(wxString::Format(_("Could not decode image:\n%s\nAbort"), fname.c_str()), _("Unsupported image file format"));
            return false;
        }
        try
        {
            vigra::ImageImportInfo info(filename.c_str());
            std::string pixelType=info.getPixelType();
            if((pixelType=="UINT8") || (pixelType=="UINT16") || (pixelType=="INT16"))
                srcImg.setResponseType(HuginBase::SrcPanoImage::RESPONSE_EMOR);
            else
                srcImg.setResponseType(HuginBase::SrcPanoImage::RESPONSE_LINEAR);
        }
        catch(std::exception & e)
        {
            std::cerr << "ERROR: caught exception: " << e.what() << std::endl;
            std::cerr << "Could not get pixel type for file " << filename << std::endl;
        };
        if (! ok && assumeSimilar) {
                 // search for image with matching size and exif data
                 // and re-use it.
                for (unsigned int i=0; i < pano.getNrOfImages(); i++) {
                    SrcPanoImage other = pano.getSrcImage(i);
                    double dummyfl=0;
                    double dummycrop = 0;
                    other.readEXIF(dummyfl, dummycrop, false, false);
                    if ( other.getSize() == srcImg.getSize()
                         &&
                         other.getExifModel() == srcImg.getExifModel() &&
                         other.getExifMake()  == srcImg.getExifMake() &&
                         other.getExifFocalLength() == srcImg.getExifFocalLength()
                       )
                    {
                        double ev = srcImg.getExposureValue();
                        srcImg = pano.getSrcImage(i);
                        srcImg.setFilename(filename);
                        srcImg.deleteAllMasks();
                        srcImg.readEXIF(focalLength, cropFactor, false, false);
                        // add image
                        int imgNr = pano.addImage(srcImg);
                        variable_groups.update();
                        lenses.switchParts(imgNr, lenses.getPartNumber(i));
                        lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_ExposureValue, i);
                        srcImg.setExposureValue(ev);
                        lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_WhiteBalanceRed, i);
                        lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_WhiteBalanceBlue, i);
                        srcImg.setWhiteBalanceRed(1);
                        srcImg.setWhiteBalanceBlue(1);
                        pano.setSrcImage(imgNr, srcImg);
                        added=true;
                        break;
                    }
                }
                if (added) continue;
        }
        int matchingLensNr=-1;
        // if no similar image found, ask user
        if (! ok) {
            srcImg.readProjectionFromDB();
            if (!getLensDataFromUser(MainFrame::Get(), srcImg, focalLength, cropFactor)) {
                // assume a standart lens
                srcImg.setHFOV(50);
                srcImg.setExifCropFactor(1);
            }
        }

        // check the image hasn't disappeared on us since the HFOV dialog was
        // opened
        wxString fn(srcImg.getFilename().c_str(),HUGIN_CONV_FILENAME);
        if (!wxFileName::FileExists(fn)) {
            DEBUG_INFO("Image: " << fn.mb_str() << " has disappeared, skipping...");
            continue;
        }

        // FIXME: check if the exif information
        // indicates this image matches a already used lens
        variable_groups.update();
        double ev = 0;
        bool set_exposure = false;
        for (unsigned int i=0; i < pano.getNrOfImages(); i++) {
            SrcPanoImage other = pano.getSrcImage(i);
            // force reading of exif data, as it is currently not stored in the
            // Panorama data class
            if (other.readEXIF(focalLength, cropFactor, false, false)) {
                if (other.getSize() == srcImg.getSize()
                    && other.getExifModel() == srcImg.getExifModel()
                    && other.getExifMake()  == srcImg.getExifMake()
                    && other.getExifFocalLength() == srcImg.getExifFocalLength()
                   )
                {
                    matchingLensNr = lenses.getPartNumber(i);
                    // copy data from other image, just keep
                    // the file name and reload the exif data (for exposure)
                    ev = srcImg.getExposureValue();
                    set_exposure = true;
                    srcImg = pano.getSrcImage(i);
                    srcImg.setFilename(filename);
                    srcImg.deleteAllMasks();
                    srcImg.readEXIF(focalLength, cropFactor, false, false);
                    srcImg.setExposureValue(ev);
                    break;
                }
            } else if (assumeSimilar) {
                // no exiv information, just check image size.
                if (other.getSize() == srcImg.getSize() ) {
                    matchingLensNr = lenses.getPartNumber(i);
                    // copy data from other image, just keep
                    // the file name
                    srcImg = pano.getSrcImage(i);
                    srcImg.setFilename(filename);
                    srcImg.deleteAllMasks();
                    srcImg.readEXIF(focalLength, cropFactor, false, false);
                    break;
                }
            }
        }

        // If matchingLensNr == -1 still, we haven't found a good lens to use.
        // We shouldn't attach the image to a lens in this case, it will have
        // its own new lens.
        srcImg.readProjectionFromDB();
        int imgNr = pano.addImage(srcImg);
        variable_groups.update();
        if (matchingLensNr != -1)
        {
            lenses.switchParts(imgNr, matchingLensNr);
            // unlink and set exposure value, if wanted.
            if (set_exposure)
            {
                lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_ExposureValue, imgNr);
                lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_WhiteBalanceRed, imgNr);
                lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_WhiteBalanceBlue, imgNr);
                //don't link image size, this will foul the photometric optimizer
                lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_Size, imgNr);
                /// @todo avoid copying the SrcPanoImage.
                SrcPanoImage t = pano.getSrcImage(imgNr);
                t.setExposureValue(ev);
                pano.setSrcImage(imgNr, t);
            }
        }
        if (imgNr == 0) {
            // get initial value for output exposure
            PanoramaOptions opts = pano.getOptions();
            opts.outputExposureValue = srcImg.getExposureValue();
            pano.setOptions(opts);
            // set the exposure, but there isn't anything to link to so don't try unlinking.
            // links are made by default when adding new images.
            if (set_exposure)
            {
                /// @todo avoid copying the SrcPanoImage.
                SrcPanoImage t = pano.getSrcImage(imgNr);
                t.setExposureValue(ev);
                pano.setSrcImage(imgNr, t);
            }
        }
    }
    return true;
}


bool wxLoadPTProjectCmd::processPanorama(Panorama& pano)
{
    PanoramaMemento newPano;
    int ptoVersion = 0;
    std::ifstream in(filename.c_str());
    if (newPano.loadPTScript(in, ptoVersion, prefix))
    {
        pano.setMemento(newPano);
        PanoramaOptions opts = pano.getOptions();
        // always reset to TIFF_m ...
        opts.outputFormat = PanoramaOptions::TIFF_m;
        // get enblend and enfuse options from preferences
        if (ptoVersion < 2)
        {
            // no options stored in file, use default arguments in config file
            opts.enblendOptions = wxConfigBase::Get()->Read(wxT("/Enblend/Args"), wxT(HUGIN_ENBLEND_ARGS)).mb_str(wxConvLocal);
            opts.enfuseOptions = wxConfigBase::Get()->Read(wxT("/Enfuse/Args"), wxT(HUGIN_ENFUSE_ARGS)).mb_str(wxConvLocal);
        }
        // Set the nona gpu flag base on what is in preferences as it is not
        // stored in the file.
        opts.remapUsingGPU = wxConfigBase::Get()->Read(wxT("/Nona/UseGPU"),HUGIN_NONA_USEGPU) == 1;
        pano.setOptions(opts);

        HuginBase::StandardImageVariableGroups variableGroups(pano);
        HuginBase::ImageVariableGroup & lenses = variableGroups.getLenses();

        unsigned int nImg = pano.getNrOfImages();
        wxString basedir;
        double focalLength=0;
        double cropFactor=0;
        bool autopanoSiftFile=false;
        SrcPanoImage autopanoSiftRefImg;
        for (unsigned int i = 0; i < nImg; i++) {
            wxFileName fname(wxString (pano.getImage(i).getFilename().c_str(), HUGIN_CONV_FILENAME));
            while (! fname.FileExists()){
                        // Is file in the new path
                if (basedir != wxT("")) {
                    DEBUG_DEBUG("Old filename: " << pano.getImage(i).getFilename());
                    std::string fn = stripPath(pano.getImage(i).getFilename());
                    DEBUG_DEBUG("Old filename, without path): " << fn);
                    wxString newname(fn.c_str(), HUGIN_CONV_FILENAME);
                            // GetFullName does only work with local paths (not compatible across platforms)
//                            wxString newname = fname.GetFullName();
                    fname.AssignDir(basedir);
                    fname.SetFullName(newname);
                    DEBUG_TRACE("filename with new path: " << fname.GetFullPath().mb_str(wxConvLocal));
                    if (fname.FileExists()) {
                        pano.setImageFilename(i, (const char *)fname.GetFullPath().mb_str(HUGIN_CONV_FILENAME));
                        DEBUG_TRACE("New filename set: " << fname.GetFullPath().mb_str(wxConvLocal));
                                // TODO - set pano dirty flag so that new paths are saved
                        continue;
                    }
                }

                wxMessageBox(wxString::Format(_("Image file not found:\n%s\nPlease select correct image"), fname.GetFullPath().c_str()), _("Image file not found"));

                if (basedir == wxT("")) {
                    basedir = fname.GetPath();
                }

                // open file dialog
                wxFileDialog dlg(MainFrame::Get(), _("Add images"),
                                 basedir, fname.GetFullName(),
                                 HUGIN_WX_FILE_IMG_FILTER, wxFD_OPEN, wxDefaultPosition);
                dlg.SetDirectory(basedir);
                if (dlg.ShowModal() == wxID_OK) {
                    pano.setImageFilename(i, (const char *)dlg.GetPath().mb_str(HUGIN_CONV_FILENAME));
                            // save used path
                    basedir = dlg.GetDirectory();
                    DEBUG_INFO("basedir is: " << basedir.mb_str(wxConvLocal));
                } else {
                    PanoramaMemento emptyPano;
                    pano.setMemento(emptyPano);
                            // set an empty panorama
                    return true;
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
            }
            // check if script contains invalid HFOV
            double hfov = pano.getImage(i).getHFOV();
            if (pano.getImage(i).getProjection() == HuginBase::SrcPanoImage::RECTILINEAR
                && hfov >= 180 && autopanoSiftFile == false)
            {
                autopanoSiftFile = true;
                // something is wrong here, try to read from exif data (all images)
                bool ok = initImageFromFile(srcImg, focalLength, cropFactor, false);
                if (! ok) {
                    getLensDataFromUser(MainFrame::Get(), srcImg, focalLength, cropFactor);
                }
                autopanoSiftRefImg = srcImg;
            } else if (autopanoSiftFile) {
                // need to copy the lens parameters from the first lens.
                srcImg.setHFOV(autopanoSiftRefImg.getHFOV());
            } else {
                // load exif data, but do not apply it
                srcImg.readEXIF(focalLength, cropFactor, false, false);
            }
            pano.setSrcImage(i, srcImg);
        }
        // Link image projection across each lens, since it is not saved.
        for (unsigned int i = 0; i < lenses.getNumberOfParts(); i++)
        {
            // link the variables
            lenses.linkVariablePart(
                                HuginBase::ImageVariableGroup::IVE_Projection,
                                i
                                   );
        }
    } else {
        DEBUG_ERROR("could not load panotools script");
    }
    in.close();

    // Verify control points are valid
    // loop through entire list of points, confirming they are inside the
    // bounding box of their images
    const PT::CPVector & oldCPs = pano.getCtrlPoints();
    PT::CPVector goodCPs;
    int bad_cp_count = 0;
    for (PT::CPVector::const_iterator it = oldCPs.begin();
            it != oldCPs.end(); ++it)
    {
        PT::ControlPoint point = *it;
        const SrcPanoImage & img1 = pano.getImage(point.image1Nr);
        const SrcPanoImage & img2 = pano.getImage(point.image2Nr);
        if (0 > point.x1 || point.x1 > img1.getSize().x ||
            0 > point.y1 || point.y1 > img1.getSize().y ||
            0 > point.x2 || point.x2 > img2.getSize().x ||
            0 > point.y2 || point.y2 > img2.getSize().y)
        {
            bad_cp_count++;
        } else
        {
            goodCPs.push_back(point);
        }
    }

    if (bad_cp_count > 0)
    {
        wxString errMsg = wxString::Format(_("%d invalid control point(s) found.\n\nPress OK to remove."), bad_cp_count);
        wxMessageBox(errMsg, _("Error Detected"), wxICON_ERROR);
        pano.setCtrlPoints(goodCPs);
    }

    // Update control point error values
    HuginBase::PTools::calcCtrlPointErrors(pano);
    if(markAsOptimized)
    {
        pano.markAsOptimized();
    };
    return true;
}

bool wxNewProjectCmd::processPanorama(Panorama& pano)
{
    pano.reset();

    // Setup pano with options from preferences
    PanoramaOptions opts = pano.getOptions();
    wxConfigBase* config = wxConfigBase::Get();
    opts.quality = config->Read(wxT("/output/jpeg_quality"),HUGIN_JPEG_QUALITY);
    switch(config->Read(wxT("/output/tiff_compression"), HUGIN_TIFF_COMPRESSION))
    {
        case 0:
        default:
            opts.outputImageTypeCompression = "NONE";
            opts.tiffCompression = "NONE";
            break;
        case 1:
            opts.outputImageTypeCompression = "PACKBITS";
            opts.tiffCompression = "PACKBITS";
            break;
        case 2:
            opts.outputImageTypeCompression = "LZW";
            opts.tiffCompression = "LZW";
            break;
        case 3:
            opts.outputImageTypeCompression = "DEFLATE";
            opts.tiffCompression = "DEFLATE";
            break;
    }
    switch (config->Read(wxT("/output/ldr_format"), HUGIN_LDR_OUTPUT_FORMAT)) {
    case 1:
        opts.outputImageType ="jpg";
        break;
    case 2:
        opts.outputImageType ="png";
        break;
    case 3:
        opts.outputImageType ="exr";
        break;
    default:
    case 0:
        opts.outputImageType ="tif";
        break;
    }
    // HDR disabled because there is no real choice at the moment:  HDR TIFF is broken and there is only EXR
    // opts.outputImageTypeHDR = config->Read(wxT("/output/hdr_format"), HUGIN_HDR_OUTPUT_FORMAT);
    opts.outputFormat = PanoramaOptions::TIFF_m;
    opts.blendMode = PanoramaOptions::ENBLEND_BLEND;
    opts.enblendOptions = config->Read(wxT("Enblend/Args"),wxT(HUGIN_ENBLEND_ARGS)).mb_str(wxConvLocal);
    opts.enfuseOptions = config->Read(wxT("Enfuse/Args"),wxT(HUGIN_ENFUSE_ARGS)).mb_str(wxConvLocal);
    opts.interpolator = (vigra_ext::Interpolator)config->Read(wxT("Nona/Interpolator"),HUGIN_NONA_INTERPOLATOR);
    opts.remapUsingGPU = config->Read(wxT("Nona/useGPU"),HUGIN_NONA_USEGPU)!=0;
    opts.tiff_saveROI = config->Read(wxT("Nona/CroppedImages"),HUGIN_NONA_CROPPEDIMAGES)!=0;
    opts.hdrMergeMode = PanoramaOptions::HDRMERGE_AVERAGE;
    opts.hdrmergeOptions = HUGIN_HDRMERGE_ARGS;
    pano.setOptions(opts);

    pano.setOptimizerSwitch(HuginBase::OPT_PAIR);
    pano.setPhotometricOptimizerSwitch(HuginBase::OPT_EXPOSURE | HuginBase::OPT_VIGNETTING | HuginBase::OPT_RESPONSE);
    return true;
}


bool wxApplyTemplateCmd::processPanorama(Panorama& pano)
{
    wxConfigBase* config = wxConfigBase::Get();

    if (pano.getNrOfImages() == 0) {
        // TODO: prompt for images!
        wxString path = config->Read(wxT("actualPath"), wxT(""));
        wxFileDialog dlg(MainFrame::Get(), _("Add images"),
                path, wxT(""),
                HUGIN_WX_FILE_IMG_FILTER, wxFD_OPEN|wxFD_MULTIPLE , wxDefaultPosition);
        dlg.SetDirectory(path);

        // remember the image extension
        wxString img_ext;
        if (config->HasEntry(wxT("lastImageType"))){
            img_ext = config->Read(wxT("lastImageType")).c_str();
        }
        if (img_ext == wxT("all images"))
            dlg.SetFilterIndex(0);
        else if (img_ext == wxT("jpg"))
            dlg.SetFilterIndex(1);
        else if (img_ext == wxT("tiff"))
            dlg.SetFilterIndex(2);
        else if (img_ext == wxT("png"))
            dlg.SetFilterIndex(3);
        else if (img_ext == wxT("hdr"))
            dlg.SetFilterIndex(4);
        else if (img_ext == wxT("exr"))
            dlg.SetFilterIndex(5);
        else if (img_ext == wxT("all files"))
            dlg.SetFilterIndex(6);
        DEBUG_INFO ( "Image extention: " << img_ext.mb_str(wxConvLocal) );

        // call the file dialog
        if (dlg.ShowModal() == wxID_OK) {
            // get the selections
            wxArrayString Pathnames;
            dlg.GetPaths(Pathnames);

            // save the current path to config
#ifdef __WXGTK__
            //workaround a bug in GTK, see https://bugzilla.redhat.com/show_bug.cgi?id=849692 and http://trac.wxwidgets.org/ticket/14525
            config->Write(wxT("/actualPath"), wxPathOnly(Pathnames[0]));
#else
            config->Write(wxT("/actualPath"), dlg.GetDirectory());
#endif
            DEBUG_INFO ( wxString::Format(wxT("img_ext: %d"), dlg.GetFilterIndex()).mb_str(wxConvLocal) );
            // save the image extension
            switch ( dlg.GetFilterIndex() ) {
                case 0: config->Write(wxT("lastImageType"), wxT("all images")); break;
                case 1: config->Write(wxT("lastImageType"), wxT("jpg")); break;
                case 2: config->Write(wxT("lastImageType"), wxT("tiff")); break;
                case 3: config->Write(wxT("lastImageType"), wxT("png")); break;
                case 4: config->Write(wxT("lastImageType"), wxT("hdr")); break;
                case 5: config->Write(wxT("lastImageType"), wxT("exr")); break;
                case 6: config->Write(wxT("lastImageType"), wxT("all files")); break;
            }

            HuginBase::StandardImageVariableGroups variable_groups(pano);
            HuginBase::ImageVariableGroup & lenses = variable_groups.getLenses();
            // add images.
            for (unsigned int i=0; i< Pathnames.GetCount(); i++) {
                std::string filename = (const char *)Pathnames[i].mb_str(HUGIN_CONV_FILENAME);
                vigra::ImageImportInfo inf(filename.c_str());
                SrcPanoImage img(filename);
                img.setSize(inf.size());
                int imgNr = pano.addImage(img);
                lenses.updatePartNumbers();
                if (i > 0) lenses.switchParts(imgNr, 0);
            }

        }
    }

    unsigned int nOldImg = pano.getNrOfImages();
    PanoramaMemento newPanoMem;

    int ptoVersion = 0;
    if (newPanoMem.loadPTScript(in, ptoVersion, "")) {
        Panorama newPano;
        newPano.setMemento(newPanoMem);

        unsigned int nNewImg = newPano.getNrOfImages();
        if (nOldImg != nNewImg) {
            wxString errMsg = wxString::Format(_("Error, template expects %d images,\ncurrent project contains %d images\n"), nNewImg, nOldImg);
            wxMessageBox(errMsg, _("Could not apply template"), wxICON_ERROR);
            return false;
        }

        // check image sizes, and correct parameters if required.
        for (unsigned int i = 0; i < nNewImg; i++) {

            // check if image size is correct
            const SrcPanoImage & oldSrcImg = pano.getImage(i);
            SrcPanoImage newSrcImg = newPano.getSrcImage(i);

            // just keep the file name
            DEBUG_DEBUG("apply template fn:" <<  newSrcImg.getFilename() << " real fn: " << oldSrcImg.getFilename());
            newSrcImg.setFilename(oldSrcImg.getFilename());
            if (oldSrcImg.getSize() != newSrcImg.getSize()) {
                // adjust size properly.
                newSrcImg.resize(oldSrcImg.getSize());
            }
            newPano.setSrcImage(i, newSrcImg);
        }
        // keep old control points.
        newPano.setCtrlPoints(pano.getCtrlPoints());
        newPanoMem = newPano.getMemento();
        pano.setMemento(newPanoMem);
    } else {
        wxMessageBox(_("Error loading project file"), _("Could not apply template"), wxICON_ERROR);
    }
    return true;
}

#ifdef HUGIN_HSI
bool PythonScriptPanoCmd::processPanorama(Panorama& pano)
{
    std::cout << "run python script: " << m_scriptFile.c_str() << std::endl;

    int success = hpi::callhpi ( m_scriptFile.c_str() , 1 ,
                   "HuginBase::Panorama*" , &pano ) ;

    if(success!=0)
        wxMessageBox(wxString::Format(wxT("Script returned %d"),success),_("Result"), wxICON_INFORMATION);
    std::cout << "Python interface returned " << success << endl ;
    // notify other of change in panorama
    if(pano.getNrOfImages()>0)
    {
        for(unsigned int i=0;i<pano.getNrOfImages();i++)
        {
            pano.imageChanged(i);
        };
    };

    return true;
}
#endif

} // namespace

