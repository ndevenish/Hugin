// -*- c-basic-offset: 4 -*-
/**  @file FindPanoDialog.h
 *
 *  @brief Definition of FindPano class
 *
 *  @author Thomas Modes
 *
 */

/*  This is free software; you can redistribute it and/or
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

#ifndef _FINDPANODIALOG_H
#define _FINDPANODIALOG_H

#include <vector>
#include "panoinc_WX.h"
#include "panoinc.h"
#include "BatchFrame.h"

extern "C"
{
    #include "tiff.h"
    #include "tiffio.h"
}

class PossiblePano;

/** Dialog for finding panorama in given directory
 *
 * The algorithm transverse all directories for suitable image files (currently only jpeg and tiff)
 * If it found images, it compares EXIF information to deduce which images could belong
 * to a panorama.
 * After it the user can select which panoramas should created and added to detection queue
 * 
 */
class FindPanoDialog : public wxDialog
{
public:
    /** Constructor, read from xrc ressource; restore last uses settings, size and position */
    FindPanoDialog(BatchFrame *batchframe,wxString xrcPrefix);
    /** destructor, saves size and position */
    ~FindPanoDialog();

protected:
	/** closes window */
	void OnButtonClose(wxCommandEvent & e);
    /** select directory with dialog */
    void OnButtonChoose(wxCommandEvent & e);
    /** start/stops detections */
    void OnButtonStart(wxCommandEvent & e);
    /** add selected projects to queue */
    void OnButtonSend(wxCommandEvent & e);
    /** prevents closing window when running detection */
    void OnClose(wxCloseEvent &e);

private:
    BatchFrame *m_batchframe;
    wxButton *m_button_start;
    wxButton *m_button_choose;
    wxButton *m_button_send;
    wxButton *m_button_close;
    wxTextCtrl *m_textctrl_dir;
    wxCheckBox *m_cb_subdir;
    wxStaticText *m_statustext;
    wxCheckListBox *m_list_pano;
    wxChoice *m_cb_naming;

    std::vector<PossiblePano*> m_panos;
    wxString m_start_dir;
    bool m_isRunning;
    bool m_stopped;
    TIFFErrorHandler m_oldtiffwarning;

    void EnableButtons(const bool state);
    void SearchInDir(wxString dirstring,bool includeSubdir);
    void CleanUpPanolist();
    DECLARE_EVENT_TABLE()
};

struct SortFilename
{
    bool operator() (const SrcPanoImage* img1, const SrcPanoImage* img2);
};

typedef set<SrcPanoImage*,SortFilename> ImageSet;

class PossiblePano
{
    public:
        /** enumeration for different naming conventions, must be match combobox in ressource */
        enum NamingConvention
        {
            NAMING_PANO=0,
            NAMING_FIRST_LAST=1,
            NAMING_FOLDER=2
        };
        /** destructor, cleans up used variables */
        ~PossiblePano();
        /** return true, if the image could belong to the given PossiblePano,
          * it checks camera maker and model, focal length, image size and date/time */
        bool BelongsTo(SrcPanoImage* img,const wxTimeSpan max_time_diff);
        /** adds the given SrcPanoImage to this pano-group */
        void AddSrcPanoImage(SrcPanoImage* img);
        /** returns number of images in this group */
        const unsigned int GetImageCount() const { return m_images.size(); };
        /** returns a string which contains description of this pano */
        const wxString GetItemString(const wxString BasePath) const;
        /** returns a string with the filename of the first and last file */
        const wxString GetFilestring(const wxString BasePath, const bool stripExtension=false) const;
        /** generates the panorama file from this set of images 
          * @return the generated project file, or wxEmptyString if generation failed */
        wxString GeneratePanorama(NamingConvention nc);

    private:
        /** does some reformating date/time format */
        const wxDateTime GetDateTime(const SrcPanoImage* img);
        /** returns a given filename, which does not already exists */
        bool GetNewProjectFilename(NamingConvention nc,const wxString basePath, wxFileName &projectFile);

        std::string m_make;
        std::string m_camera;
        wxDateTime m_dt_start;
        wxDateTime m_dt_end;
        double m_focallength;
        vigra::Size2D m_size;
        ImageSet m_images;
};

#endif //_FINDPANODIALOG_H
