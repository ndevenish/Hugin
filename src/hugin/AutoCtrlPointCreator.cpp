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
using namespace utils;

void AutoCtrlPointCreator::readUpdatedControlPoints(const std::string & file,
                                                    std::map<int,int> & imgMapping,
                                                    PT::Panorama & pano)
{
    ifstream stream(file.c_str());
    if (! stream.is_open()) {
        DEBUG_ERROR("Could not open autopano output: " << file);
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
            if (!getParam(t, line, "t")) {
                t = 0;
            }
            point.mode = (ControlPoint::OptimizeMode) t;
            ctrlPoints.push_back(point);
        } else {
            DEBUG_DEBUG("skipping line: " << line);
        }
    }

    wxString msg;
    wxMessageBox(wxString::Format(_("Added %d control points"), ctrlPoints.size()), _("Autopano result"));
    GlobalCmdHist::getInstance().addCommand(
        new PT::AddCtrlPointsCmd(pano, ctrlPoints)
        );
}


void AutoCtrlPointCreator::automatch(Panorama & pano, 
		                     const UIntSet & imgs,
				     int nFeatures)
{
    int t = wxConfigBase::Get()->Read("/AutoPano/Type",-1);
    if (t < 0) {

	wxString tmp[2];
    	tmp[0] = _("Autopano (version 1.03 or greater), from http://autopano.kolor.com");
    	tmp[1] = _("Autopano-Sift, from http://user.cs.tu-berlin.de/~nowozin/autopano-sift/");
    	// determine autopano type
    	wxSingleChoiceDialog d(NULL,  _("Choose which autopano program should be used\n"), _("Select autopano type"),
	   	 	       2, tmp, NULL);

	if (d.ShowModal() == wxID_OK) {
	    t = d.GetSelection();
	} else {
	    return;
	}
    }
    switch (t) {
	case 0:
	{
	    // autopano@kolor
	    AutoPanoKolor matcher;
	    matcher.automatch(pano, imgs, nFeatures);
	    break;
	}
	case 1:
	{
	    // autopano-sift
	    AutoPanoSift matcher;
	    matcher.automatch(pano, imgs, nFeatures);
	    break;
	}
	default:
	    DEBUG_ERROR("Invalid autopano type, delete registry if needed");
    }
    wxConfigBase::Get()->Write("/AutoPano/Type",t);

}

void AutoPanoSift::automatch(Panorama & pano, const UIntSet & imgs,
                                     int nFeatures)
{
    // create suitable command line..

#ifdef __WXMSW__
    wxString autopanoExe = wxConfigBase::Get()->Read("/AutoPanoSift/AutopanoExe","");
    if (!wxFile::Exists(autopanoExe)){
        wxFileDialog dlg(0,_("Select autopano program / frontend script"),
                         "", "",
                         "Executables (*.exe,*.vbs,*.cmd)|*.exe;*.vbs;*.cmd",
                         wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            autopanoExe = dlg.GetPath();
            wxConfigBase::Get()->Write("/Autopano/AutoPanoSiftExe",autopanoExe);
        } else {
            wxLogError(_("No autopano selected"));
            return;
        }
    }
#elif defined (__WXMAC__)
    wxString autopanoExe = wxConfigBase::Get()->Read("/AutoPanoSift/AutopanoExe","");
    if (!wxFile::Exists(autopanoExe)){
        wxFileDialog dlg(0,_("Select autopano-sift frontend script"),
                         "", "",
                         "Shell Scripts (*.sh)|*.sh",
                         wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            autopanoExe = dlg.GetPath();
            wxConfigBase::Get()->Write("/Autopano/AutoPanoSiftExe",autopanoExe);
        } else {
            wxLogError(_("No autopano selected"));
            return;
        }
    }
#else
    // todo: selection of autopano on linux..
    wxString autopanoExe = wxConfigBase::Get()->Read("/AutoPanoSift/AutopanoExe","autopano-complete.sh");
#endif

    wxString autopanoArgs = wxConfigBase::Get()->Read("/AutoPanoSift/Args"," -o %o -p %p %i");

    // build a list of all image files, and a corrosponding connection map.
    // local img nr -> global (panorama) img number
    std::map<int,int> imgMapping;
    string imgFiles;
    int imgNr=0;
    for(UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); it++)
    {
        imgMapping[imgNr] = *it;
        imgFiles.append(" ").append(quoteFilename(pano.getImage(*it).getFilename()));
        imgNr++;
    }

    wxString ptofile("autopano_result_tempfile.pto");
    autopanoArgs.Replace("%o", ptofile);
    wxString tmp;
    tmp.Printf("%d", nFeatures);
    autopanoArgs.Replace("%p", tmp);
    autopanoArgs.Replace("%i", imgFiles.c_str());
    wxString cmd;
    cmd.Printf("%s %s", autopanoExe.c_str(), autopanoArgs.c_str());
#ifdef __WXMSW__
    if (cmd.size() > 1950) {
        wxMessageBox(_("autopano command line too long.\n"
                       "This is a windows limitation\n"
                       "Please select less images, or place the images in a folder with\n"
                       "a shorter pathname"), _("Too many images selected"),
                     wxCANCEL );
        return;
    }
#endif
    DEBUG_DEBUG("Executing: " << cmd.c_str());
    
    wxProgressDialog progress(_("Running autopano"),_("Please wait while autopano searches control points\nSee the command window for autopanos' progress"));
    // run autopano in an own output window

