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

#ifdef __WXMAC__
#include <CFBundle.h>
#include <wx/utils.h>
#endif

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
    int t = wxConfigBase::Get()->Read(wxT("/AutoPano/Type"),HUGIN_AP_TYPE);
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
    wxConfigBase::Get()->Write(wxT("/AutoPano/Type"),t);

}

void AutoPanoSift::automatch(Panorama & pano, const UIntSet & imgs,
                                     int nFeatures)
{
    // create suitable command line..

#ifdef __WXMSW__
    wxString autopanoExe = wxConfigBase::Get()->Read(wxT("/AutoPanoSift/AutopanoExe"), wxT(HUGIN_APSIFT_EXE));
    if (!wxFile::Exists(autopanoExe)){
        wxFileDialog dlg(0,_("Select autopano program / frontend script"),
                         wxT(""), wxT("autopano-win32.exe"),
                         _("Executables (*.exe)|*.exe"),
                         wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            autopanoExe = dlg.GetPath();
            wxConfigBase::Get()->Write(wxT("/AutopanoSift/AutopanoExe"),autopanoExe);
        } else {
            wxLogError(_("No autopano selected"));
            return;
        }
    }
#elif (defined __WXMAC__)
    wxString autopanoExe;
    
    CFBundleRef mainbundle = CFBundleGetMainBundle();
    if(!mainbundle)
    {
        DEBUG_INFO("Mac: Not bundled");
    }
    else
    {
        CFURLRef XRCurl = CFBundleCopyResourceURL(mainbundle, CFSTR(HUGIN_APSIFT_EXE), NULL, NULL);
        if(!XRCurl)
        {
            DEBUG_INFO("Mac: Cannot locate autopano-sift frontend script in the bundle.");
        }
        else
        {
            CFIndex bufLen = 1024;
            unsigned char buffer[(int) bufLen];
            if(!CFURLGetFileSystemRepresentation(XRCurl, TRUE, buffer, bufLen))
            {
                CFRelease(XRCurl);
                DEBUG_INFO("Mac: Failed to get file system representation");
            }
            else
            {
                buffer[((int) bufLen) - 1] = '\0';
                CFRelease(XRCurl);
                autopanoExe = wxString::FromAscii( (char *) buffer);
                DEBUG_INFO("Mac: using bundled autopano-sift frontend script");
                
                wxConfigBase::Get()->Write(wxT("/AutopanoSift/AutopanoExe"), wxT(HUGIN_APSIFT_EXE));
            }
        }
    }
    
    if(!autopanoExe)
    {
        autopanoExe = wxConfigBase::Get()->Read(wxT("/AutoPanoSift/AutopanoExe"), wxT(HUGIN_APSIFT_EXE));
        if (!wxFile::Exists(autopanoExe)){
            wxFileDialog dlg(0,_("Select autopano frontend script"),
                             wxT(""), wxT(""),
                             _("Shell Scripts (*.sh)|*.sh"),
                             wxOPEN, wxDefaultPosition);
            if (dlg.ShowModal() == wxID_OK) {
                autopanoExe = dlg.GetPath();
                wxConfigBase::Get()->Write(wxT("/AutopanoSift/AutopanoExe"), autopanoExe);
            } else {
                wxLogError(_("No autopano selected"));
                return;
            }
        }
    }
#else
    // autopano should be in the path on linux
    wxString autopanoExe = wxConfigBase::Get()->Read(wxT("/AutoPanoSift/AutopanoExe"),wxT(HUGIN_APSIFT_EXE));
#endif

    wxString autopanoArgs = wxConfigBase::Get()->Read(wxT("/AutoPanoSift/Args"),
                                                      wxT(HUGIN_APSIFT_ARGS));

#ifdef __WXMSW__
	// remember cwd.
	wxString cwd = wxGetCwd();
	wxString apDir = wxPathOnly(autopanoExe);
	if (apDir.Length() > 0) {
		wxSetWorkingDirectory(apDir);
	}
#endif

    wxString ptofile(wxT("autopano_result_tempfile.pto"));
    autopanoArgs.Replace(wxT("%o"), ptofile);
    wxString tmp;
    tmp.Printf(wxT("%d"), nFeatures);
    autopanoArgs.Replace(wxT("%p"), tmp);

    // build a list of all image files, and a corrosponding connection map.
    // local img nr -> global (panorama) img number
    std::map<int,int> imgMapping;

    long idx = autopanoArgs.Find(wxT("%namefile")) ;
    DEBUG_DEBUG("find %namefile in '"<< autopanoArgs.mb_str() << "' returned: " << idx);
    bool use_namefile = idx >=0;
    idx = autopanoArgs.Find(wxT("%i"));
    DEBUG_DEBUG("find %i in '"<< autopanoArgs.mb_str() << "' returned: " << idx);
    bool use_params = idx >=0;
    if (use_namefile && use_params) {
        wxMessageBox(_("Please use either %namefile or %i in the autopano-sift command line."),
                     _("Error in Autopano command"), wxOK | wxICON_ERROR);
        return;
    }
    if ((! use_namefile) && (! use_params)) {
        wxMessageBox(_("Please use  %namefile or %i to specify the input files for autopano-sift"),
                     _("Error in Autopano command"), wxOK | wxICON_ERROR);
        return;
    }

    wxFile namefile;
    if (use_namefile) {
        // create temporary file with image names.
        wxString fname = wxFileName::CreateTempFileName(wxT("ap_imgnames"), &namefile);
        DEBUG_DEBUG("before replace %namefile: " << autopanoArgs.mb_str());
        autopanoArgs.Replace(wxT("%namefile"), fname);
        DEBUG_DEBUG("after replace %namefile: " << autopanoArgs.mb_str());
        int imgNr=0;
        for(UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); it++)
        {
            imgMapping[imgNr] = *it;
            namefile.Write(wxString(pano.getImage(*it).getFilename().c_str(), *wxConvCurrent));
            namefile.Write(wxT("\r\n"));
            imgNr++;
        }
    } else {
        string imgFiles;
        int imgNr=0;
        for(UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); it++)
        {
            imgMapping[imgNr] = *it;
            imgFiles.append(" ").append(quoteFilename(pano.getImage(*it).getFilename()));
            imgNr++;
        }
        autopanoArgs.Replace(wxT("%i"), wxString (imgFiles.c_str(), *wxConvCurrent));
    }
    
