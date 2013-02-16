// -*- c-basic-offset: 4 -*-

/** @file PreferencesDialog.cpp
 *
 *  @brief implementation of PreferencesDialog Class
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
#include "wx/listbook.h"
#include "panoinc.h"

#include "base_wx/wxPlatform.h"

#include "hugin/huginApp.h"
#include "hugin/config_defaults.h"
#include "hugin/PreferencesDialog.h"
#include "hugin/CPDetectorDialog.h"
#include "hugin/MainFrame.h"
#include "base_wx/huginConfig.h"

// validators are working different somehow...
//#define MY_STR_VAL(id, filter) { XRCCTRL(*this, "prefs_" #id, wxTextCtrl)->SetValidator(wxTextValidator(filter, &id)); }
//#define MY_SPIN_VAL(id) {     XRCCTRL(*this, "prefs_" #id, wxSpinCtrl)->SetValidator(wxGenericValidator(&id)); }

#define MY_STR_VAL(id, val) { XRCCTRL(*this, id, wxTextCtrl)->SetValue(val); };
#define MY_SPIN_VAL(id, val) { XRCCTRL(*this, id, wxSpinCtrl)->SetValue(val); };
#define MY_BOOL_VAL(id, val) { XRCCTRL(*this, id, wxCheckBox)->SetValue(val); };
#define MY_CHOICE_VAL(id, val) { XRCCTRL(*this, id, wxChoice)->SetSelection(val); };
#define MY_STATIC_VAL(id, val) { XRCCTRL(*this, id, wxStaticText)->SetLabel(val); };

#define MY_G_STR_VAL(id)  XRCCTRL(*this, id, wxTextCtrl)->GetValue()
#define MY_G_SPIN_VAL(id)  XRCCTRL(*this, id, wxSpinCtrl)->GetValue()
#define MY_G_BOOL_VAL(id)  XRCCTRL(*this, id, wxCheckBox)->GetValue()
#define MY_G_CHOICE_VAL(id)  XRCCTRL(*this, id, wxChoice)->GetSelection()



BEGIN_EVENT_TABLE(PreferencesDialog, wxDialog)
    EVT_BUTTON(wxID_OK, PreferencesDialog::OnOk)
    EVT_BUTTON(wxID_HELP, PreferencesDialog::OnHelp)
    EVT_BUTTON(wxID_CANCEL, PreferencesDialog::OnCancel)
    EVT_BUTTON(XRCID("prefs_defaults"), PreferencesDialog::OnRestoreDefaults)
    EVT_BUTTON(XRCID("prefs_enblend_select"), PreferencesDialog::OnEnblendExe)
    EVT_BUTTON(XRCID("prefs_enblend_enfuse_select"), PreferencesDialog::OnEnfuseExe)
    EVT_CHECKBOX(XRCID("prefs_ft_RotationSearch"), PreferencesDialog::OnRotationCheckBox)
    EVT_CHECKBOX(XRCID("prefs_enblend_Custom"), PreferencesDialog::OnCustomEnblend)
    EVT_CHECKBOX(XRCID("prefs_enblend_enfuseCustom"), PreferencesDialog::OnCustomEnfuse)
    EVT_BUTTON(XRCID("pref_cpdetector_new"), PreferencesDialog::OnCPDetectorAdd)
    EVT_BUTTON(XRCID("pref_cpdetector_edit"), PreferencesDialog::OnCPDetectorEdit)
    EVT_BUTTON(XRCID("pref_cpdetector_del"), PreferencesDialog::OnCPDetectorDelete)
    EVT_BUTTON(XRCID("pref_cpdetector_moveup"), PreferencesDialog::OnCPDetectorMoveUp)
    EVT_BUTTON(XRCID("pref_cpdetector_movedown"), PreferencesDialog::OnCPDetectorMoveDown)
    EVT_BUTTON(XRCID("pref_cpdetector_default"), PreferencesDialog::OnCPDetectorDefault)
    EVT_LISTBOX_DCLICK(XRCID("pref_cpdetector_list"), PreferencesDialog::OnCPDetectorListDblClick)
    EVT_BUTTON(XRCID("pref_cpdetector_load"), PreferencesDialog::OnCPDetectorLoad)
    EVT_BUTTON(XRCID("pref_cpdetector_save"), PreferencesDialog::OnCPDetectorSave)
    EVT_CHOICE(XRCID("pref_ldr_output_file_format"), PreferencesDialog::OnFileFormatChanged)
    EVT_CHOICE(XRCID("pref_processor_gui"), PreferencesDialog::OnProcessorChanged)
    EVT_TEXT(XRCID("prefs_project_filename"), PreferencesDialog::OnUpdateProjectFilename)
    EVT_TEXT(XRCID("prefs_output_filename"), PreferencesDialog::OnUpdateOutputFilename)
END_EVENT_TABLE()


PreferencesDialog::PreferencesDialog(wxWindow* parent)
//: wxDialog(parent, -1, _("Preferences - hugin"))
{
    DEBUG_TRACE("");
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("pref_dialog"));

#ifdef __WXMSW__
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);

    // Custom setup ( stuff that can not be done in XRC )
    XRCCTRL(*this, "prefs_ft_RotationStartAngle", wxSpinCtrl)->SetRange(-180,0);
    XRCCTRL(*this, "prefs_ft_RotationStopAngle", wxSpinCtrl)->SetRange(0,180);
    XRCCTRL(*this, "prefs_ass_nControlPoints", wxSpinCtrl)->SetRange(3,3000);

    wxChoice* lang_choice = XRCCTRL(*this, "prefs_gui_language", wxChoice);

#if __WXMAC__
    lang_choice->Disable();
#endif

    // add languages to choice
    long* lp = new long;
    *lp = wxLANGUAGE_DEFAULT;
    lang_choice->Append(_("System default"), lp);
    lp = new long;
    *lp = wxLANGUAGE_BASQUE;
    lang_choice->Append(_("Basque"), lp);
    lp = new long;
    *lp = wxLANGUAGE_BULGARIAN;
    lang_choice->Append(_("Bulgarian"), lp);
    lp = new long;
    *lp = wxLANGUAGE_CATALAN;
    lang_choice->Append(_("Catalan"), lp);
    lp = new long;
    *lp = wxLANGUAGE_CHINESE_SIMPLIFIED;
    lang_choice->Append(_("Chinese (Simplified)"), lp);
    lp = new long;
    *lp = wxLANGUAGE_CHINESE_TRADITIONAL;
    lang_choice->Append(_("Chinese (Traditional)"), lp);
    lp = new long;
    *lp = wxLANGUAGE_CZECH;
    lang_choice->Append(_("Czech"), lp);
    lp = new long;
    *lp = wxLANGUAGE_DANISH;
    lang_choice->Append(_("Danish"), lp);
    lp = new long;
    *lp = wxLANGUAGE_DUTCH;
    lang_choice->Append(_("Dutch"), lp);
    lp = new long;
    *lp = wxLANGUAGE_ENGLISH;
    lang_choice->Append(_("English"), lp);
    lp = new long;
    *lp = wxLANGUAGE_FRENCH;
    lang_choice->Append(_("French"), lp);
    lp = new long;
    *lp = wxLANGUAGE_GERMAN;
    lang_choice->Append(_("German"), lp);
    lp = new long;
    *lp = wxLANGUAGE_HUNGARIAN;
    lang_choice->Append(_("Hungarian"), lp);
    lp = new long;
    *lp = wxLANGUAGE_ITALIAN;
    lang_choice->Append(_("Italian"), lp);
    lp = new long;
    *lp = wxLANGUAGE_JAPANESE;
    lang_choice->Append(_("Japanese"), lp);
    lp = new long;
    *lp = wxLANGUAGE_KOREAN;
    lang_choice->Append(_("Korean"), lp);
    lp = new long;
    *lp = wxLANGUAGE_POLISH;
    lang_choice->Append(_("Polish"), lp);
    lp = new long;
    *lp = wxLANGUAGE_PORTUGUESE_BRAZILIAN;
    lang_choice->Append(_("Portuguese (Brazilian)"), lp);
    lp = new long;
    *lp = wxLANGUAGE_RUSSIAN;
    lang_choice->Append(_("Russian"), lp);
    lp = new long;
    *lp = wxLANGUAGE_SLOVAK;
    lang_choice->Append(_("Slovak"), lp);
    lp = new long;
    *lp = wxLANGUAGE_SLOVENIAN;
    lang_choice->Append(_("Slovenian"), lp);
    lp = new long;
    *lp = wxLANGUAGE_SPANISH;
    lang_choice->Append(_("Spanish"), lp);
    lp = new long;
    *lp = wxLANGUAGE_SWEDISH;
    lang_choice->Append(_("Swedish"), lp);
    lp = new long;
    *lp = wxLANGUAGE_UKRAINIAN;
    lang_choice->Append(_("Ukrainian"), lp);
    lp = new long;
    *lp = wxLANGUAGE_FINNISH;
    lang_choice->Append(_("Finnish"), lp);
    lang_choice->SetSelection(0);

#if wxCHECK_VERSION(2,9,1)
    wxStaticText* preview=XRCCTRL(*this, "prefs_project_filename_preview", wxStaticText);
    preview->SetWindowStyle(preview->GetWindowStyle() | wxST_ELLIPSIZE_START);
    preview=XRCCTRL(*this, "prefs_output_filename_preview", wxStaticText);
    preview->SetWindowStyle(preview->GetWindowStyle() | wxST_ELLIPSIZE_START);
#endif
    // load autopano settings
    wxConfigBase* cfg = wxConfigBase::Get();
    m_CPDetectorList = XRCCTRL(*this, "pref_cpdetector_list", wxListBox);
    cpdetector_config_edit.Read(cfg);

    // Load configuration values from wxConfig
    UpdateDisplayData(0);

#ifdef __WXMSW__
    // wxFrame does have a strange background color on Windows, copy color from a child widget
    this->SetBackgroundColour(XRCCTRL(*this, "prefs_tab", wxNotebook)->GetBackgroundColour());
#endif

#if wxCHECK_VERSION(2,9,1)
    wxCheckBox* show_hints=XRCCTRL(*this,"pref_show_projection_hints",wxCheckBox);
    show_hints->Enable(true);
    show_hints->Show(true);
    show_hints->Update();
    Update();
#endif

    GetSizer()->SetSizeHints(this);
    //    GetSizer()->Layout();


    // only enable bundled if the build is actually bundled.
#if defined __WXMSW__ || defined MAC_SELF_CONTAINED_BUNDLE

#else
    MY_BOOL_VAL("prefs_enblend_Custom", HUGIN_ENBLEND_EXE_CUSTOM);
    XRCCTRL(*this, "prefs_enblend_Custom", wxCheckBox)->Hide();
    cfg->Write(wxT("/Enblend/Custom"), HUGIN_ENBLEND_EXE_CUSTOM);

    MY_BOOL_VAL("prefs_enblend_enfuseCustom", HUGIN_ENFUSE_EXE_CUSTOM);
    XRCCTRL(*this, "prefs_enblend_enfuseCustom", wxCheckBox)->Hide();
    cfg->Write(wxT("/Enfuse/Custom"), HUGIN_ENFUSE_EXE_CUSTOM);
#endif

    RestoreFramePosition(this, wxT("PreferencesDialog"));
}


PreferencesDialog::~PreferencesDialog()
{
    DEBUG_TRACE("begin dtor");

    StoreFramePosition(this, wxT("PreferencesDialog"));

    // delete custom list data
    wxChoice* lang_choice = XRCCTRL(*this, "prefs_gui_language", wxChoice);
    for (int i = 0; i < (int) lang_choice->GetCount(); i++)
    {
        delete static_cast<long*>(lang_choice->GetClientData(i));
    }

    DEBUG_TRACE("end dtor");
}

void PreferencesDialog::OnOk(wxCommandEvent& e)
{
    UpdateConfigData();
    this->EndModal(wxOK);
}

void PreferencesDialog::OnCancel(wxCommandEvent& e)
{
    this->EndModal(wxCANCEL);
}

void PreferencesDialog::OnHelp(wxCommandEvent& e)
{
    MainFrame::Get()->DisplayHelp(wxT("/Hugin_Preferences.html"));
};

void PreferencesDialog::OnRotationCheckBox(wxCommandEvent& e)
{
    EnableRotationCtrls(e.IsChecked());
}

void PreferencesDialog::OnEnblendExe(wxCommandEvent& e)
{
    wxFileDialog dlg(this,_("Select Enblend"),
                     wxT(""), wxT(HUGIN_ENBLEND_EXE),
#ifdef __WXMSW__
                     _("Executables (*.exe)|*.exe"),
#else
                     wxT("*"),
#endif
                     wxFD_OPEN, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK)
    {
        XRCCTRL(*this, "prefs_enblend_EnblendExe", wxTextCtrl)->SetValue(
            dlg.GetPath());
    }
}

void PreferencesDialog::OnEnfuseExe(wxCommandEvent& e)
{
    wxFileDialog dlg(this,_("Select Enfuse"),
                     wxT(""), wxT(HUGIN_ENFUSE_EXE),
#ifdef __WXMSW__
                     _("Executables (*.exe)|*.exe"),
#else
                     wxT("*"),
#endif
                     wxFD_OPEN, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK)
    {
        XRCCTRL(*this, "prefs_enblend_EnfuseExe", wxTextCtrl)->SetValue(
            dlg.GetPath());
    }
}

void PreferencesDialog::OnCustomEnblend(wxCommandEvent& e)
{
    XRCCTRL(*this, "prefs_enblend_EnblendExe", wxTextCtrl)->Enable(e.IsChecked());
    XRCCTRL(*this, "prefs_enblend_select", wxButton)->Enable(e.IsChecked());
}

void PreferencesDialog::OnCustomEnfuse(wxCommandEvent& e)
{
    XRCCTRL(*this, "prefs_enblend_EnfuseExe", wxTextCtrl)->Enable(e.IsChecked());
    XRCCTRL(*this, "prefs_enblend_enfuse_select", wxButton)->Enable(e.IsChecked());
}

void PreferencesDialog::EnableRotationCtrls(bool enable)
{
    XRCCTRL(*this, "prefs_ft_rot_panel", wxPanel)->Enable(enable);
}

void PreferencesDialog::UpdateDisplayData(int panel)
{
    DEBUG_DEBUG("Updating display data");

    double d;
    bool t;
    wxString tstr;
    wxConfigBase* cfg = wxConfigBase::Get();

    if (panel==0 || panel == 1)
    {
        // memory setting
        unsigned long long mem = cfg->Read(wxT("/ImageCache/UpperBound"), HUGIN_IMGCACHE_UPPERBOUND);
#ifdef __WXMSW__
        unsigned long mem_low = cfg->Read(wxT("/ImageCache/UpperBound"), HUGIN_IMGCACHE_UPPERBOUND);
        unsigned long mem_high = cfg->Read(wxT("/ImageCache/UpperBoundHigh"), (long) 0);
        if (mem_high > 0)
        {
            mem = ((unsigned long long) mem_high << 32) + mem_low;
        }
        else
        {
            mem = mem_low;
        }
#endif
        MY_SPIN_VAL("prefs_cache_UpperBound", mem >> 20);

        // number of threads
        int nThreads = wxThread::GetCPUCount();
        if (nThreads < 1)
        {
            nThreads = 1;
        }
        nThreads = cfg->Read(wxT("/Nona/NumberOfThreads"), nThreads);
        MY_SPIN_VAL("prefs_nona_NumberOfThreads", nThreads);

        // language
        // check if current language is in list and activate it then.
        wxChoice* lang_choice = XRCCTRL(*this, "prefs_gui_language", wxChoice);
        int curlang = cfg->Read(wxT("language"), HUGIN_LANGUAGE);
        bool found = false;
        int idx = 0;
        for (int i = 0; i < (int)lang_choice->GetCount(); i++)
        {
            long lang = * static_cast<long*>(lang_choice->GetClientData(i));
            if (curlang == lang)
            {
                found = true;
                idx = i;
            }
        }
        if (found)
        {
            DEBUG_DEBUG("wxChoice language updated:" << curlang);
            // update language
            lang_choice->SetSelection(idx);
        }
        else
        {
            // unknown language selected..
            DEBUG_WARN("Unknown language configured");
        }

        // smart undo
        t = cfg->Read(wxT("smartUndo"), HUGIN_SMART_UNDO) == 1;
        MY_BOOL_VAL("prefs_smart_undo", t);

        // copy log to clipboard
        t = cfg->Read(wxT("CopyLogToClipboard"), 0l) == 1;
        MY_BOOL_VAL("prefs_copy_log", t);

        t = cfg->Read(wxT("/GLPreviewFrame/ShowProjectionHints"), HUGIN_SHOW_PROJECTION_HINTS) == 1;
        MY_BOOL_VAL("pref_show_projection_hints", t)
    };

    // filename panel
    if(panel==0 || panel==2)
    {
        // tempdir
        MY_STR_VAL("prefs_misc_tempdir", cfg->Read(wxT("tempDir"),wxT("")));
        // default filenames
        wxString filename=cfg->Read(wxT("ProjectFilename"), wxT(HUGIN_DEFAULT_PROJECT_NAME));
#ifdef __WXMSW__
        filename.Replace(wxT("/"),wxT("\\"),true);
#endif
        MY_STR_VAL("prefs_project_filename", filename);
        filename=cfg->Read(wxT("OutputFilename"), wxT(HUGIN_DEFAULT_OUTPUT_NAME));
#ifdef __WXMSW__
        filename.Replace(wxT("/"),wxT("\\"),true);
#endif
        MY_STR_VAL("prefs_output_filename", filename);
    }

    if (panel==0 || panel == 3)
    {
        // Assistant settings
        t = cfg->Read(wxT("/Assistant/autoAlign"), HUGIN_ASS_AUTO_ALIGN) == 1;
        MY_BOOL_VAL("prefs_ass_autoAlign", t);
        MY_SPIN_VAL("prefs_ass_nControlPoints",
                    cfg->Read(wxT("/Assistant/nControlPoints"), HUGIN_ASS_NCONTROLPOINTS));
        double factor = HUGIN_ASS_PANO_DOWNSIZE_FACTOR;
        cfg->Read(wxT("/Assistant/panoDownsizeFactor"), &factor);
        MY_SPIN_VAL("prefs_ass_panoDownsizeFactor",(int)(factor*100.0));
        t = cfg->Read(wxT("/Assistant/Linefind"), HUGIN_ASS_LINEFIND) == 1;
        MY_BOOL_VAL("prefs_ass_linefind", t);
        t = cfg->Read(wxT("/Celeste/Auto"), HUGIN_CELESTE_AUTO) == 1;
        MY_BOOL_VAL("prefs_celeste_auto", t);
        t = cfg->Read(wxT("/Assistant/AutoCPClean"), HUGIN_ASS_AUTO_CPCLEAN) == 1;
        MY_BOOL_VAL("prefs_auto_cpclean", t);
    }
    // Fine tune settings

    if (panel==0 || panel == 4)
    {
        // hdr display settings
        MY_CHOICE_VAL("prefs_misc_hdr_mapping", cfg->Read(wxT("/ImageCache/Mapping"), HUGIN_IMGCACHE_MAPPING_FLOAT));
        //MY_CHOICE_VAL("prefs_misc_hdr_range", cfg->Read(wxT("/ImageCache/Range"), HUGIN_IMGCACHE_RANGE));

        MY_SPIN_VAL("prefs_ft_TemplateSize",
                    cfg->Read(wxT("/Finetune/TemplateSize"),HUGIN_FT_TEMPLATE_SIZE));
        MY_SPIN_VAL("prefs_ft_SearchAreaPercent",cfg->Read(wxT("/Finetune/SearchAreaPercent"),
                    HUGIN_FT_SEARCH_AREA_PERCENT));
        MY_SPIN_VAL("prefs_ft_LocalSearchWidth", cfg->Read(wxT("/Finetune/LocalSearchWidth"),
                    HUGIN_FT_LOCAL_SEARCH_WIDTH));

        d=HUGIN_FT_CORR_THRESHOLD;
        cfg->Read(wxT("/Finetune/CorrThreshold"), &d, HUGIN_FT_CORR_THRESHOLD);
        tstr = hugin_utils::doubleTowxString(d);
        MY_STR_VAL("prefs_ft_CorrThreshold", tstr);

        cfg->Read(wxT("/Finetune/CurvThreshold"), &d, HUGIN_FT_CURV_THRESHOLD);
        tstr = hugin_utils::doubleTowxString(d);
        MY_STR_VAL("prefs_ft_CurvThreshold", tstr);

        t = cfg->Read(wxT("/Finetune/RotationSearch"), HUGIN_FT_ROTATION_SEARCH) == 1;
        MY_BOOL_VAL("prefs_ft_RotationSearch", t);
        EnableRotationCtrls(t);

        d = HUGIN_FT_ROTATION_START_ANGLE;
        cfg->Read(wxT("/Finetune/RotationStartAngle"),&d,HUGIN_FT_ROTATION_START_ANGLE);
        MY_SPIN_VAL("prefs_ft_RotationStartAngle", hugin_utils::roundi(d))

        d = HUGIN_FT_ROTATION_STOP_ANGLE;
        cfg->Read(wxT("/Finetune/RotationStopAngle"), &d, HUGIN_FT_ROTATION_STOP_ANGLE);
        MY_SPIN_VAL("prefs_ft_RotationStopAngle", hugin_utils::roundi(d));

        MY_SPIN_VAL("prefs_ft_RotationSteps", cfg->Read(wxT("/Finetune/RotationSteps"),
                    HUGIN_FT_ROTATION_STEPS));
    }

    /////
    /// MISC

    /////
    /// CP Detector programs

    if (panel==0 || panel == 5)
    {
        cpdetector_config_edit.FillControl(m_CPDetectorList,true,true);
    }

    if (panel==0 || panel == 6)
    {
        /////
        /// DEFAULT OUTPUT FORMAT
        MY_CHOICE_VAL("pref_ldr_output_file_format", cfg->Read(wxT("/output/ldr_format"), HUGIN_LDR_OUTPUT_FORMAT));
        /** HDR currently deactivated since HDR TIFF broken and only choice is EXR */
        // MY_CHOICE_VAL("pref_hdr_output_file_format", cfg->Read(wxT("/output/hdr_format"), HUGIN_HDR_OUTPUT_FORMAT));
        MY_CHOICE_VAL("pref_tiff_compression", cfg->Read(wxT("/output/tiff_compression"), HUGIN_TIFF_COMPRESSION));
        MY_SPIN_VAL("pref_jpeg_quality", cfg->Read(wxT("/output/jpeg_quality"), HUGIN_JPEG_QUALITY));
        UpdateFileFormatControls();

        /////
        /// PROCESSOR
        MY_CHOICE_VAL("pref_processor_gui", cfg->Read(wxT("/Processor/gui"), HUGIN_PROCESSOR_GUI));
        t = cfg->Read(wxT("/Processor/start"), HUGIN_PROCESSOR_START) == 1;
        MY_BOOL_VAL("pref_processor_start", t);
        t = cfg->Read(wxT("/Processor/parallel"), HUGIN_PROCESSOR_PARALLEL) == 1;
        MY_BOOL_VAL("pref_processor_parallel", t);
        t = cfg->Read(wxT("/Processor/overwrite"), HUGIN_PROCESSOR_OVERWRITE) == 1;
        MY_BOOL_VAL("pref_processor_overwrite", t);
        t = cfg->Read(wxT("/Processor/verbose"), HUGIN_PROCESSOR_VERBOSE) == 1;
        MY_BOOL_VAL("pref_processor_verbose", t);
        UpdateProcessorControls();

        t = cfg->Read(wxT("/output/useExiftool"), 1l) == 1;
        MY_BOOL_VAL("pref_exiftool_metadata", t);

        int nThreads = cfg->Read(wxT("/output/NumberOfThreads"), 0l);
        MY_SPIN_VAL("prefs_output_NumberOfThreads", nThreads);

    }

    if (panel==0 || panel == 7)
    {

        /////
        /// NONA
        MY_CHOICE_VAL("prefs_nona_interpolator", cfg->Read(wxT("/Nona/Interpolator"), HUGIN_NONA_INTERPOLATOR));
        t = cfg->Read(wxT("/Nona/CroppedImages"), HUGIN_NONA_CROPPEDIMAGES) == 1;
        MY_BOOL_VAL("prefs_nona_createCroppedImages", t);
        t = cfg->Read(wxT("/Nona/UseGPU"), HUGIN_NONA_USEGPU) == 1;
        MY_BOOL_VAL("prefs_nona_useGpu", t);

        /////
        /// ENBLEND
        MY_STR_VAL("prefs_enblend_EnblendExe", cfg->Read(wxT("/Enblend/Exe"),
                   wxT(HUGIN_ENBLEND_EXE)));
        bool customEnblendExe = HUGIN_ENBLEND_EXE_CUSTOM;
        cfg->Read(wxT("/Enblend/Custom"), &customEnblendExe);
        MY_BOOL_VAL("prefs_enblend_Custom", customEnblendExe);
        XRCCTRL(*this, "prefs_enblend_EnblendExe", wxTextCtrl)->Enable(customEnblendExe);
        XRCCTRL(*this, "prefs_enblend_select", wxButton)->Enable(customEnblendExe);
        MY_STR_VAL("prefs_enblend_EnblendArgs", cfg->Read(wxT("/Enblend/Args"),
                   wxT(HUGIN_ENBLEND_ARGS)));
        /////
        /// ENFUSE
        MY_STR_VAL("prefs_enblend_EnfuseExe", cfg->Read(wxT("/Enfuse/Exe"),
                   wxT(HUGIN_ENFUSE_EXE)));
        bool customEnfuseExe = HUGIN_ENFUSE_EXE_CUSTOM;
        cfg->Read(wxT("/Enfuse/Custom"), &customEnfuseExe);
        MY_BOOL_VAL("prefs_enblend_enfuseCustom", customEnfuseExe);
        XRCCTRL(*this, "prefs_enblend_EnfuseExe", wxTextCtrl)->Enable(customEnfuseExe);
        XRCCTRL(*this, "prefs_enblend_enfuse_select", wxButton)->Enable(customEnfuseExe);
        MY_STR_VAL("prefs_enblend_EnfuseArgs", cfg->Read(wxT("/Enfuse/Args"),
                   wxT(HUGIN_ENFUSE_ARGS)));
    }

    if (panel==0 || panel == 8)
    {
        // Celeste settings
        d=HUGIN_CELESTE_THRESHOLD;
        cfg->Read(wxT("/Celeste/Threshold"), &d, HUGIN_CELESTE_THRESHOLD);
        tstr = hugin_utils::doubleTowxString(d);
        MY_STR_VAL("prefs_celeste_threshold", tstr);
        MY_CHOICE_VAL("prefs_celeste_filter", cfg->Read(wxT("/Celeste/Filter"), HUGIN_CELESTE_FILTER));
        // photometric optimizer settings
        MY_SPIN_VAL("prefs_photo_optimizer_nr_points", cfg->Read(wxT("/OptimizePhotometric/nRandomPointsPerImage"), HUGIN_PHOTOMETRIC_OPTIMIZER_NRPOINTS));

    }
}