#ifdef unix
    DEBUG_DEBUG("using system() to execute autopano-sift");
    int ret = system(cmd);
    if (ret == -1) {
	perror("system() failed");
    } else {
	ret = WEXITSTATUS(ret);
    }
#else
    int ret = wxExecute(cmd, wxEXEC_SYNC);
#endif

    if (ret == -1) {
        wxMessageBox( _("Could not execute command: " + cmd), _("wxExecute Error"));
        return;
    } else if (ret > 0) {
        wxMessageBox(_("command: ") + cmd +
                     _("\nfailed with error code: ") + wxString::Format("%d",ret),
		     _("wxExecute Error"),
                     wxCANCEL );
        return;
    }

    if (! wxFileExists(ptofile.c_str())) {
        wxMessageBox(wxString(_("Could not open ")) + ptofile + _(" for reading\n"
                       "This is an indicator that the autopano call failed,\n"
                       "or wrong command line parameters have been used.\n\n"
                       "Autopano command: ") + cmd, _("autopano failure"), wxCANCEL );
        return;
    }
    // read and update control points
    readUpdatedControlPoints(ptofile.c_str(), imgMapping, pano);

    if (!wxRemoveFile(ptofile)) {
        DEBUG_DEBUG("could not remove temporary file: " << ptofile.c_str());
    }
}


void AutoPanoKolor::automatch(Panorama & pano, const UIntSet & imgs,
                              int nFeatures)
{
#ifdef __WXMSW__
    wxString autopanoExe = wxConfigBase::Get()->Read("/AutoPanoKolor/AutopanoExe","");
    if (!wxFile::Exists(autopanoExe)){
        wxFileDialog dlg(0,_("Select autopano program / frontend script"),
                         "", "",
                         "Executables (*.exe,*.vbs,*.cmd)|*.exe;*.vbs;*.cmd",
                         wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            autopanoExe = dlg.GetPath();
            wxConfigBase::Get()->Write("/AutoPanoKolor/AutopanoExe",autopanoExe);
        } else {
            wxLogError(_("No autopano selected"));
            return;
        }
    }
#else
    // todo: selection of autopano on linux..
    wxString autopanoExe = wxConfigBase::Get()->Read("/AutoPanoKolor/AutopanoExe","autopano");
#endif

    // write default autopano.kolor.com flags
    wxString autopanoArgs = wxConfigBase::Get()->Read("/AutoPanoKolor/Args"," /path:%d /keys:%p /project:oto /name:%o /size:1024 /f %i");
    
    // build a list of all image files, and a corrosponding connection map.
    // local img nr -> global (panorama) img number
    std::map<int,int> imgMapping;
    string imgFiles;
    int imgNr=0;
    for(UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); it++)
    {
        imgMapping[imgNr] = *it;
        imgFiles.append(" ").append(quoteFilename(pano.getImage(*it).getFilename()));
        imgNr++;
    }

    wxString ptofile("autopano_result_tempfile");
    autopanoArgs.Replace("%o", ptofile);
    wxString tmp;
    tmp.Printf("%d", nFeatures);
    autopanoArgs.Replace("%p", tmp);
    autopanoArgs.Replace("%i", imgFiles.c_str());

    wxString tempdir = wxConfigBase::Get()->Read("tempDir","");
    autopanoArgs.Replace("%d", tempdir);
    wxString cmd;
    cmd.Printf("%s %s", autopanoExe.c_str(), autopanoArgs.c_str());
#ifdef __WXMSW__
    if (cmd.size() > 1950) {
        wxMessageBox(_("autopano command line too long.\n"
                       "This is a windows limitation\n"
                       "Please select less images, or place the images in a folder with\n"
                       "a shorter pathname"), _("Too many images selected"),
                     wxCANCEL );
        return;
    }
#endif
    DEBUG_DEBUG("Executing: " << cmd.c_str());
    
    wxProgressDialog progress(_("Running autopano"),_("Please wait while autopano searches control points\nSee the command window for autopanos' progress"));
    // run autopano in an own output window
#ifdef unix
    DEBUG_DEBUG("using system() to execute autopano");
    int ret = system(cmd);
    if (ret == -1) {
	perror("system() failed");
    } else {
	ret = WEXITSTATUS(ret);
    }
#else
    int ret = wxExecute(cmd, wxEXEC_SYNC);
#endif

    if (ret == -1) {
        wxMessageBox( _("Could not execute command: " + cmd), _("wxExecute Error"));
        return;
    } else if (ret > 0) {
        wxMessageBox(_("command: ") + cmd +
                     _("\nfailed with error code: ") + wxString::Format("%d",ret),
		     _("wxExecute Error"),
                     wxCANCEL );
        return;
    }

    ptofile.append("0.oto");
    if (! wxFileExists(ptofile.c_str()) ) {
        wxMessageBox(wxString(_("Could not open ")) + ptofile + _(" for reading\n"
                       "This is an indicator that the autopano call failed,\n"
                       "or wrong command line parameters have been used.\n\n"
                       "Autopano command: ") + cmd + "\n current directory:" + 
			wxGetCwd(),
		       _("autopano failure"), wxCANCEL );
        return;
    }
    // read and update control points
    readUpdatedControlPoints(ptofile.c_str(), imgMapping, pano);

    if (!wxRemoveFile(ptofile)) {
        DEBUG_DEBUG("could not remove temporary file: " << ptofile.c_str());
    }
}