#ifdef __WXMAC__
    wxString autopanoExeDir = wxConfigBase::Get()->Read(wxT("/AutoPanoSift/AutopanoExeDir"), wxT(""));
    if (! wxFileExists( autopanoExeDir + wxT("/autopano.exe") )){
        wxFileDialog dlg(0, _("Select autopano .Net executable."),
                         wxT(""), wxT(""),
                         wxT("Mono executables (*.exe)|*.exe"),
                         wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            autopanoExeDir = wxPathOnly( dlg.GetPath() );
            wxConfigBase::Get()->Write(wxT("/AutopanoSift/AutopanoExeDir"), autopanoExeDir);
            autopanoArgs = wxT("-a ") + autopanoExeDir + wxT(" ") + autopanoArgs;
        } else {
            wxLogError(_("No autopano directory selected"));
            return;
        }
    }
#endif
    
#ifdef __WXMSW__
    if (autopanoArgs.size() > 1930) {
        wxMessageBox(_("autopano command line too long.\nThis is a windows limitation\nPlease select less images, or place the images in a folder with\na shorter pathname"),
                     _("Too many images selected"),
                     wxCANCEL | wxICON_ERROR );
        return;
    }
#endif

    wxString cmd = autopanoExe + wxT(" ") + autopanoArgs;
    DEBUG_DEBUG("Executing: " << cmd.c_str());

    wxProgressDialog progress(_("Running autopano"),_("Please wait while autopano searches control points\nSee the command window for autopanos' progress"));
    // run autopano in an own output window

    int ret = 0;
#ifdef unix
    DEBUG_DEBUG("using system() to execute autopano-sift");
    ret = system(cmd.mb_str());
    if (ret == -1) {
	perror("system() failed");
    } else {
	ret = WEXITSTATUS(ret);
    }
#elif WIN32
    wxFileName tname(autopanoExe);
    wxString ext = tname.GetExt();
    if (ext == wxT("vbs")) {
#if wxUSE_UNICODE
        wxChar * exe_c = (wxChar *)autopanoExe.c_str();
#else //ANSI
        char * exe_c = (char *) autopanoExe.mb_str();
#endif
        SHELLEXECUTEINFO seinfo;
        memset(&seinfo, 0, sizeof(SHELLEXECUTEINFO));
        seinfo.cbSize = sizeof(SHELLEXECUTEINFO);
        seinfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        seinfo.lpFile = exe_c;
        seinfo.lpParameters = autopanoArgs.c_str();
        if (!ShellExecuteEx(&seinfo)) {
            ret = -1;
            wxMessageBox(_("Could not execute command: ") + cmd, _("ShellExecuteEx failed"), wxCANCEL | wxICON_ERROR);
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
        wxMessageBox( _("Could not execute command: " + cmd), _("wxExecute Error"), wxOK | wxICON_ERROR);
        return;
    } else if (ret > 0) {
        wxMessageBox(_("command: ") + cmd +
                     _("\nfailed with error code: ") + wxString::Format(wxT("%d"),ret),
		     _("wxExecute Error"),
                     wxOK | wxICON_ERROR);
        return;
    }

    if (! wxFileExists(ptofile.c_str())) {
        wxMessageBox(wxString(_("Could not open ")) + ptofile + _(" for reading\nThis is an indicator that the autopano call failed,\nor wrong command line parameters have been used.\n\nAutopano command: ")
                     + cmd, _("autopano failure"), wxOK | wxICON_ERROR );
        return;
    }
    // read and update control points
    readUpdatedControlPoints((const char *)ptofile.mb_str(), pano);

#ifdef __WXMSW__
	// set old cwd.
	wxSetWorkingDirectory(cwd);
#endif


    if (!wxRemoveFile(ptofile)) {
        DEBUG_DEBUG("could not remove temporary file: " << ptofile.c_str());
    }
}