void PreferencesDialog::OnRestoreDefaults(wxCommandEvent& e)
{
    DEBUG_TRACE("");
    wxConfigBase* cfg = wxConfigBase::Get();
    // check which tab is enabled
    wxNotebook* noteb = XRCCTRL(*this, "prefs_tab", wxNotebook);
    int really = wxMessageBox(_("Really reset displayed preferences to default values?"), _("Load Defaults"),
                              wxYES_NO, this);
    if ( really == wxYES)
    {
        if (noteb->GetSelection() == 0)
        {
            // MISC
            // cache
            /*
             * special treatment for windows not necessary here since we know the value of
             * HUGIN_IMGCACHE_UPPERBOUND must fit into 32bit to be compatible with 32bit systems.
             * However, just as a reminder:
            #ifdef __WXMSW__
                cfg->Write(wxT("/ImageCache/UpperBoundHigh"), HUGIN_IMGCACHE_UPPERBOUND >> 32);
            #endif
            */
            cfg->Write(wxT("/ImageCache/UpperBound"), HUGIN_IMGCACHE_UPPERBOUND);
            // number of threads
            int cpucount = wxThread::GetCPUCount();
            if (cpucount < 1)
            {
                cpucount = 1;
            }
            cfg->Write(wxT("/Nona/NumberOfThreads"), cpucount);
            // locale
            cfg->Write(wxT("language"), int(HUGIN_LANGUAGE));
            // smart undo
            cfg->Write(wxT("smartUndo"), HUGIN_SMART_UNDO);
            cfg->Write(wxT("CopyLogToClipboard"), 0l);
            // projection hints
            cfg->Write(wxT("/GLPreviewFrame/ShowProjectionHints"), HUGIN_SHOW_PROJECTION_HINTS);
        }
        if(noteb->GetSelection() == 1)
        {
            cfg->Write(wxT("tempDir"), wxT(""));
            cfg->Write(wxT("ProjectFilename"), wxT(HUGIN_DEFAULT_PROJECT_NAME));
            cfg->Write(wxT("OutputFilename"), wxT(HUGIN_DEFAULT_OUTPUT_NAME));
        };
        if (noteb->GetSelection() == 2)
        {
            cfg->Write(wxT("/Assistant/autoAlign"), HUGIN_ASS_AUTO_ALIGN);
            cfg->Write(wxT("/Assistant/nControlPoints"), HUGIN_ASS_NCONTROLPOINTS);
            cfg->Write(wxT("/Assistant/panoDownsizeFactor"),HUGIN_ASS_PANO_DOWNSIZE_FACTOR);
            cfg->Write(wxT("/Assistant/Linefind"), HUGIN_ASS_LINEFIND);
            cfg->Write(wxT("/Celeste/Auto"), HUGIN_CELESTE_AUTO);
            cfg->Write(wxT("/Assistant/AutoCPClean"), HUGIN_ASS_AUTO_CPCLEAN);
        }
        if (noteb->GetSelection() == 3)
        {
            // hdr
            cfg->Write(wxT("/ImageCache/Mapping"), HUGIN_IMGCACHE_MAPPING_FLOAT);
            //cfg->Write(wxT("/ImageCache/Range"), HUGIN_IMGCACHE_RANGE);
            // Fine tune settings
            cfg->Write(wxT("/Finetune/SearchAreaPercent"), HUGIN_FT_SEARCH_AREA_PERCENT);
            cfg->Write(wxT("/Finetune/TemplateSize"), HUGIN_FT_TEMPLATE_SIZE);
            cfg->Write(wxT("/Finetune/LocalSearchWidth"), HUGIN_FT_LOCAL_SEARCH_WIDTH);

            cfg->Write(wxT("/Finetune/CorrThreshold"), HUGIN_FT_CORR_THRESHOLD);
            cfg->Write(wxT("/Finetune/CurvThreshold"), HUGIN_FT_CURV_THRESHOLD);

            cfg->Write(wxT("/Finetune/RotationSearch"), HUGIN_FT_ROTATION_SEARCH);
            cfg->Write(wxT("/Finetune/RotationStartAngle"), HUGIN_FT_ROTATION_START_ANGLE);
            cfg->Write(wxT("/Finetune/RotationStopAngle"), HUGIN_FT_ROTATION_STOP_ANGLE);
            cfg->Write(wxT("/Finetune/RotationSteps"), HUGIN_FT_ROTATION_STEPS);
        }
        if (noteb->GetSelection() == 4)
        {
            /////
            /// AUTOPANO
            cpdetector_config_edit.ReadFromFile(huginApp::Get()->GetDataPath()+wxT("default.setting"));
            cpdetector_config_edit.Write(cfg);
        }
        if (noteb->GetSelection() == 5)
        {
            /// OUTPUT
            cfg->Write(wxT("/output/ldr_format"), HUGIN_LDR_OUTPUT_FORMAT);
            /** HDR currently deactivated since HDR TIFF broken and only choice is EXR */
            // cfg->Write(wxT("/output/hdr_format"), HUGIN_HDR_OUTPUT_FORMAT);
            cfg->Write(wxT("/output/tiff_compression"), HUGIN_TIFF_COMPRESSION);
            cfg->Write(wxT("/output/jpeg_quality"), HUGIN_JPEG_QUALITY);
            // stitching engine
            cfg->Write(wxT("/Processor/gui"), HUGIN_PROCESSOR_GUI);
            cfg->Write(wxT("/Processor/start"), HUGIN_PROCESSOR_START);
            cfg->Write(wxT("/Processor/parallel"), HUGIN_PROCESSOR_PARALLEL);
            cfg->Write(wxT("/Processor/overwrite"), HUGIN_PROCESSOR_OVERWRITE);
            cfg->Write(wxT("/Processor/verbose"), HUGIN_PROCESSOR_VERBOSE);
            cfg->Write(wxT("/output/useExiftool"), 1l);
            cfg->Write(wxT("/output/NumberOfThreads"), 0l);

        }
        if (noteb->GetSelection() == 6)
        {
            /// ENBLEND
            cfg->Write(wxT("/Enblend/Exe"), wxT(HUGIN_ENBLEND_EXE));
            cfg->Write(wxT("/Enblend/Custom"), HUGIN_ENBLEND_EXE_CUSTOM);
            cfg->Write(wxT("/Enblend/Args"), wxT(HUGIN_ENBLEND_ARGS));

            cfg->Write(wxT("/Enfuse/Exe"), wxT(HUGIN_ENFUSE_EXE));
            cfg->Write(wxT("/Enfuse/Custom"), HUGIN_ENFUSE_EXE_CUSTOM);
            cfg->Write(wxT("/Enfuse/Args"), wxT(HUGIN_ENFUSE_ARGS));
        }

        if (noteb->GetSelection() == 7)
        {
            /// Celeste
            cfg->Write(wxT("/Celeste/Threshold"), HUGIN_CELESTE_THRESHOLD);
            cfg->Write(wxT("/Celeste/Filter"), HUGIN_CELESTE_FILTER);
            cfg->Write(wxT("/OptimizePhotometric/nRandomPointsPerImage"), HUGIN_PHOTOMETRIC_OPTIMIZER_NRPOINTS);
        }

        /*
                if (noteb->GetSelection() == 5) {
                    cfg->Write(wxT("/PTmender/Exe"), wxT(HUGIN_PT_MENDER_EXE) );
                    cfg->Write(wxT("/PTmender/Custom"),HUGIN_PT_MENDER_EXE_CUSTOM);
                    cfg->Write(wxT("/PanoTools/ScriptFile"), wxT("PT_script.txt"));
                }
        */
        UpdateDisplayData(noteb->GetSelection() + 1);
    }
}

