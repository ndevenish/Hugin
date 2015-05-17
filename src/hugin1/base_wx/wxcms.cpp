// -*- c-basic-offset: 4 -*-

/** @file wxcms.cpp
 *
 *  @brief implementation of helper function for color managment
 *
 *  @author T. Modes
 */

/*  This program is free software; you can redistribute it and/or
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "wxcms.h"
#ifdef __WXGTK__
#include <X11/Xlib.h>
#include "hugin_utils/utils.h"
#endif

namespace HuginBase
{
    namespace Color
    {
        namespace detail
        {

#ifdef __WXGTK__
            cmsHPROFILE GetProfileFromAtom(Display* disp, const char* prop_name)
            {
                Atom atom = XInternAtom(disp, prop_name, True);
                if (atom)
                {
                    int actual_format_return;
                    unsigned long nitems_return = 0;
                    unsigned long bytes_after_return = 0;
                    unsigned char* prop_return = 0;
                    Atom a;
                    Window w = XDefaultRootWindow(disp);
                    if (w)
                    {
                        XGetWindowProperty(disp, w, atom, 0, INT_MAX, False,
                            AnyPropertyType,
                            &a, &actual_format_return, &nitems_return,
                            &bytes_after_return, &prop_return);
                        if (nitems_return && prop_return)
                        {
                            cmsHPROFILE profile = cmsOpenProfileFromMem(prop_return, nitems_return);
                            XFree(prop_return);
                            if (profile != NULL)
                            {
                                return profile;
                            };
                        };
                    };
                };
                return NULL;
            }
#endif

            void GetMonitorProfile(wxString& profileName, cmsHPROFILE& profile)
            {
#ifdef __WXMSW__
                // look up monitor profile in system
                HDC hdc = GetDC(NULL);
                if (hdc)
                {
                    wxChar filename[MAX_PATH];
                    DWORD len;
                    if (GetICMProfile(hdc, &len, filename))
                    {
                        profileName = filename;
                        profile = cmsOpenProfileFromFile(profileName.c_str(), "r");
                    };
                    ReleaseDC(NULL, hdc);
                };
#elif __WXGTK__
                Display *disp = XOpenDisplay(0);
                if (disp)
                {
                    profile = GetProfileFromAtom(disp, "_ICC_DEVICE_PROFILE");
                    if (profile == NULL)
                    {
                        profile = GetProfileFromAtom(disp, "_ICC_PROFILE");
                    };
                    if (profile != NULL)
                    {
                        profileName = wxString(hugin_utils::GetICCDesc(profile).c_str(), wxConvLocal);
                    }
                    XSync(disp, False);
                    XCloseDisplay(disp);
                };
#endif
            }
        }

        void GetMonitorProfile(wxString& profileName, cmsHPROFILE& profile)
        {
            if (profile != NULL)
            {
                cmsCloseProfile(profile);
            }
            profileName.Clear();
            detail::GetMonitorProfile(profileName, profile);
            // check if monitor profile could be successful loaded, if not switch back to default sRGB profile
            if (profile == NULL)
            {
                profile = cmsCreate_sRGBProfile();
                profileName.Clear();
            };
        };

        void CorrectImage(wxImage& image, const vigra::ImageImportInfo::ICCProfile& iccProfile, const cmsHPROFILE& monitorProfile)
        {
            cmsHPROFILE inputICC = NULL;
            if (!iccProfile.empty())
            {
                inputICC = cmsOpenProfileFromMem(iccProfile.data(), iccProfile.size());
            };
            // check type of input profile
            if (inputICC != NULL)
            {
                if (cmsGetColorSpace(inputICC) != cmsSigRgbData)
                {
                    cmsCloseProfile(inputICC);
                    inputICC = NULL;
                };
            };
            // if there is no icc profile in file fall back to sRGB
            if (inputICC == NULL)
            {
                inputICC = cmsCreate_sRGBProfile();
            };
            // now build transform
            cmsHTRANSFORM transform = cmsCreateTransform(inputICC, TYPE_RGB_8,
                monitorProfile, TYPE_RGB_8,
                INTENT_PERCEPTUAL, cmsFLAGS_BLACKPOINTCOMPENSATION);
            unsigned char* imgData = image.GetData();
            const int imgWidth = image.GetWidth();
            const int imgHeight = image.GetHeight();
#pragma omp parallel for
            for (int y = 0; y < imgHeight; ++y)
            {
                cmsDoTransform(transform, imgData + 3 * y * imgWidth, imgData + 3 * y * imgWidth, imgWidth);
            };
            cmsDeleteTransform(transform);
            cmsCloseProfile(inputICC);
        };

    }; // namespace Color
}; // namespace HuginBase
