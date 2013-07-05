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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _WXPANOCOMMAND__H
#define _WXPANOCOMMAND__H

#include "PT/PanoToolsUtils.h"
#include "PT/PanoCommand.h"
#include "vigra/impex.hxx"
#include "hugin/config_defaults.h"
#include "base_wx/platform.h"

namespace PT {

struct FileIsNewer: public std::binary_function<const std::string &, const std::string &, bool>
{

    bool operator()(const std::string & file1, const std::string & file2)
    {
        // lets hope the operating system caches files stats.
        return wxFileModificationTime(wxString(file1.c_str(),HUGIN_CONV_FILENAME)) < wxFileModificationTime(wxString(file2.c_str(),HUGIN_CONV_FILENAME));
    };

};

/** add image(s) to a panorama */
class wxAddImagesCmd : public PanoCommand
{
public:
    wxAddImagesCmd(Panorama & pano, const std::vector<std::string> & newfiles)
    : PanoCommand(pano), files(newfiles)
    { };

    virtual bool processPanorama(Panorama& pano);

    virtual std::string getName() const
        {
            return "add images";
        }
private:
    std::vector<std::string> files;
};



/** dump the current project and load a new one.
 *
 */
class wxLoadPTProjectCmd : public PanoCommand
{
public:
    wxLoadPTProjectCmd(Panorama & p, const std::string filename, const std::string prefix = "", const bool markAsOptimized=false, const bool clearDirty=true)
        : PanoCommand(p),
          filename(filename),
          prefix(prefix),
          markAsOptimized(markAsOptimized)
    { m_clearDirty=clearDirty; };

    virtual bool processPanorama(Panorama& pano);

    virtual std::string getName() const
    {
        return "load project";
    }
private:
    const std::string filename;
    const std::string prefix;
    const bool markAsOptimized;
};

/** start a new project, reset options to values in preferences
 *
 */
class wxNewProjectCmd : public PanoCommand
{
public:
    wxNewProjectCmd(Panorama & p) : PanoCommand(p) { m_clearDirty=true; };

    virtual bool processPanorama(Panorama& pano);

    virtual std::string getName() const
    {
        return "new project";
    }
};


/** dump the current project and load a new one.
 *
 */
class wxApplyTemplateCmd : public PanoCommand
{
public:
    wxApplyTemplateCmd(Panorama & p, std::istream & i)
        : PanoCommand(p),
          in(i)
    { };

    virtual bool processPanorama(Panorama& pano);

    virtual std::string getName() const
    {
        return "apply template";
    }
private:
    std::istream & in;
};

    //=========================================================================
    //=========================================================================


    /** add a control point */
    class wxAddCtrlPointGridCmd : public PanoCommand
    {
    public:
        wxAddCtrlPointGridCmd(Panorama & p, unsigned int i1,
                            unsigned int i2, double scale, double threshold)
            : PanoCommand(p), img1(i1), img2(i2), scale(scale), cornerThreshold(threshold)
            { }

        virtual bool processPanorama(Panorama& pano);

        virtual std::string getName() const
            {
                return "add control point";
            }
    private:
        unsigned int img1,img2,dx,dy;
        double scale;
        double cornerThreshold;
    };

    /** run a python script */
#ifdef HUGIN_HSI
    class PythonScriptPanoCmd : public PanoCommand
    {
    public:
        PythonScriptPanoCmd(Panorama & pano, const std::string & scriptFile)
            : PanoCommand(pano), m_scriptFile(scriptFile)
            { };

        virtual bool processPanorama(Panorama& pano);
        
        virtual std::string getName() const
            {
                return "python script";
            }
    private:
        std::string m_scriptFile;
    };
#endif


} // namespace PT

#endif // _WXPANOCOMMAND__H