void PreferencesDialog::UpdateConfigData()
{
    DEBUG_TRACE("");
    wxConfigBase* cfg = wxConfigBase::Get();
    // Assistant
    cfg->Write(wxT("/Assistant/autoAlign"),MY_G_BOOL_VAL("prefs_ass_autoAlign"));
    cfg->Write(wxT("/Assistant/nControlPoints"), MY_G_SPIN_VAL("prefs_ass_nControlPoints"));
    cfg->Write(wxT("/Assistant/panoDownsizeFactor"), MY_G_SPIN_VAL("prefs_ass_panoDownsizeFactor") / 100.0);
    cfg->Write(wxT("/Assistant/Linefind"), MY_G_BOOL_VAL("prefs_ass_linefind"));
    cfg->Write(wxT("/Celeste/Auto"), MY_G_BOOL_VAL("prefs_celeste_auto"));
    cfg->Write(wxT("/Assistant/AutoCPClean"), MY_G_BOOL_VAL("prefs_auto_cpclean"));

    // hdr display
    cfg->Write(wxT("/ImageCache/Mapping"),MY_G_CHOICE_VAL("prefs_misc_hdr_mapping"));
    //cfg->Write(wxT("/ImageCache/Range"),MY_G_CHOICE_VAL("prefs_misc_hdr_range"));

    // Fine tune settings
    cfg->Write(wxT("/Finetune/SearchAreaPercent"), MY_G_SPIN_VAL("prefs_ft_SearchAreaPercent"));
    cfg->Write(wxT("/Finetune/TemplateSize"), MY_G_SPIN_VAL("prefs_ft_TemplateSize"));
    cfg->Write(wxT("/Finetune/LocalSearchWidth"), MY_G_SPIN_VAL("prefs_ft_LocalSearchWidth"));
    wxString t = MY_G_STR_VAL("prefs_ft_CorrThreshold");
    double td= HUGIN_FT_CORR_THRESHOLD;
    hugin_utils::stringToDouble(std::string(t.mb_str(wxConvLocal)), td);
    cfg->Write(wxT("/Finetune/CorrThreshold"), td);

    t = MY_G_STR_VAL("prefs_ft_CurvThreshold");
    td = HUGIN_FT_CURV_THRESHOLD;
    hugin_utils::stringToDouble(std::string(t.mb_str(wxConvLocal)), td);
    cfg->Write(wxT("/Finetune/CurvThreshold"), td);

    cfg->Write(wxT("/Finetune/RotationSearch"), MY_G_BOOL_VAL("prefs_ft_RotationSearch"));
    cfg->Write(wxT("/Finetune/RotationStartAngle"), (double) MY_G_SPIN_VAL("prefs_ft_RotationStartAngle"));
    cfg->Write(wxT("/Finetune/RotationStopAngle"), (double) MY_G_SPIN_VAL("prefs_ft_RotationStopAngle"));
    cfg->Write(wxT("/Finetune/RotationSteps"), MY_G_SPIN_VAL("prefs_ft_RotationSteps"));

    /////
    /// MISC
    // cache
#ifdef __WXMSW__
    // shifting only 12 bits rights: 32-20=12 and the prefs_cache_UpperBound is in GB
    cfg->Write(wxT("/ImageCache/UpperBoundHigh"), (long) MY_G_SPIN_VAL("prefs_cache_UpperBound") >> 12);
#endif
    cfg->Write(wxT("/ImageCache/UpperBound"), (long) MY_G_SPIN_VAL("prefs_cache_UpperBound") << 20);
    // number of threads
    cfg->Write(wxT("/Nona/NumberOfThreads"), MY_G_SPIN_VAL("prefs_nona_NumberOfThreads"));

    // locale
    // language
    wxChoice* lang = XRCCTRL(*this, "prefs_gui_language", wxChoice);
    // DEBUG_TRACE("Language Selection Name: " << huginApp::Get()->GetLocale().GetLanguageName((int) lang->GetClientData(lang->GetSelection())).mb_str(wxConvLocal));
    //DEBUG_INFO("Language Selection locale: " << ((huginApp::Get()->GetLocale().GetLanguageInfo((int) lang->GetClientData(lang->GetSelection())))->CanonicalName).mb_str(wxConvLocal));
    //DEBUG_INFO("Current System Language ID: " << huginApp::Get()->GetLocale().GetSystemLanguage());

    void* tmplp = lang->GetClientData(lang->GetSelection());
    long templ =  * static_cast<long*>(tmplp);
    cfg->Write(wxT("language"), templ);
    DEBUG_INFO("Language Selection ID: " << templ);
    // smart undo
    cfg->Write(wxT("smartUndo"), MY_G_BOOL_VAL("prefs_smart_undo"));
    cfg->Write(wxT("CopyLogToClipboard"), MY_G_BOOL_VAL("prefs_copy_log"));
    // show projections hints
    cfg->Write(wxT("/GLPreviewFrame/ShowProjectionHints"), MY_G_BOOL_VAL("pref_show_projection_hints"));
    // tempdir
    cfg->Write(wxT("tempDir"),MY_G_STR_VAL("prefs_misc_tempdir"));
    // filename templates
    wxString filename=XRCCTRL(*this, "prefs_project_filename", wxTextCtrl)->GetValue();
#ifdef __WXMSW__
    filename.Replace(wxT("\\"), wxT("/"), true);
#endif
    cfg->Write(wxT("ProjectFilename"), filename);
    filename=XRCCTRL(*this, "prefs_output_filename", wxTextCtrl)->GetValue();
#ifdef __WXMSW__
    filename.Replace(wxT("\\"), wxT("/"), true);
#endif
    cfg->Write(wxT("OutputFilename"), filename);
    /////
    /// AUTOPANO
    cpdetector_config_edit.Write(cfg);

    /////
    /// OUTPUT
    cfg->Write(wxT("/output/ldr_format"), MY_G_CHOICE_VAL("pref_ldr_output_file_format"));
    /** HDR currently deactivated since HDR TIFF broken and only choice is EXR */
    // cfg->Write(wxT("/output/hdr_format"), MY_G_CHOICE_VAL("pref_hdr_output_file_format"));
    cfg->Write(wxT("/output/tiff_compression"), MY_G_CHOICE_VAL("pref_tiff_compression"));
    cfg->Write(wxT("/output/jpeg_quality"), MY_G_SPIN_VAL("pref_jpeg_quality"));

    /////
    /// PROCESSOR
    cfg->Write(wxT("/Processor/gui"), MY_G_CHOICE_VAL("pref_processor_gui"));
    cfg->Write(wxT("/Processor/start"), MY_G_BOOL_VAL("pref_processor_start"));
    cfg->Write(wxT("/Processor/parallel"), MY_G_BOOL_VAL("pref_processor_parallel"));
    cfg->Write(wxT("/Processor/overwrite"), MY_G_BOOL_VAL("pref_processor_overwrite"));
    cfg->Write(wxT("/Processor/verbose"), MY_G_BOOL_VAL("pref_processor_verbose"));

    cfg->Write(wxT("/output/useExiftool"), MY_G_BOOL_VAL("pref_exiftool_metadata"));
    cfg->Write(wxT("/output/NumberOfThreads"), MY_G_SPIN_VAL("prefs_output_NumberOfThreads"));
    /////
    /// STITCHING
    cfg->Write(wxT("/Nona/Interpolator"), MY_G_CHOICE_VAL("prefs_nona_interpolator"));
    cfg->Write(wxT("/Nona/CroppedImages"), MY_G_BOOL_VAL("prefs_nona_createCroppedImages"));
    cfg->Write(wxT("/Nona/UseGPU"), MY_G_BOOL_VAL("prefs_nona_useGpu"));

    /////
    /// ENBLEND
    cfg->Write(wxT("/Enblend/Custom"), MY_G_BOOL_VAL("prefs_enblend_Custom"));
    cfg->Write(wxT("/Enblend/Exe"), MY_G_STR_VAL("prefs_enblend_EnblendExe"));
    cfg->Write(wxT("/Enblend/Args"), MY_G_STR_VAL("prefs_enblend_EnblendArgs"));

    cfg->Write(wxT("/Enfuse/Custom"), MY_G_BOOL_VAL("prefs_enblend_enfuseCustom"));
    cfg->Write(wxT("/Enfuse/Exe"), MY_G_STR_VAL("prefs_enblend_EnfuseExe"));
    cfg->Write(wxT("/Enfuse/Args"), MY_G_STR_VAL("prefs_enblend_EnfuseArgs"));

    // Celeste
    t = MY_G_STR_VAL("prefs_celeste_threshold");
    td = HUGIN_CELESTE_THRESHOLD;
    hugin_utils::stringToDouble(std::string(t.mb_str(wxConvLocal)), td);
    cfg->Write(wxT("/Celeste/Threshold"), td);
    cfg->Write(wxT("/Celeste/Filter"), MY_G_CHOICE_VAL("prefs_celeste_filter"));
    //photometric optimizer
    cfg->Write(wxT("/OptimizePhotometric/nRandomPointsPerImage"), MY_G_SPIN_VAL("prefs_photo_optimizer_nr_points"));

    cfg->Flush();
    UpdateDisplayData(0);
}

