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

#include <config.h>

#include "panoinc_WX.h"
#include "panoinc.h"

#include <fstream>

#include "PT/Panorama.h"

#include "hugin/huginApp.h"
#include "hugin/config_defaults.h"
#include "hugin/AutoCtrlPointCreator.h"
#include "hugin/CommandHistory.h"

using namespace std;
using namespace PT;
using namespace utils;

void AutoCtrlPointCreator::readUpdatedControlPoints(const std::string & file,
                                                    PT::Panorama & pano)
{
    ifstream stream(file.c_str());
    if (! stream.is_open()) {
        DEBUG_ERROR("Could not open autopano output: " << file);
        return;
    }

    Panorama tmpp;
    PanoramaMemento newPano;
    newPano.loadPTScript(stream, "");
    tmpp.setMemento(newPano);

    // create mapping between the panorama images.
    map<unsigned int, unsigned int> imgMapping;
    for (unsigned int ni = 0; ni < tmpp.getNrOfImages(); ni++) {
        std::string nname = stripPath(tmpp.getImage(ni).getFilename());
        for (unsigned int oi=0; oi < pano.getNrOfImages(); oi++) {
            std::string oname = stripPath(pano.getImage(oi).getFilename());
            if (nname == oname) {
                // insert image
                imgMapping[ni] = oi;
                break;
            }
        }
        if (! set_contains(imgMapping, ni)) {
            DEBUG_ERROR("Could not find image " << ni << ", name: " << tmpp.getImage(ni).getFilename() << " in autopano output");
            return;
        }
    }


    // get control points
    CPVector ctrlPoints = tmpp.getCtrlPoints();
    // make sure they are in correct order
    for (CPVector::iterator it= ctrlPoints.begin(); it != ctrlPoints.end(); ++it) {
        (*it).image1Nr = imgMapping[(*it).image1Nr];
        (*it).image2Nr = imgMapping[(*it).image2Nr];
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
    int t = wxConfigBase::Get()->Read("/AutoPano/Type",HUGIN_AP_TYPE);
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
	    DEBUG_ERROR("Invalid autopano type");
    }
    wxConfigBase::Get()->Write("/AutoPano/Type",t);

}

void AutoPanoSift::automatch(Panorama & pano, const UIntSet & imgs,
                                     int nFeatures)
{
    // create suitable command line..

#ifdef __WXMSW__
    wxString autopanoExe = wxConfigBase::Get()->Read("/AutoPanoSift/AutopanoExe", HUGIN_APSIFT_EXE);
    if (!wxFile::Exists(autopanoExe)){
        wxFileDialog dlg(0,_("Select autopano program / frontend script"),
                         "", "Autopano-SIFT-Cmdline.vbs",
                         "Executables (*.exe,*.vbs,*.cmd)|*.exe;*.vbs;*.cmd",
                         wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            autopanoExe = dlg.GetPath();
            wxConfigBase::Get()->Write("/AutopanoSift/AutopanoExe",autopanoExe);
        } else {
            wxLogError(_("No autopano selected"));
            return;
        }
    }
#elif defined (__WXMAC__)
    wxString autopanoExe = wxConfigBase::Get()->Read("/AutoPanoSift/AutopanoExe", HUGIN_APSIFT_EXE);
    if (!wxFile::Exists(autopanoExe)){
        wxFileDialog dlg(0,_("Select autopano-sift frontend script"),
                         "", "",
                         "Shell Scripts (*.sh)|*.sh",
                         wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            autopanoExe = dlg.GetPath();
            wxConfigBase::Get()->Write("/AutopanoSift/AutopanoExe",autopanoExe);
        } else {
            wxLogError(_("No autopano selected"));
            return;
        }
    }
#else
    // autopano should be in the path on linux
    wxString autopanoExe = wxConfigBase::Get()->Read("/AutoPanoSift/AutopanoExe",HUGIN_APSIFT_EXE);
#endif

    wxString autopanoArgs = wxConfigBase::Get()->Read("/AutoPanoSift/Args",
                                                      HUGIN_APSIFT_ARGS);

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
    wxString cmd = autopanoExe + " " + autopanoArgs;
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

    int ret = 0;
#ifdef unix
    DEBUG_DEBUG("using system() to execute autopano-sift");
    ret = system(cmd);
    if (ret == -1) {
	perror("system() failed");
    } else {
	ret = WEXITSTATUS(ret);
    }
#elif WIN32
    wxFileName tname(autopanoExe);
    wxString ext = tname.GetExt();
    if (ext == "vbs") {
        // this is a script.. execute it with ShellExecute
        char * exe_c = (char *)autopanoExe.c_str();
        SHELLEXECUTEINFO seinfo;
        memset(&seinfo, 0, sizeof(SHELLEXECUTEINFO));
        seinfo.cbSize = sizeof(SHELLEXECUTEINFO);
        seinfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        seinfo.lpFile = exe_c;
        seinfo.lpParameters = autopanoArgs.c_str();
        if (!ShellExecuteEx(&seinfo)) {
            ret = -1;
            wxMessageBox(_("Could not execute command: ") + cmd, _("ShellExecuteEx failed"));
        }
        // wait for process
        WaitForSingleObject(seinfo.hProcess, INFINITE);
    } else {
        // normal wxExecute
        ret = wxExecute(cmd, wxEXEC_SYNC);
    }
#else
    ret = wxExecute(cmd, wxEXEC_SYNC);
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
    readUpdatedControlPoints(ptofile.c_str(), pano);

    if (!wxRemoveFile(ptofile)) {
        DEBUG_DEBUG("could not remove temporary file: " << ptofile.c_str());
    }
}


void AutoPanoKolor::automatch(Panorama & pano, const UIntSet & imgs,
                              int nFeatures)
{
#ifdef __WXMSW__
    wxString autopanoExe = wxConfigBase::Get()->Read("/AutoPanoKolor/AutopanoExe", HUGIN_APKOLOR_EXE);
    if (!wxFile::Exists(autopanoExe)){
        wxFileDialog dlg(0,_("Select autopano program / frontend script"),
                         "", "autopano.exe",
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
    wxString autopanoExe = wxConfigBase::Get()->Read("/AutoPanoKolor/AutopanoExe",HUGIN_APKOLOR_EXE);
#endif

    // write default autopano.kolor.com flags
    wxString autopanoArgs = wxConfigBase::Get()->Read("/AutoPanoKolor/Args",
                                                      HUGIN_APKOLOR_ARGS);

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

    wxString tempdir = huginApp::Get()->GetWorkDir();
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
    readUpdatedControlPoints(ptofile.c_str(), pano);

    if (!wxRemoveFile(ptofile)) {
        DEBUG_DEBUG("could not remove temporary file: " << ptofile.c_str());
    }
}


