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
    MY_STR_VAL("prefs_pt_PTStitcherEXE", cfg->Read("/Panotools/PTStitcherExe",wxT("PTStitcher")));
    MY_STR_VAL("prefs_pt_PTOptimizerEXE", cfg->Read("/Panotools/PTOptimizerExe",wxT("PTOptimizer")));

    MY_STR_VAL("prefs_pt_ScriptFile", cfg->Read("/PanoTools/ScriptFile","PT_script.txt"));

    // finetune settings
    MY_SPIN_VAL("prefs_ft_TemplateSize",
                cfg->Read("/Finetune/TemplateSize",HUGIN_FT_TEMPLATE_SIZE));
    MY_SPIN_VAL("prefs_ft_SearchAreaPercent",cfg->Read("/Finetune/SearchAreaPercent",
                                               HUGIN_FT_SEARCH_AREA_PERCENT));
    MY_SPIN_VAL("prefs_ft_LocalSearchWidth", cfg->Read("/Finetune/LocalSearchWidth",
                                               HUGIN_FT_LOCAL_SEARCH_WIDTH));

    d=HUGIN_FT_CORR_THRESHOLD;
    cfg->Read("/Finetune/CorrThreshold", &d, HUGIN_FT_CORR_THRESHOLD);
    tstr = utils::doubleToString(d).c_str();
    MY_STR_VAL("prefs_ft_CorrThreshold", tstr);

    bool t = cfg->Read("/Finetune/RotationSearch", HUGIN_FT_ROTATION_SEARCH) == 1;
    MY_BOOL_VAL("prefs_ft_RotationSearch", t);
    EnableRotationCtrls(t);

    d = HUGIN_FT_ROTATION_START_ANGLE;
    cfg->Read("/Finetune/RotationStartAngle",&d,HUGIN_FT_ROTATION_START_ANGLE);
    MY_SPIN_VAL("prefs_ft_RotationStartAngle", utils::roundi(d))

    d = HUGIN_FT_ROTATION_STOP_ANGLE;
    cfg->Read("/Finetune/RotationStopAngle", &d, HUGIN_FT_ROTATION_STOP_ANGLE);
    MY_SPIN_VAL("prefs_ft_RotationStopAngle", utils::roundi(d));

    MY_SPIN_VAL("prefs_ft_RotationSteps", cfg->Read("/Finetune/RotationSteps",
                                            HUGIN_FT_ROTATION_STEPS));
    
    /////
    /// MISC

    // memory setting
    long mem = cfg->Read("/ImageCache/UpperBound", HUGIN_IMGCACHE_UPPERBOUND);
    MY_SPIN_VAL("prefs_cache_UpperBound", mem >> 20);

    // cursor setting
    mem = cfg->Read("/CPImageCtrl/CursorType", HUGIN_CP_CURSOR);
    MY_SPIN_VAL("prefs_cp_CursorType", mem);
    
    // tempdir
    MY_STR_VAL("prefs_misc_tempdir", cfg->Read("tempDir",""));

    /////
    /// AUTOPANO

    // active autopano
    MY_CHOICE_VAL("prefs_AutoPanoType", cfg->Read("/AutoPano/Type", HUGIN_AP_TYPE));
    
    // Autopano-SIFT
    MY_STR_VAL("prefs_AutoPanoSIFTExe", cfg->Read("/AutoPanoSift/AutopanoExe",
                                                  HUGIN_APSIFT_EXE));
    MY_STR_VAL("prefs_AutoPanoSIFTArgs", cfg->Read("/AutoPanoSift/AutopanoArgs",
                                                   HUGIN_APSIFT_ARGS));

    // Autopano
    MY_STR_VAL("prefs_AutoPanoKolorExe", cfg->Read("/AutoPanoKolor/AutopanoExe",
                                                  HUGIN_APKOLOR_EXE));
    MY_STR_VAL("prefs_AutoPanoKolorArgs", cfg->Read("/AutoPanoKolor/AutopanoArgs",
                                                   HUGIN_APKOLOR_ARGS));

    
    /////
    /// ENBLEND
    MY_STR_VAL("prefs_enblend_EnblendExe", cfg->Read("/Enblend/EnblendExe",
                                                     HUGIN_ENBLEND_EXE));
    MY_STR_VAL("prefs_enblend_EnblendArgs", cfg->Read("/Enblend/EnblendArgs",
                                                      HUGIN_ENBLEND_ARGS));
    
    
}

void PreferencesDialog::UpdateConfigData()
{
    wxConfigBase *cfg = wxConfigBase::Get();
    // Panotools settings

    cfg->Write("/Panotools/PTStitcherExe", MY_G_STR_VAL("prefs_pt_PTStitcherEXE"));
    cfg->Write("/Panotools/PTOptimizerExe", MY_G_STR_VAL("prefs_pt_PTOptimizerEXE"));
    cfg->Write("/PanoTools/ScriptFile", MY_G_STR_VAL("prefs_pt_ScriptFile"));

    // Fine tune settings
    cfg->Write("/Finetune/TemplateSize", MY_G_SPIN_VAL("prefs_ft_TemplateSize"));
    cfg->Write("/Finetune/LocalSearchWidth", MY_G_SPIN_VAL("prefs_ft_LocalSearchWidth"));
    wxString t = MY_G_STR_VAL("prefs_ft_CorrThreshold");
    double td= HUGIN_FT_CORR_THRESHOLD;
    utils::stringToDouble(t.c_str(), td);
    cfg->Write("/Finetune/CorrThreshold", td);

    cfg->Write("/Finetune/RotationSearch", MY_G_BOOL_VAL("prefs_ft_RotationSearch"));
    cfg->Write("/Finetune/RotationStartAngle", (double) MY_G_SPIN_VAL("prefs_ft_RotationStartAngle"));
    cfg->Write("/Finetune/RotationStopAngle", (double) MY_G_SPIN_VAL("prefs_ft_RotationStopAngle"));
    cfg->Write("/Finetune/RotationSteps", MY_G_SPIN_VAL("prefs_ft_RotationSteps"));

    /////
    /// MISC
    // cache
    cfg->Write("/ImageCache/UpperBound", MY_G_SPIN_VAL("prefs_cache_UpperBound") << 20);
    // cursor
    cfg->Write("/CPImageCtrl/CursorType", MY_G_SPIN_VAL("prefs_cp_CursorType"));
    // tempdir
    cfg->Write("tempDir",MY_G_STR_VAL("prefs_misc_tempdir"));
    
    /////
    /// AUTOPANO
    cfg->Write("/AutoPano/Type",MY_G_CHOICE_VAL("prefs_AutoPanoType"));

    cfg->Write("/AutoPanoSift/AutopanoExe",MY_G_STR_VAL("prefs_AutoPanoSIFTExe"));
    cfg->Write("/AutoPanoSift/AutopanoArgs",MY_G_STR_VAL("prefs_AutoPanoSIFTArgs"));

    cfg->Write("/AutoPanoKolor/AutopanoExe",MY_G_STR_VAL("prefs_AutoPanoKolorExe"));
    cfg->Write("/AutoPanoKolor/AutopanoArgs",MY_G_STR_VAL("prefs_AutoPanoKolorArgs"));
    
    
    /////
    /// ENBLEND
    cfg->Write("/Enblend/EnblendExe", MY_G_STR_VAL("prefs_enblend_EnblendExe"));
    cfg->Write("/Enblend/EnblendArgs", MY_G_STR_VAL("prefs_enblend_EnblendArgs"));

}