void PreferencesDialog::OnCPDetectorAdd(wxCommandEvent& e)
{
    CPDetectorDialog cpdetector_dlg(this);
    if(cpdetector_dlg.ShowModal()==wxOK)
    {
        cpdetector_config_edit.settings.Add(new CPDetectorSetting);
        cpdetector_dlg.UpdateSettings(&cpdetector_config_edit,cpdetector_config_edit.GetCount()-1);
        cpdetector_config_edit.FillControl(m_CPDetectorList,false,true);
        m_CPDetectorList->SetSelection(cpdetector_config_edit.GetCount()-1);
    };
};

void PreferencesDialog::OnCPDetectorEdit(wxCommandEvent& e)
{
    CPDetectorDialog autopano_dlg(this);
    int selection=m_CPDetectorList->GetSelection();
    if (selection == wxNOT_FOUND)
    {
        wxMessageBox(_("Please select an entry first"),_("Select Entry"),wxOK |
                     wxICON_EXCLAMATION,this);
    }
    else
    {
        autopano_dlg.UpdateFields(&cpdetector_config_edit, selection);
        if(autopano_dlg.ShowModal()==wxOK)
        {
            autopano_dlg.UpdateSettings(&cpdetector_config_edit, selection);
            cpdetector_config_edit.FillControl(m_CPDetectorList,false,true);
            m_CPDetectorList->SetSelection(selection);
        };
    }
};

