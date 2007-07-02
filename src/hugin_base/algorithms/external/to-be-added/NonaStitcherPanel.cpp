// -*- c-basic-offset: 4 -*-

/** @file NonaStitcherPanel.cpp
 *
 *  @brief implementation of NonaStitcherPanel Class
 *
 *  @author Kai-Uwe Behrmann <web@tiscali.de> and
 *          Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: NonaStitcherPanel.cpp 1950 2007-04-15 20:51:03Z dangelo $
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

bool NonaStitcherPanel::Stitch( const Panorama & pano,
                                PanoramaOptions options)
{
	// work around a flaw in wxProgresDialog that results in incorrect layout  
	// by pre-allocting sufficient horizontal and vertical space 
    MyProgressDialog pdisp(_("Stitching Panorama"), (wxString((wxChar)' ', 60) + wxT("\n \n ") ), NULL, 
						   wxPD_ELAPSED_TIME | wxPD_AUTO_HIDE | wxPD_APP_MODAL );

    PanoramaOptions opts = options;
    DEBUG_DEBUG("Stitching to " << opts.outfile);

    // set number of threads.
    long nthreads = wxConfigBase::Get()->Read(wxT("/Nona/NumberOfThreads"), wxThread::GetCPUCount());
    if (nthreads < 1) nthreads = 1;
    vigra_ext::ThreadManager::get().setNThreads(nthreads);

    UIntSet imgs;
    if (wxConfigBase::Get()->Read(wxT("/General/UseOnlySelectedImages"),
		                          HUGIN_USE_SELECTED_IMAGES))
    {
        // use only selected images.
        imgs = pano.getActiveImages();
    } else {
        fill_set(imgs, 0, pano.getNrOfImages()-1);
    }

    bool enblend = m_EnblendCheckBox->IsChecked();
    try {
        if (enblend) {
            // output must be set to TIFF
            DEBUG_ASSERT(opts.outputFormat == PanoramaOptions::TIFF);
            // set output to multiple tiff.
            // hope the next enblend will also contain multilayer support
            opts.outputFormat = PanoramaOptions::TIFF_m;

            opts.tiff_saveROI = (wxConfigBase::Get()->Read(wxT("/Enblend/UseCroppedFiles"),
                                  HUGIN_ENBLEND_USE_CROPPED_FILES)) != 0;
        }
        // stitch panorama
        PT::stitchPanorama(pano, opts,
                           pdisp, opts.outfile, imgs);
    } catch (std::bad_alloc &) {
        wxMessageBox(_("Out of memory.\nTry again with a smaller panorama image size\n"),
                     _("Error during stitching"),
                     wxICON_ERROR | wxOK);
        return false;
    } catch (std::exception & e) {
        wxMessageBox(wxString(e.what(), *wxConvCurrent),
                     _("Error during stitching"),
                     wxICON_ERROR | wxOK);
        return false;
    }

	wxString outpath;
	wxString outname;
	wxString output;
	wxFileName::SplitPath(wxString(opts.outfile.c_str(), *wxConvCurrent), &outpath, &outname, NULL);
    output = outpath + wxFileName::GetPathSeparator() + outname;
    if (enblend) {
        wxConfigBase* config = wxConfigBase::Get();
#ifdef __WXMSW__
        wxString enblendExe = config->Read(wxT("/Enblend/EnblendExe"),wxT(HUGIN_ENBLEND_EXE));
        if (!wxFile::Exists(enblendExe)){
            wxFileDialog dlg(this,_("Select enblend.exe"),
                             wxT(""), wxT("enblend.exe"),
                             wxT("Executables (*.exe)|*.exe"),
                              wxOPEN, wxDefaultPosition);
            if (dlg.ShowModal() == wxID_OK) {
                enblendExe = dlg.GetPath();
                config->Write(wxT("/Enblend/EnblendExe"),enblendExe);
            } else {
                wxLogError(_("No enblend.exe selected"));
                return false;
            }
        }
#elif defined __WXMAC__
        wxString enblendExe = config->Read(wxT("/Enblend/EnblendExe"), wxT(HUGIN_ENBLEND_EXE));
        
        // TODO: for now, default path triggers non-custom path but to be fixed
        if(enblendExe == wxT(HUGIN_ENBLEND_EXE))
            enblendExe = MacGetPathTOBundledExecutableFile(CFSTR("enblend"));
        else if (!wxFile::Exists(enblendExe)) {
            wxFileDialog dlg(this,_("Select enblend commandline tool"),
                             wxT(""), wxT(""),
                             wxT("Any Files |*"),
                             wxOPEN, wxDefaultPosition);
            if (dlg.ShowModal() == wxID_OK) {
                enblendExe = dlg.GetPath();
                config->Write(wxT("/Enblend/EnblendExe"),enblendExe);
            } else {
                wxLogError(_("No enblend commandline tool selected"));
                return false;
            }
        }
#else
        wxString enblendExe = config->Read(wxT("/Enblend/EnblendExe"), wxT(HUGIN_ENBLEND_EXE));
#endif
        // call enblend, and create the right output file
        // I hope this works correctly with filenames that contain
        // spaces

        wxString args = config->Read(wxT("/Enblend/EnblendArgs"));
        if (opts.getHFOV() == 360.0) {
            // blend over the border
            args.append(wxT(" -w"));
        }

        if (opts.tiff_saveROI) {
            args.append(wxString::Format(wxT(" -f %dx%d"), opts.getWidth(), opts.getHeight()));
        }

        args.append(wxT(" -o "));
        wxString quoted = output + wxT(".tif");
        quoted = utils::wxQuoteFilename(quoted);
        args.append(quoted);

// DGSW FIXME - Unreferenced
//		unsigned int nImg = imgs.size();
        for (UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); ++it)
        {
            quoted = output + wxString::Format(wxT("%04d.tif"), *it);
            quoted = utils::wxQuoteFilename(quoted);
            args.append(wxT(" "));
            args.append(quoted);
        }

        int ret = -1;

        wxString cmdline = utils::wxQuoteFilename(enblendExe) + wxT(" ") + args;
#ifdef __WXMSW__
        if (cmdline.size() > 32766) {
    wxMessageBox(_("Can not call enblend with a command line > 32766 characters.\nThis is a Windows limitation\nPlease use less images, or place the images in a folder with\na shorter pathname"),
                 _("Too many images selected"));
    return false;
        }
#endif

        {
            // use MyExternalCmdExecDialog
            ret = MyExecuteCommandOnDialog(enblendExe, args, this);
        }
        DEBUG_NOTICE("enblend returned with: " << ret);

        if (ret == -1) {
            wxLogError( _("Could not execute command: ") + cmdline, _("wxExecute Error"));
            return false;
        } else if (ret > 0) {
            wxLogError(_("command: ") + cmdline +
                _("\nfailed with error code: ") + wxString::Format(wxT("%d"),ret),
                _("enblend error"));
            return false;
        }
        if (wxConfigBase::Get()->Read(wxT("/Enblend/DeleteRemappedFiles"), HUGIN_ENBLEND_DELETE_REMAPPED_FILES) != 0) {
            // delete remapped tiff files
            for (UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); ++it)
            {
                wxString f = output + wxString::Format(wxT("%04d.tif"), *it);
                wxRemoveFile(f);
            }
        }
    }
    return true;
}

