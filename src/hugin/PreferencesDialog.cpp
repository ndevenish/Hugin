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
#include "panoinc.h"


#include "hugin/config_defaults.h"
#include "hugin/PreferencesDialog.h"

// validators are working different somehow...
//#define MY_STR_VAL(id, filter) { XRCCTRL(*this, "prefs_" #id, wxTextCtrl)->SetValidator(wxTextValidator(filter, &id)); }
//#define MY_SPIN_VAL(id) {     XRCCTRL(*this, "prefs_" #id, wxSpinCtrl)->SetValidator(wxGenericValidator(&id)); }

#define MY_STR_VAL(id, val) { XRCCTRL(*this, id, wxTextCtrl)->SetValue(val); };
#define MY_SPIN_VAL(id, val) { XRCCTRL(*this, id, wxSpinCtrl)->SetValue(val); };
#define MY_BOOL_VAL(id, val) { XRCCTRL(*this, id, wxCheckBox)->SetValue(val); };
#define MY_CHOICE_VAL(id, val) { XRCCTRL(*this, id, wxChoice)->SetSelection(val); };

#define MY_G_STR_VAL(id)  XRCCTRL(*this, id, wxTextCtrl)->GetValue()
#define MY_G_SPIN_VAL(id)  XRCCTRL(*this, id, wxSpinCtrl)->GetValue()
#define MY_G_BOOL_VAL(id)  XRCCTRL(*this, id, wxCheckBox)->GetValue()
#define MY_G_CHOICE_VAL(id)  XRCCTRL(*this, id, wxChoice)->GetSelection()



BEGIN_EVENT_TABLE(PreferencesDialog, wxFrame)
    EVT_CLOSE(PreferencesDialog::OnClose)
    EVT_BUTTON(wxID_OK, PreferencesDialog::OnOk)
    EVT_BUTTON(wxID_APPLY,PreferencesDialog::OnApply)
    EVT_BUTTON(wxID_CANCEL, PreferencesDialog::OnCancel)
    EVT_BUTTON(XRCID("prefs_load_defaults"), PreferencesDialog::OnDefaults)
    EVT_CHECKBOX(XRCID("prefs_ft_RotationSearch"), PreferencesDialog::OnRotationCheckBox)
//    EVT_CLOSE(RunOptimizerFrame::OnClose)
END_EVENT_TABLE()


PreferencesDialog::PreferencesDialog(wxWindow *parent)
//    : wxFrame(parent, -1, _("Preferences - hugin"))
{
    DEBUG_TRACE("");
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadFrame(this, parent, wxT("pref_dialog"));

    // Custom setup ( stuff that can not be done in XRC )
    XRCCTRL(*this, "prefs_ft_RotationStartAngle", wxSpinCtrl)->SetRange(-180,0);
    XRCCTRL(*this, "prefs_ft_RotationStopAngle", wxSpinCtrl)->SetRange(0,180);

    // Load configuration values from wxConfig
    UpdateDisplayData();
}


PreferencesDialog::~PreferencesDialog()
{
    DEBUG_TRACE("begin dtor");
    DEBUG_TRACE("end dtor");
}

void PreferencesDialog::OnApply(wxCommandEvent & e)
{
    UpdateConfigData();
}

void PreferencesDialog::OnOk(wxCommandEvent & e)
{
    UpdateConfigData();
    DEBUG_DEBUG("show false");
    this->Show(FALSE);
}

void PreferencesDialog::OnCancel(wxCommandEvent & e)
{
    UpdateDisplayData();
    DEBUG_DEBUG("show false");
    this->Show(FALSE);
}

void PreferencesDialog::OnClose(wxCloseEvent& event)
{
    DEBUG_DEBUG("OnClose");
    // do not close, just hide if we're not forced
    if (event.CanVeto()) {
        event.Veto();
        Hide();
        DEBUG_DEBUG("hiding");
    } else {
        DEBUG_DEBUG("about to destroy");
        Destroy();
        DEBUG_DEBUG("destroyed");
    }
}

void PreferencesDialog::OnDefaults(wxCommandEvent & e)
{
    DEBUG_WARN("Not implemented yet");
}

void PreferencesDialog::OnRotationCheckBox(wxCommandEvent & e)
{
    EnableRotationCtrls(e.IsChecked());
}

void PreferencesDialog::EnableRotationCtrls(bool enable)
{
    XRCCTRL(*this, "prefs_ft_rot_panel", wxPanel)->Enable(enable);
}