void PreferencesDialog::OnCPDetectorDelete(wxCommandEvent& e)
{
    unsigned int selection=m_CPDetectorList->GetSelection();
    if(m_CPDetectorList->GetCount()==1)
    {
        wxMessageBox(_("You can't delete the last setting.\nAt least one setting is required."),_("Warning"),wxOK | wxICON_EXCLAMATION,this);
    }
    else
    {
        if(wxMessageBox(wxString::Format(_("Do you really want to remove control point detector setting \"%s\"?"),cpdetector_config_edit.settings[selection].GetCPDetectorDesc().c_str())
                        ,_("Delete control point detector setting"),wxYES_NO | wxICON_QUESTION,this)==wxYES)
        {
            if(cpdetector_config_edit.GetDefaultGenerator()==selection)
            {
                cpdetector_config_edit.SetDefaultGenerator(0);
            }
            cpdetector_config_edit.settings.RemoveAt(selection);
            cpdetector_config_edit.FillControl(m_CPDetectorList,false,true);
            if(selection>=m_CPDetectorList->GetCount())
            {
                selection=m_CPDetectorList->GetCount()-1;
            }
            m_CPDetectorList->SetSelection(selection);
        };
    };
};

void PreferencesDialog::OnCPDetectorMoveUp(wxCommandEvent& e)
{
    unsigned int selection=m_CPDetectorList->GetSelection();
    if(selection>0)
    {
        cpdetector_config_edit.Swap(selection-1);
        cpdetector_config_edit.FillControl(m_CPDetectorList,false,true);
        m_CPDetectorList->SetSelection(selection-1);
    };
};

