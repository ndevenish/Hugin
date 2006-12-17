// -*- c-basic-offset: 4 -*-
/** @file wxPanoCommand.h
 *
 *  wxwindows specific panorama commands
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _WXPANOCOMMAND__H
#define _WXPANOCOMMAND__H

#include "PT/PanoCommand.h"
#include "common/stl_utils.h"
#include "vigra/impex.hxx"
#include "hugin/LensPanel.h"
#include "hugin/config_defaults.h"

namespace PT {

struct FileIsNewer: public std::binary_function<const std::string &, const std::string &, bool>
{

    bool operator()(const std::string & file1, const std::string & file2)
    {
        // lets hope the operating system caches files stats.
        return wxFileModificationTime(wxString(file1.c_str(),*wxConvCurrent)) < wxFileModificationTime(wxString(file2.c_str(),*wxConvCurrent));
    };

};

/** add image(s) to a panorama */
class wxAddImagesCmd : public PanoCommand
{
public:
    wxAddImagesCmd(Panorama & pano, const std::vector<std::string> & newfiles)
    : PanoCommand(pano), files(newfiles)
    { };
    virtual void execute()
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


            double cropFactor=0;

            bool sameSettings=false;

            Lens lens;
            double roll=0;
            VariableMap vars;
            fillVariableMap(vars);
            ImageOptions imgopts;

            for (it = files.begin(); it != files.end(); ++it) {
                const std::string &filename = *it;

                initLensFromFile(filename, cropFactor, lens, vars, imgopts, sameSettings);
                if( lens.getImageSize().x == 0) {
                    // if image size is invalid, do not add image.
                    pano.changeFinished();
                    wxLogError(_("Could not read image size"));
                    return;
                }
                if (!lens.m_hasExif) {
                    sameSettings = true;
                }

                int matchingLensNr=-1;
                // FIXME: check if the exif information
                // indicates other camera parameters
                for (unsigned int lnr=0; lnr < pano.getNrOfLenses(); lnr++) {
                    const Lens & l = pano.getLens(lnr);

                    // use a lens if hfov and ratio are the same
                    // should add a check for exif camera information as
                    // well.
                    double l_v = const_map_get(l.variables,"v").getValue();
                    double lens_v = const_map_get(lens.variables,"v").getValue();
                    if ((l.getAspectRatio() == lens.getAspectRatio()) &&
                        (l.isLandscape() == lens.isLandscape()) &&
                        (fabs(l_v - lens_v) < 0.001) &&
                        (l.getSensorSize() == lens.getSensorSize()))
                    {
                        matchingLensNr= lnr;
                    }
                }

                if (matchingLensNr == -1) {
                    matchingLensNr = pano.addLens(lens);
                }

                DEBUG_ASSERT(matchingLensNr >= 0);
                PanoImage img(filename, lens.getImageSize().x, lens.getImageSize().y, (unsigned int) matchingLensNr);
                img.setOptions(imgopts);
                pano.addImage(img, vars);
            }
            pano.changeFinished();
        }
    virtual std::string getName() const
        {
            return "add images";
        }
private:
    std::vector<std::string> files;
};



/** dump the current project and load a new one.
 *
 */
class wxLoadPTProjectCmd : public PanoCommand
{
public:
    wxLoadPTProjectCmd(Panorama & p, std::istream & i, const std::string & prefix = "")
        : PanoCommand(p),
          in(i),
          prefix(prefix)
        { }

    virtual void execute()
        {
            PanoCommand::execute();
            PanoramaMemento newPano;
            if (newPano.loadPTScript(in,prefix)) {
                pano.setMemento(newPano);
                unsigned int nImg = pano.getNrOfImages();
                wxString basedir;
                double defaultCropFactor = -1;
                double defaultHFOV = -1;
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
                        pano.setSrcImg(i, srcImg);
                    }
                    // check if script contains invalid HFOV
                    unsigned lNr = pano.getImage(i).getLensNr();
                    Lens cLens = pano.getLens(lNr);
                    double roll = 0;
                    double hfov = const_map_get(pano.getVariables()[i], "v").getValue();
                    if (cLens.getProjection() == Lens::RECTILINEAR
                        && hfov >= 180)
                    {
                        // try to load hfov from exif info
                        VariableMap vars;
                        fillVariableMap(vars);
                        ImageOptions imgopts;
                        initLensFromFile(pano.getImage(i).getFilename(),
                                         defaultCropFactor, cLens, vars, imgopts, false);
                        if ( cLens.getImageSize().x == 0)
                        {
                            // if image size is invalid, abort script reading
                            PanoramaMemento emptyPano;
                            pano.setMemento(emptyPano);
                            pano.changeFinished();
                            wxLogError(wxString::Format(_("Could not read image size of file %s"),
                                                        pano.getImage(i).getFilename().c_str()) );
                            return;
                        }

                        if (cLens.getHFOV() >= 180 && defaultHFOV <= 0) {
                            // failed to load a better hfov.. Ask user
                            wxString tval(wxT("50"));
                            wxString t = wxGetTextFromUser(wxString::Format(_("Enter focal length for image\n%s\n"), pano.getImage(i).getFilename().c_str()),
                                                           _("Loading project"), tval);
                            t.ToDouble(&defaultHFOV);
                            cLens.setFocalLength(defaultHFOV);
                        }
                        // update lens (and concerning variables) with
                        // useful data...
                        PT::Variable var_v("v", cLens.getHFOV());
                        pano.updateLens(lNr, cLens);
                    }
                }
            } else {
                DEBUG_ERROR("could not load panotools script");
            }
            pano.changeFinished();
        }
        virtual std::string getName() const
            {
                return "load project";
            }
    private:
    std::istream & in;
	const std::string &prefix;
    };

    //=========================================================================
    //=========================================================================


    /** add a control point */
    class wxAddCtrlPointGridCmd : public PanoCommand
    {
    public:
        wxAddCtrlPointGridCmd(Panorama & p, unsigned int i1,
                            unsigned int i2, double scale, double threshold)
            : PanoCommand(p), img1(i1), img2(i2), scale(scale), cornerThreshold(threshold)
            { }

        virtual void execute();

        virtual std::string getName() const
            {
                return "add control point";
            }
    private:
        unsigned int img1,img2,dx,dy;
        double scale;
        double cornerThreshold;
    };


} // namespace PT

#endif // _WXPANOCOMMAND__H
