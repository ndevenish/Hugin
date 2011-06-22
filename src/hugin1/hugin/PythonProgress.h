// -*- c-basic-offset: 4 -*-
/** @file PythonProgress.h
 *
  *  @brief declaration of Python progress window
 *
 *  @author T. Modes
 *
 */

/*
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
#ifndef PYTHONPROGRESS_H
#define PYTHONPROGRESS_H

#include "panoinc_WX.h"
#include "panoinc.h"

#if wxCHECK_VERSION(2,9,0)
    wxDECLARE_EVENT(EVT_PYTHON_FINISHED,wxCommandEvent);
#else
#if _WINDOWS && defined Hugin_shared
    DECLARE_LOCAL_EVENT_TYPE(EVT_PYTHON_FINISHED,-1)
#else
    DECLARE_EVENT_TYPE(EVT_PYTHON_FINISHED,-1)
#endif
#endif

/** The progress window for running Python scripts
 */
class PythonProgress : public wxDialog, public wxThreadHelper
{
public:
    /** constructor, builds windows, initialize some variables 
     */
    PythonProgress(wxWindow* parent, PT::Panorama &pano, wxString scriptfile);
    /** destructor */
    ~PythonProgress();
    /** starts the Python script 
      *  @returns true, if thread was started successful 
      */
    bool RunScript();
    /** returns the modified PanoramaMemento */
    const HuginBase::PanoramaMemento GetPanoramaMemento();
protected:
    /** event handler for close button */
    void OnCloseButton(wxCommandEvent &e);
    /** prevent closing window if thread is running */
    void OnClose(wxCloseEvent &e);
    /** notify event when Python has finished */
    void OnPythonFinished(wxCommandEvent &e);
    /** main Python calling */
    virtual wxThread::ExitCode Entry();

    // panorama object and critical section to lock if Python is running
    HuginBase::Panorama & m_pano;
    wxCriticalSection m_panoCS;
    wxString m_scriptfile;
private:
    wxStreamToTextRedirector* m_redirect;
    wxButton* m_closeButton;
    wxCheckBox* m_closeWhenDone;
    bool m_success;

    DECLARE_EVENT_TABLE()
};

/** Python progress window similiar to PythonProgress, passes selected images to Python */
class PythonWithImagesProgress:public PythonProgress
{
public:
    /** constructor, builds windows, initialize some variables 
     */
    PythonWithImagesProgress(wxWindow* parent, PT::Panorama &pano, HuginBase::UIntSet images, wxString scriptfile);
protected:
    /** main Python calling */
    virtual wxThread::ExitCode Entry();
private:
    HuginBase::UIntSet m_images;
};

#endif // PYTHONPROGRESS_H