void PreferencesDialog::OnCPDetectorMoveDown(wxCommandEvent& e)
{
    unsigned int selection=m_CPDetectorList->GetSelection();
    if(selection<m_CPDetectorList->GetCount()-1)
    {
        cpdetector_config_edit.Swap(selection);
        cpdetector_config_edit.FillControl(m_CPDetectorList,false,true);
        m_CPDetectorList->SetSelection(selection+1);
    };
};

void PreferencesDialog::OnCPDetectorDefault(wxCommandEvent& e)
{
    unsigned int selection=m_CPDetectorList->GetSelection();
    if(selection!=cpdetector_config_edit.GetDefaultGenerator())
    {
        cpdetector_config_edit.SetDefaultGenerator(selection);
        cpdetector_config_edit.FillControl(m_CPDetectorList,false,true);
        m_CPDetectorList->SetSelection(selection);
    };
};

void PreferencesDialog::OnCPDetectorListDblClick(wxCommandEvent& e)
{
    OnCPDetectorEdit(e);
};

void PreferencesDialog::OnCPDetectorLoad(wxCommandEvent& e)
{
    wxFileDialog dlg(this,_("Load control point detector settings"),
                     wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")), wxEmptyString,
                     _("Control point detector settings (*.setting)|*.setting"),wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK)
    {
        wxConfig::Get()->Write(wxT("/actualPath"), dlg.GetDirectory());  // remember for later
        wxString fn = dlg.GetPath();
        cpdetector_config_edit.ReadFromFile(fn);
        cpdetector_config_edit.Write();
        UpdateDisplayData(4);
    };
};

