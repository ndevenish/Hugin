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
#include "hugin/LensPanel.h"
#include "hugin/config_defaults.h"

namespace PT {

struct FileIsNewer: public std::binary_function<const std::string &, const std::string &, bool>
{

    bool operator()(const std::string & file1, const std::string & file2)
    {
        // lets hope the operating system caches files stats.
        return wxFileModificationTime(wxString(file1.c_str(),*wxConvCurrent)) > wxFileModificationTime(wxString(file2.c_str(),*wxConvCurrent));
    };

};

/** add image(s) to a panorama */
class wxAddImagesCmd : public PanoCommand
{
public:
    wxAddImagesCmd(Panorama & pano, const std::vector<std::string> & newfiles)
    : PanoCommand(pano), newfiles(newfiles)
    { };
    virtual void execute()
        {
            PanoCommand::execute();

            std::vector<std::string> files = newfiles;
            // check if the files should be sorted by date
            long sortbydate = wxConfigBase::Get()->Read(wxT("General/SortNewImgByDate"), HUGIN_GUI_SORT_IMG_BY_DATE);
            if (sortbydate) {
                std::sort(newfiles.begin(), newfiles.end(), FileIsNewer());
            }
            
            std::vector<std::string>::const_iterator it;


            double cropFactor=0;

            for (it = files.begin(); it != files.end(); ++it) {
                const std::string &filename = *it;

                Lens lens;
                initLensFromFile(filename, cropFactor, lens);
                if( lens.getImageSize().x == 0) {
                    // if image size is invalid, do not add image.
                    pano.changeFinished();
                    wxLogError(_("Could not read image size"));
                    return;
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

                VariableMap vars;
                fillVariableMap(vars);

                DEBUG_ASSERT(matchingLensNr >= 0);
                PanoImage img(filename, lens.getImageSize().x, lens.getImageSize().y, (unsigned int) matchingLensNr);
                pano.addImage(img, vars);
            }
            pano.changeFinished();
        }
    virtual std::string getName() const
        {
            return "add images";
        }
private:
    std::vector<std::string> newfiles;
};



/** dump the current project and load a new one.
 *
 *  Use this for  style projects.
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
                for (unsigned int i = 0; i < nImg; i++) {
                    wxFileName fname(wxString (pano.getImage(i).getFilename().c_str(), *wxConvCurrent));
                    while (! fname.FileExists()){
                        wxMessageBox(wxString::Format(_("Image file not found:\n%s\nPlease select correct image"), fname.GetFullPath().c_str()), _("Image file not found"));

                        if (basedir == wxT("")) {
                            basedir = fname.GetPath();
                        }
                        // open file dialog
                        wxString wildcard (_("Image files (*.jpg)|*.jpg;*.JPG|Image files (*.png)|*.png;*.PNG|Image files (*.tif)|*.tif;*.TIF|All files (*.*)|*.*"));
                        wxFileDialog dlg(MainFrame::Get(), _("Add images"),
                                         basedir, fname.GetName(),
                                         wildcard, wxOPEN, wxDefaultPosition);
                        if (dlg.ShowModal() == wxID_OK) {
                            pano.setImageFilename(i, (const char *)dlg.GetPath().mb_str());
                            // save used path
                            basedir = dlg.GetDirectory();
                        } else {
                            PanoramaMemento emptyPano;
                            pano.setMemento(emptyPano);
                            // set an empty panorama
                            pano.changeFinished();
                            return;
                        }
                        fname.Assign(dlg.GetPath());
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

} // namespace PT

#endif // _WXPANOCOMMAND__H
