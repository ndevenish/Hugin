// -*- c-basic-offset: 4 -*-
/** @file wxPanoCommand.h
*
*  wxwindows specific panorama commands
*
*  @author Pablo d'Angelo <pablo.dangelo@web.de>
*
*  $Id$
*
*  This is free software; you can redistribute it and/or
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

#ifndef _WXPANOCOMMAND__H
#define _WXPANOCOMMAND__H

#include "panotools/PanoToolsUtils.h"
#include "PanoCommand.h"
#include "hugin_shared.h"
#include "hugin/config_defaults.h"
#include "base_wx/platform.h"

namespace PanoCommand
{

    struct FileIsNewer : public std::binary_function<const std::string &, const std::string &, bool>
    {

        bool operator()(const std::string & file1, const std::string & file2)
        {
            // lets hope the operating system caches files stats.
            return wxFileModificationTime(wxString(file1.c_str(), HUGIN_CONV_FILENAME)) < wxFileModificationTime(wxString(file2.c_str(), HUGIN_CONV_FILENAME));
        };

    };

    /** add image(s) to a panorama */
    class WXIMPEX wxAddImagesCmd : public PanoCommand
    {
    public:
        wxAddImagesCmd(HuginBase::Panorama & pano, const std::vector<std::string> & newfiles)
            : PanoCommand(pano), files(newfiles)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "add images"; };
    private:
        std::vector<std::string> files;
    };

    /** dump the current project and load a new one.
    *
    */
    class WXIMPEX wxLoadPTProjectCmd : public PanoCommand
    {
    public:
        wxLoadPTProjectCmd(HuginBase::Panorama & p, const std::string& filename, const std::string& prefix = "", const bool markAsOptimized = false, const bool clearDirty = true)
            : PanoCommand(p), filename(filename), prefix(prefix), markAsOptimized(markAsOptimized)
        {
            m_clearDirty = clearDirty;
        };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "load project"; };
    private:
        const std::string filename;
        const std::string prefix;
        const bool markAsOptimized;
    };

    /** start a new project, reset options to values in preferences
    *
    */
    class WXIMPEX wxNewProjectCmd : public PanoCommand
    {
    public:
        explicit wxNewProjectCmd(HuginBase::Panorama & p) : PanoCommand(p) { m_clearDirty = true; };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "new project"; };
    };

    /** apply a template to a panorama object
    *
    */
    class WXIMPEX wxApplyTemplateCmd : public PanoCommand
    {
    public:
        wxApplyTemplateCmd(HuginBase::Panorama & p, std::istream & i)
            : PanoCommand(p), in(i)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "apply template"; };
    private:
        std::istream & in;
    };

    /** add a control point */
    class WXIMPEX wxAddCtrlPointGridCmd : public PanoCommand
    {
    public:
        wxAddCtrlPointGridCmd(HuginBase::Panorama & p, unsigned int i1, unsigned int i2, double scale, double threshold)
            : PanoCommand(p), img1(i1), img2(i2), dx(0), dy(0), scale(scale), cornerThreshold(threshold)
        { }
        virtual bool processPanorama(HuginBase::Panorama& pano);

        virtual std::string getName() const { return "add control point"; };
    private:
        unsigned int img1, img2, dx, dy;
        double scale;
        double cornerThreshold;
    };

    /** run a python script */
#ifdef HUGIN_HSI
    class WXIMPEX PythonScriptPanoCmd : public PanoCommand
    {
    public:
        PythonScriptPanoCmd(HuginBase::Panorama & pano, const std::string & scriptFile)
            : PanoCommand(pano), m_scriptFile(scriptFile)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "python script"; };
    private:
        std::string m_scriptFile;
    };
#endif

} // namespace PanoCommand

#endif // _WXPANOCOMMAND__H