void PreferencesDialog::OnCPDetectorSave(wxCommandEvent& e)
{
    wxFileDialog dlg(this,_("Save control point detector settings"),
                     wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")), wxEmptyString,
                     _("Control point detector settings (*.setting)|*.setting"),wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK)
    {
        wxConfig::Get()->Write(wxT("/actualPath"), dlg.GetDirectory());  // remember for later
        wxString fn = dlg.GetPath();
#ifndef __WXMSW__
        //append extension if not given
        //not necessary on Windows, the wxFileDialog appends it automatic
        if(fn.Right(8)!=wxT(".setting"))
        {
            fn.Append(wxT(".setting"));
        };
#endif
        cpdetector_config_edit.WriteToFile(fn);
    };
};

void PreferencesDialog::OnFileFormatChanged(wxCommandEvent& e)
{
    UpdateFileFormatControls();
};

void PreferencesDialog::UpdateFileFormatControls()
{
    int i=MY_G_CHOICE_VAL("pref_ldr_output_file_format");
    XRCCTRL(*this,"pref_tiff_compression_label",wxStaticText)->Show(i==0);
    XRCCTRL(*this,"pref_tiff_compression",wxChoice)->Show(i==0);
    XRCCTRL(*this,"pref_jpeg_quality_label",wxStaticText)->Show(i==1);
    XRCCTRL(*this,"pref_jpeg_quality",wxSpinCtrl)->Show(i==1);
    XRCCTRL(*this,"pref_tiff_compression",wxChoice)->GetParent()->Layout();
};

