// -*- c-basic-offset: 4 -*-

/** @file PTStitcherPanel.cpp
 *
 *  @brief implementation of PTStitcherPanel Class
 *
 *  @author Kai-Uwe Behrmann <web@tiscali.de> and
 *          Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PTStitcherPanel.cpp 1880 2007-01-24 19:16:59Z dangelo $
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



bool PTStitcherPanel::Stitch(const Panorama & pano,
                             PanoramaOptions opts_)
{
    PanoramaOptions opts(opts_);

    /*
    // work around a bug in PTStitcher, which doesn't
    // allow multilayer tif files to end with .tif
    if ( opts.outputFormat == PanoramaOptions::TIFF_m
         || opts.outputFormat == PanoramaOptions::TIFF_mask )
    {
        opts.outfile = stripExtension(opts.outfile);
    }
    */

    opts.outfile = stripExtension(opts.outfile);
    // check if the path contains a .
    if (opts.outfile.find('.') != std::string::npos) {
	int r = wxMessageBox(_("PTStitcher does not support output filenames that include a dot character (.).\nPlease save your projects in a directory without dot in the pathname.\n\nDo you want to continue anyway?"),
			     _("PTStitcher problem"),
			     wxYES_NO);
	if (r == wxNO) {
	    return false;
	}
    }
    
#if __unix__ || WIN32
    if ( opts.outputFormat == PanoramaOptions::QTVR ) {
        wxMessageBox(_("PTStitcher.exe does not support QTVR output on Windows and Linux"), _("PTStitcher note"));
        return false;
    }

#endif
	UIntSet imgs;
	if (wxConfigBase::Get()->Read(wxT("/General/UseOnlySelectedImages"),
		                          HUGIN_USE_SELECTED_IMAGES))
	{
		// use only selected images.
		imgs = pano.getActiveImages();
	} else {
        fill_set(imgs, 0, pano.getNrOfImages()-1);
	}


    // get PTStitcher executable
    wxConfigBase* config = wxConfigBase::Get();
#ifdef __WXMSW__
    wxString stitcherExe = config->Read(wxT("/PanoTools/PTStitcherExe"),wxT(HUGIN_PT_STITCHER_EXE));
    if (!wxFile::Exists(stitcherExe)){
        wxFileDialog dlg(this,_("Select PTStitcher.exe"),
        wxT(""), wxT("PTStitcher.exe"),
        _("Executables (*.exe)|*.exe"),
        wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            stitcherExe = dlg.GetPath();
            config->Write(wxT("/PanoTools/PTStitcherExe"),stitcherExe);
        } else {
            wxLogError(_("No PTStitcher.exe selected"));
        }
    }
#elif (defined __WXMAC__)
    wxString stitcherExe = config->Read(wxT("/PanoTools/PTStitcherExe"),wxT(HUGIN_PT_SCRIPTFILE));    
    // TODO: for now, default path triggers non-custom path but to be fixed
    if(stitcherExe == wxT(HUGIN_PT_SCRIPTFILE))
        DEBUG_ASSERT(false); //shouldn't get here
    else if (!wxFile::Exists(stitcherExe)) {
        wxFileDialog dlg(this,_("Select PTStitcher commandline tool"),
                         wxT(""), wxT(""),
                         wxT("Any Files |*"),
                         wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            stitcherExe = dlg.GetPath();
            config->Write(wxT("/PanoTools/PTStitcherExe"),stitcherExe);
        } else {
            wxLogError(_("No PTStitcher commandline tool selected"));
            return false;
      }
    }
#else
    wxString stitcherExe = config->Read(wxT("/PanoTools/PTStitcherExe"),wxT(HUGIN_PT_STITCHER_EXE));
#endif

    // display warning about PTmender
    if (stitcherExe.Lower().Contains(wxT("ptmender")) && (opts.outputFormat != PanoramaOptions::TIFF_m
        || opts.colorCorrection != PanoramaOptions::NONE))
    {
        int ret = wxMessageBox(_("Warning: PTmender (from panotools 2.9 or later) only supports TIFF_m output.\nFor color and brightness correction, the resulting tiff files must be processed with PTblender.\n\nContinue anyway?"),
            _("PTmender creates only plain TIFF_m files"), wxYES_NO | wxICON_EXCLAMATION);
        if (ret !=wxYES) {
            return false;
        }
    }

    wxString PTScriptFile = config->Read(wxT("/PanoTools/ScriptFile"),wxT(HUGIN_PT_SCRIPTFILE));
    stringstream script_stream;
    pano.printStitcherScript(script_stream, opts, imgs);
    std::string script(script_stream.str());
    if (m_editScriptCB->IsChecked()) {
        // open a text dialog with an editor inside
        wxDialog * edit_dlg = wxXmlResource::Get()->LoadDialog(this, wxT("edit_script_dialog"));
        DEBUG_ASSERT(edit_dlg);
        wxTextCtrl *txtCtrl=XRCCTRL(*edit_dlg,"script_edit_text",wxTextCtrl);
        DEBUG_ASSERT(txtCtrl);
        txtCtrl->SetValue(wxString(script.c_str(), *wxConvCurrent));

        if (edit_dlg->ShowModal() == wxID_OK) {
            script = txtCtrl->GetValue().mb_str();
        } else {
            script = script_stream.str();
        }
    } else {
        script = script_stream.str();
    }

    // start PTStitcher process

    std::ofstream scriptfile(PTScriptFile.mb_str());
    if (!scriptfile.good()) {
        DEBUG_FATAL("could not open/create PTScript file");
    }
    scriptfile << script;
    scriptfile.close();

    wxString outputFN = wxQuoteFilename(wxString(opts.outfile.c_str(), *wxConvCurrent));
    wxString args =  wxString(wxT(" -o "));
    args = args + outputFN;
    args = args + wxString(wxT(" ")) + wxQuoteFilename(PTScriptFile);

    DEBUG_INFO("Executing cmd: " << stitcherExe.mb_str() << " " << args.mb_str());
    MyExecuteCommandOnDialog(stitcherExe, args, this);
    return true;
}