void PreferencesDialog::UpdateDisplayData()
{

    DEBUG_DEBUG("Updating display data");

    double d;
    wxString tstr;
    wxConfigBase *cfg = wxConfigBase::Get();

    // Panotools settings
    MY_STR_VAL("prefs_pt_PTStitcherEXE", cfg->Read(wxT("/Panotools/PTStitcherExe"),wxT("PTStitcher")));
    MY_STR_VAL("prefs_pt_PTOptimizerEXE", cfg->Read(wxT("/Panotools/PTOptimizerExe"),wxT("PTOptimizer")));

    MY_STR_VAL("prefs_pt_ScriptFile", cfg->Read(wxT("/PanoTools/ScriptFile"),wxT("PT_script.txt")));

    // finetune settings
    MY_SPIN_VAL("prefs_ft_TemplateSize",
                cfg->Read(wxT("/Finetune/TemplateSize"),HUGIN_FT_TEMPLATE_SIZE));
    MY_SPIN_VAL("prefs_ft_SearchAreaPercent",cfg->Read(wxT("/Finetune/SearchAreaPercent"),
                                               HUGIN_FT_SEARCH_AREA_PERCENT));
    MY_SPIN_VAL("prefs_ft_LocalSearchWidth", cfg->Read(wxT("/Finetune/LocalSearchWidth"),
                                               HUGIN_FT_LOCAL_SEARCH_WIDTH));

    d=HUGIN_FT_CORR_THRESHOLD;
    cfg->Read(wxT("/Finetune/CorrThreshold"), &d, HUGIN_FT_CORR_THRESHOLD);
    tstr = utils::doubleToString(d).c_str();
    MY_STR_VAL("prefs_ft_CorrThreshold", tstr);

    cfg->Read(wxT("/Finetune/CurvThreshold"), &d, HUGIN_FT_CURV_THRESHOLD);
    tstr = utils::doubleToString(d).c_str();
    MY_STR_VAL("prefs_ft_CurvThreshold", tstr);

    bool t = cfg->Read(wxT("/Finetune/RotationSearch"), HUGIN_FT_ROTATION_SEARCH) == 1;
    MY_BOOL_VAL("prefs_ft_RotationSearch", t);
    EnableRotationCtrls(t);

    d = HUGIN_FT_ROTATION_START_ANGLE;
    cfg->Read(wxT("/Finetune/RotationStartAngle"),&d,HUGIN_FT_ROTATION_START_ANGLE);
    MY_SPIN_VAL("prefs_ft_RotationStartAngle", utils::roundi(d))

    d = HUGIN_FT_ROTATION_STOP_ANGLE;
    cfg->Read(wxT("/Finetune/RotationStopAngle"), &d, HUGIN_FT_ROTATION_STOP_ANGLE);
    MY_SPIN_VAL("prefs_ft_RotationStopAngle", utils::roundi(d));

    MY_SPIN_VAL("prefs_ft_RotationSteps", cfg->Read(wxT("/Finetune/RotationSteps"),
                                            HUGIN_FT_ROTATION_STEPS));

    /////
    /// MISC

    // memory setting
    long mem = cfg->Read(wxT("/ImageCache/UpperBound"), HUGIN_IMGCACHE_UPPERBOUND);
    MY_SPIN_VAL("prefs_cache_UpperBound", mem >> 20);

    // cursor setting
    mem = cfg->Read(wxT("/CPImageCtrl/CursorType"), HUGIN_CP_CURSOR);
    MY_SPIN_VAL("prefs_cp_CursorType", mem);

    // tempdir
    MY_STR_VAL("prefs_misc_tempdir", cfg->Read(wxT("tempDir"),wxT("")));
    
    // show druid
    MY_BOOL_VAL("prefs_misc_showDruid", cfg->Read(wxT("/PreviewFrame/showDruid"),HUGIN_PREVIEW_SHOW_DRUID));

    /////
    /// AUTOPANO

    // active autopano
    MY_CHOICE_VAL("prefs_AutoPanoType", cfg->Read(wxT("/AutoPano/Type"), HUGIN_AP_TYPE));

    // Autopano-SIFT
    MY_STR_VAL("prefs_AutoPanoSIFTExe", cfg->Read(wxT("/AutoPanoSift/AutopanoExe"),
                                                  wxT(HUGIN_APSIFT_EXE)));
    MY_STR_VAL("prefs_AutoPanoSIFTArgs", cfg->Read(wxT("/AutoPanoSift/Args"),
                                                   wxT(HUGIN_APSIFT_ARGS)));

    // Autopano
    MY_STR_VAL("prefs_AutoPanoKolorExe", cfg->Read(wxT("/AutoPanoKolor/AutopanoExe"),
                                                  wxT(HUGIN_APKOLOR_EXE)));
    MY_STR_VAL("prefs_AutoPanoKolorArgs", cfg->Read(wxT("/AutoPanoKolor/Args"),
                                                   wxT(HUGIN_APKOLOR_ARGS)));


    /////
    /// ENBLEND
    MY_STR_VAL("prefs_enblend_EnblendExe", cfg->Read(wxT("/Enblend/EnblendExe"),
                                                     wxT(HUGIN_ENBLEND_EXE)));
    MY_STR_VAL("prefs_enblend_EnblendArgs", cfg->Read(wxT("/Enblend/EnblendArgs"),
                                                      wxT(HUGIN_ENBLEND_ARGS)));


}