void PreferencesDialog::OnProcessorChanged(wxCommandEvent& e)
{
    UpdateProcessorControls();
};

void PreferencesDialog::UpdateProcessorControls()
{
    int i=MY_G_CHOICE_VAL("pref_processor_gui");
    XRCCTRL(*this,"pref_processor_start",wxCheckBox)->Enable(i==0);
    XRCCTRL(*this,"pref_processor_parallel",wxCheckBox)->Enable(i==0);
    XRCCTRL(*this,"pref_processor_verbose",wxCheckBox)->Enable(i==0);
    switch(i)
    {
        case 0:
            //PTBatcherGUI
            {
                wxConfigBase* config=wxConfigBase::Get();
                XRCCTRL(*this,"pref_processor_start",wxCheckBox)->SetValue(config->Read(wxT("/Processor/start"), HUGIN_PROCESSOR_START) == 1);
                XRCCTRL(*this,"pref_processor_parallel",wxCheckBox)->SetValue(config->Read(wxT("/Processor/parallel"), HUGIN_PROCESSOR_PARALLEL) == 1);
                XRCCTRL(*this,"pref_processor_verbose",wxCheckBox)->SetValue(config->Read(wxT("/Processor/verbose"), HUGIN_PROCESSOR_VERBOSE) == 1);
            }
            break;
        case 1:
            //Hugin_stitch_project
            XRCCTRL(*this,"pref_processor_start",wxCheckBox)->SetValue(true);
            XRCCTRL(*this,"pref_processor_parallel",wxCheckBox)->SetValue(false);
            XRCCTRL(*this,"pref_processor_verbose",wxCheckBox)->SetValue(true);
            break;
    };
};

void PreferencesDialog::OnUpdateProjectFilename(wxCommandEvent& e)
{
    XRCCTRL(*this, "prefs_project_filename_preview", wxStaticText)->SetLabel(
        getDefaultProjectName(MainFrame::Get()->getPanorama(), XRCCTRL(*this, "prefs_project_filename", wxTextCtrl)->GetValue())
    );
};

void PreferencesDialog::OnUpdateOutputFilename(wxCommandEvent& e)
{
    XRCCTRL(*this, "prefs_output_filename_preview", wxStaticText)->SetLabel(
        getDefaultOutputName(MainFrame::Get()->getProjectName(), MainFrame::Get()->getPanorama(), XRCCTRL(*this, "prefs_output_filename", wxTextCtrl)->GetValue())
    );
};

