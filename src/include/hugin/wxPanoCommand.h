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
#include "hugin/ImageCache.h"

namespace PT {

/** add image(s) to a panorama */
class wxAddImagesCmd : public PanoCommand
{
public:
    wxAddImagesCmd(Panorama & pano, const std::vector<std::string> & files)
    : PanoCommand(pano), files(files)
    { };
    virtual void execute()
        {
            PanoCommand::execute();

            // FIXME make it possible to add other lenses,
            // for example if the exif data or image size
            // suggests it.
            std::vector<std::string>::const_iterator it;
            for (it = files.begin(); it != files.end(); ++it) {
                const std::string &filename = *it;
                std::string::size_type idx = filename.rfind('.');
                if (idx == std::string::npos) {
                    DEBUG_DEBUG("could not find extension in filename");
                }
                wxImage * image = ImageCache::getInstance().getImage(filename);
                std::string ext = filename.substr( idx+1 );


                int width = image->GetWidth();
                int height = image->GetHeight();

                Lens lens;
                lens.isLandscape = (width > height);
                if (lens.isLandscape) {
                    lens.setRatio(((double)width)/height);
                } else {
                    lens.setRatio(((double)height)/width);
                }

                if (utils::tolower(ext) == "jpg") {
                    // try to read exif data from jpeg files.
                    lens.readEXIF(filename);
                }

                int matchingLensNr=-1;
                // FIXME: check if the exif information
                // indicates other camera parameters
                for (unsigned int lnr=0; lnr < pano.getNrOfLenses(); lnr++) {
                    const Lens & l = pano.getLens(lnr);

                    // use a lens if hfov and ratio are the same
                    // should add a check for exif camera information as
                    // well.
                    if ((l.getRatio() == lens.getRatio()) &&
                        (l.isLandscape == lens.isLandscape) &&
                        (const_map_get(l.variables,"v").getValue() - const_map_get(lens.variables,"v").getValue() < 0.002) )
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
                PanoImage img(filename, width, height, (unsigned int) matchingLensNr);
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
                        wxString wildcard (_("Image files (*.jpg)|*.jpg;*.JPG|"
                                             "Image files (*.png)|*.png;*.PNG|"
                                             "Image files (*.tif)|*.tif;*.TIF|"
                                             "All files (*.*)|*.*"));
                        wxFileDialog dlg(MainFrame::Get(), _("Add images"),
                                         basedir, fname.GetName(),
                                         wildcard, wxOPEN, wxDefaultPosition);
                        if (dlg.ShowModal() == wxID_OK) {
                            pano.setImageFilename(i, (const char *)dlg.GetPath().mb_str());
                            // save used path
                            basedir = dlg.GetDirectory();
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