void PreferencesDialog::UpdateConfigData()
{
    wxConfigBase *cfg = wxConfigBase::Get();
    // Panotools settings

    cfg->Write(wxT("/Panotools/PTStitcherExe"), MY_G_STR_VAL("prefs_pt_PTStitcherEXE"));
    cfg->Write(wxT("/Panotools/PTOptimizerExe"), MY_G_STR_VAL("prefs_pt_PTOptimizerEXE"));
    cfg->Write(wxT("/PanoTools/ScriptFile"), MY_G_STR_VAL("prefs_pt_ScriptFile"));

    // Fine tune settings
    cfg->Write(wxT("/Finetune/TemplateSize"), MY_G_SPIN_VAL("prefs_ft_TemplateSize"));
    cfg->Write(wxT("/Finetune/LocalSearchWidth"), MY_G_SPIN_VAL("prefs_ft_LocalSearchWidth"));
    wxString t = MY_G_STR_VAL("prefs_ft_CorrThreshold");
    double td= HUGIN_FT_CORR_THRESHOLD;
    utils::stringToDouble(std::string(t.mb_str()), td);
    cfg->Write(wxT("/Finetune/CorrThreshold"), td);

    t = MY_G_STR_VAL("prefs_ft_CurvThreshold");
    td = HUGIN_FT_CURV_THRESHOLD;
    utils::stringToDouble(std::string(t.mb_str()), td);
    cfg->Write(wxT("/Finetune/CurvThreshold"), td);

    cfg->Write(wxT("/Finetune/RotationSearch"), MY_G_BOOL_VAL("prefs_ft_RotationSearch"));
    cfg->Write(wxT("/Finetune/RotationStartAngle"), (double) MY_G_SPIN_VAL("prefs_ft_RotationStartAngle"));
    cfg->Write(wxT("/Finetune/RotationStopAngle"), (double) MY_G_SPIN_VAL("prefs_ft_RotationStopAngle"));
    cfg->Write(wxT("/Finetune/RotationSteps"), MY_G_SPIN_VAL("prefs_ft_RotationSteps"));

    /////
    /// MISC
    // cache
    cfg->Write(wxT("/ImageCache/UpperBound"), MY_G_SPIN_VAL("prefs_cache_UpperBound") << 20);
    // cursor
    cfg->Write(wxT("/CPImageCtrl/CursorType"), MY_G_SPIN_VAL("prefs_cp_CursorType"));
    // tempdir
    cfg->Write(wxT("tempDir"),MY_G_STR_VAL("prefs_misc_tempdir"));
    // druid
    cfg->Write(wxT("/PreviewFrame/showDruid"), MY_G_BOOL_VAL("prefs_misc_showDruid"));
    
    
    /////
    /// AUTOPANO
    cfg->Write(wxT("/AutoPano/Type"),MY_G_CHOICE_VAL("prefs_AutoPanoType"));

    cfg->Write(wxT("/AutoPanoSift/AutopanoExe"),MY_G_STR_VAL("prefs_AutoPanoSIFTExe"));
    cfg->Write(wxT("/AutoPanoSift/Args"),MY_G_STR_VAL("prefs_AutoPanoSIFTArgs"));

    cfg->Write(wxT("/AutoPanoKolor/AutopanoExe"),MY_G_STR_VAL("prefs_AutoPanoKolorExe"));
    cfg->Write(wxT("/AutoPanoKolor/Args"),MY_G_STR_VAL("prefs_AutoPanoKolorArgs"));


    /////
    /// ENBLEND
    cfg->Write(wxT("/Enblend/EnblendExe"), MY_G_STR_VAL("prefs_enblend_EnblendExe"));
    cfg->Write(wxT("/Enblend/EnblendArgs"), MY_G_STR_VAL("prefs_enblend_EnblendArgs"));

}