void AutoPanoKolor::automatch(Panorama & pano, const UIntSet & imgs,
                              int nFeatures)
{
#ifdef __WXMSW__
    wxString autopanoExe = wxConfigBase::Get()->Read(wxT("/AutoPanoKolor/AutopanoExe"), wxT(HUGIN_APKOLOR_EXE));
    if (!wxFile::Exists(autopanoExe)){
        wxFileDialog dlg(0,_("Select autopano program / frontend script"),
                         wxT(""), wxT("autopano.exe"),
                         _("Executables (*.exe,*.vbs,*.cmd)|*.exe;*.vbs;*.cmd"),
                         wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            autopanoExe = dlg.GetPath();
            wxConfigBase::Get()->Write(wxT("/AutoPanoKolor/AutopanoExe"),autopanoExe);
        } else {
            wxLogError(_("No autopano selected"));
            return;
        }
    }
#else
    // todo: selection of autopano on linux..
    wxString autopanoExe = wxConfigBase::Get()->Read(wxT("/AutoPanoKolor/AutopanoExe"),wxT(HUGIN_APKOLOR_EXE));
#endif

    // write default autopano.kolor.com flags
    wxString autopanoArgs = wxConfigBase::Get()->Read(wxT("/AutoPanoKolor/Args"),
                                                      wxT(HUGIN_APKOLOR_ARGS));

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

    wxString ptofile(wxT("autopano_result_tempfile"));
    autopanoArgs.Replace(wxT("%o"), ptofile);
    wxString tmp;
    tmp.Printf(wxT("%d"), nFeatures);
    autopanoArgs.Replace(wxT("%p"), tmp);
    autopanoArgs.Replace(wxT("%i"), wxString (imgFiles.c_str(), *wxConvCurrent));

    wxString tempdir = huginApp::Get()->GetWorkDir();
    autopanoArgs.Replace(wxT("%d"), tempdir);
    wxString cmd;
    cmd.Printf(wxT("%s %s"), autopanoExe.c_str(), autopanoArgs.c_str());
#ifdef __WXMSW__
    if (cmd.size() > 1950) {
        wxMessageBox(_("autopano command line too long.\nThis is a windows limitation\nPlease select less images, or place the images in a folder with\na shorter pathname"),
                     _("Too many images selected"),
                     wxCANCEL );
        return;
    }
#endif
    DEBUG_DEBUG("Executing: " << cmd.c_str());

    wxProgressDialog progress(_("Running autopano"),_("Please wait while autopano searches control points\nSee the command window for autopanos' progress"));
    // run autopano in an own output window
#ifdef unix
    DEBUG_DEBUG("using system() to execute autopano");
    int ret = system(cmd.mb_str());
    if (ret == -1) {
	perror("system() failed");
    } else {
	ret = WEXITSTATUS(ret);
    }
#else
    int ret = wxExecute(cmd, wxEXEC_SYNC);
#endif

    if (ret == -1) {
        wxMessageBox( _("Could not execute command: " + cmd), _("wxExecute Error"),
                      wxOK | wxICON_ERROR);
        return;
    } else if (ret > 0) {
        wxMessageBox(_("command: ") + cmd +
                     _("\nfailed with error code: ") + wxString::Format(wxT("%d"),ret),
		     _("wxExecute Error"),
                     wxOK | wxICON_ERROR);
        return;
    }

    ptofile.append(wxT("0.oto"));
    if (! wxFileExists(ptofile.c_str()) ) {
        wxMessageBox(wxString(_("Could not open ")) + ptofile + _(" for reading\nThis is an indicator that the autopano call failed,\nor wrong command line parameters have been used.\n\nAutopano command: ")
                     + cmd + _("\n current directory:") +
			         wxGetCwd(),
		             _("autopano failure"), wxCANCEL );
        return;
    }
    // read and update control points
    readUpdatedControlPoints((const char *)ptofile.mb_str(), pano);

    if (!wxRemoveFile(ptofile)) {
        DEBUG_DEBUG("could not remove temporary file: " << ptofile.c_str());
    }
}


