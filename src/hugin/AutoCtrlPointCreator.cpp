// -*- c-basic-offset: 4 -*-

/** @file AutoCtrlPointCreator.cpp
 *
 *  @brief implementation of AutoCtrlPointCreator Class
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

#include "panoinc_WX.h"
#include "panoinc.h"

#include <fstream>

#include "PT/Panorama.h"

#include "hugin/AutoCtrlPointCreator.h"
#include "hugin/CommandHistory.h"

using namespace std;
using namespace PT;

void AutoCtrlPointCreator::readUpdatedControlPoints(const std::string & file,
                                                    std::map<int,int> & imgMapping,
                                                    PT::Panorama & pano)
{
    ifstream stream(file.c_str());
    if (! stream.is_open()) {
        DEBUG_ERROR("Could not open autopano output: pano0/pano0.pto");
        return;
    }

    CPVector ctrlPoints;
    string line;
    while(!stream.eof()) {
        std::getline(stream, line);

        if (line.size() > 0 && line[0] == 'c') {
            int t;
            ControlPoint point;
            getParam(point.image1Nr, line, "n");
            point.image1Nr = imgMapping[point.image1Nr];
            getParam(point.image2Nr, line, "N");
            point.image2Nr = imgMapping[point.image2Nr];
            getParam(point.x1, line, "x");
            getParam(point.x2, line, "X");
            getParam(point.y1, line, "y");
            getParam(point.y2, line, "Y");
            getParam(t, line, "t");
            point.mode = (ControlPoint::OptimizeMode) t;
            ctrlPoints.push_back(point);
        } else {
            DEBUG_DEBUG("skipping line: " << line);
        }
    }

    GlobalCmdHist::getInstance().addCommand(
        new PT::AddCtrlPointsCmd(pano, ctrlPoints)
        );
}


void AutoPanoSift::automatch(Panorama & pano, const UIntSet & imgs,
                                     int nFeatures)
{
    // create suitable command line..

#ifdef __WXMSW__
    DEBUG_ERROR("automatch does not work under windows yet");
    wxString autopanoExe = wxConfigBase::Get()->Read("/PanoTools/AutopanoExe","autopano.exe");
    if (!wxFile::Exists(autopanoExe)){
        wxFileDialog dlg(0,_("Select startscript for autopano-sift"),
                         "", "autopano-complete",
                         "Executables (*.bat)|*.bat",
                         wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            autopanoExe = dlg.GetPath();
            wxConfigBase::Get()->Write("/PanoTools/AutopanoExe",autopanoExe);
        } else {
            wxLogError(_("No autopano.exe selected (download it from http://autopano.kolor.com)"));
            return;
        }
    }
#else
    wxString autopanoExe = wxConfigBase::Get()->Read("/PanoTools/AutopanoExe","autopano-complete.sh");
#endif

    // strings for sebastians autopano.
    wxString autopanoArgs = wxConfigBase::Get()->Read("/PanoTools/AutopanoArgs","-s 1000 -o %o -p %p %i");

    // build a list of all image files, and a corrosponding connection map.
    // local img nr -> global (panorama) img number
    std::map<int,int> imgMapping;
    string imgFiles;
    int imgNr=0;
    for(UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); it++)
    {
        imgMapping[imgNr] = *it;
        imgFiles.append(" ").append(pano.getImage(*it).getFilename().c_str());
        imgNr++;
    }

    autopanoArgs.Replace("%o", "autopano.pto");
    wxString tmp;
    tmp.Printf("%d", nFeatures);
    autopanoArgs.Replace("%p", tmp);
    autopanoArgs.Replace("%i", imgFiles.c_str());
    wxString cmd;
    cmd.Printf("%s %s",autopanoExe.c_str(), autopanoArgs.c_str());
    DEBUG_DEBUG("Executing: " << cmd.c_str());
    // run autopano in an own output window
    wxShell(cmd);

    // read and update control points
    readUpdatedControlPoints("autopano.pto", imgMapping, pano);
}
