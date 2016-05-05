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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
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
    FindPanoDialog(BatchFrame* batchframe,wxString xrcPrefix);
    /** destructor, saves size and position */
    ~FindPanoDialog();

protected:
    /** closes window */
    void OnButtonClose(wxCommandEvent& e);
    /** select directory with dialog */
    void OnButtonChoose(wxCommandEvent& e);
    /** start/stops detections */
    void OnButtonStart(wxCommandEvent& e);
    /** add selected projects to queue */
    void OnButtonSend(wxCommandEvent& e);
    /** prevents closing window when running detection */
    void OnClose(wxCloseEvent& e);
    /** event to populate information on the right */
    void OnSelectPossiblePano(wxCommandEvent &e);
    /** event handler for context menu */
    void OnListItemRightClick(wxListEvent &e);
    /** event handler to remove selected image from list */
    void OnRemoveImage(wxCommandEvent &e);
    /** event handler to split into 2 panos */
    void OnSplitPanos(wxCommandEvent &e);

private:
    BatchFrame* m_batchframe;
    wxButton* m_button_start;
    wxButton* m_button_choose;
    wxButton* m_button_send;
    wxButton* m_button_close;
    wxTextCtrl* m_textctrl_dir;
    wxCheckBox* m_cb_subdir;
    wxStaticText* m_statustext;
    wxCheckListBox* m_list_pano;
    wxChoice* m_ch_naming;
    wxCheckBox* m_cb_createLinks;
    wxCheckBox* m_cb_loadDistortion;
    wxCheckBox* m_cb_loadVignetting;
    wxSpinCtrl* m_sc_minNumberImages;
    wxSpinCtrl* m_sc_maxTimeDiff;
    wxImageList* m_thumbs;
    wxListCtrl* m_thumbsList;
    wxChoice* m_ch_blender;

    std::vector<PossiblePano*> m_panos;
    wxString m_start_dir;
    bool m_isRunning;
    bool m_stopped;
    TIFFErrorHandler m_oldtiffwarning;

    void EnableButtons(const bool state);
    void SearchInDir(wxString dirstring, const bool includeSubdir, const bool loadDistortion, const bool loadVignetting, const size_t minNumberImages, const size_t maxTimeDiff);
    void CleanUpPanolist();
    DECLARE_EVENT_TABLE()
};

struct SortFilename
{
    bool operator() (const HuginBase::SrcPanoImage* img1, const HuginBase::SrcPanoImage* img2);
};

typedef std::set<HuginBase::SrcPanoImage*, SortFilename> ImageSet;

class PossiblePano
{
public:
    /** enumeration for different naming conventions, must be match combobox in ressource */
    enum NamingConvention
    {
        NAMING_PANO=0,
        NAMING_FIRST_LAST=1,
        NAMING_FOLDER=2,
        NAMING_TEMPLATE=3
    };
    /** constructor, init values */
    PossiblePano() : m_focallength(0) {};
    /** destructor, cleans up used variables */
    ~PossiblePano();
    /** return true, if the image could belong to the given PossiblePano,
      * it checks camera maker and model, focal length, image size and date/time */
    bool BelongsTo(HuginBase::SrcPanoImage* img, const wxTimeSpan max_time_diff);
    /** adds the given SrcPanoImage to this pano-group */
    void AddSrcPanoImage(HuginBase::SrcPanoImage* img);
    /** returns number of images in this group */
    const unsigned int GetImageCount() const
    {
        return m_images.size();
    };
    /** returns a string which contains description of this pano */
    const wxString GetItemString(const wxString BasePath) const;
    /** returns a string with the filename of the first and last file */
    const wxString GetFilestring(const wxString BasePath, const bool stripExtension=false) const;
    /** generates the panorama file from this set of images
      * @return the generated project file, or wxEmptyString if generation failed */
    wxString GeneratePanorama(NamingConvention nc, bool createLinks, HuginBase::PanoramaOptions::BlendingMechanism defaultBlender);
    /** returns the camera name */
    wxString GetCameraName();
    /** returns the lens name */
    wxString GetLensName();
    /** returns the focal length as string */
    wxString GetFocalLength();
    /** return the start date/time as string */
    wxString GetStartString();
    /** returns the duration as string */
    wxString GetDuration();
    /** add all images to wxListCtrl */
    void PopulateListCtrl(wxListCtrl* list, wxImageList* thumbs);
    /** removes the image at given index */
    void RemoveImage(const unsigned int index);
    /** split pano into 2 subpanos, index is used as first image of second pano
      @returns pointer to second subpano */
    PossiblePano* SplitPano(const unsigned int index);

private:
    /** does some reformating date/time format */
    const wxDateTime GetDateTime(const HuginBase::SrcPanoImage* img);
    /** returns a given filename, which does not already exists */
    bool GetNewProjectFilename(NamingConvention nc,const wxString basePath, wxFileName& projectFile);
    /** updates the internal date/time representations */
    void UpdateDateTimes();

    std::string m_make;
    std::string m_camera;
    std::string m_lens;
    wxDateTime m_dt_start;
    wxDateTime m_dt_end;
    double m_focallength;
    vigra::Size2D m_size;
    ImageSet m_images;
};

#endif //_FINDPANODIALOG_H
