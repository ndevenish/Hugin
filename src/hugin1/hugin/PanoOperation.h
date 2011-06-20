// -*- c-basic-offset: 4 -*-
/**  @file PanoOperation.h
 *
 *  @brief Definition of PanoOperation class
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

#ifndef PANOOPERATION_H
#define PANOOPERATION_H

#include "panoinc_WX.h"
#include "panoinc.h"
#include <wx/treectrl.h>
#include "icpfind/CPDetectorConfig.h"

/** base class for different PanoOperations 
  * derived classes should overwrite protected PanoOperation::GetInternalCommand to implement the operation
  */
class PanoOperation : public wxTreeItemData
{
public:
    /** return true, if operation is enabled with the given image set */
    virtual bool IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images);
    /** returns the appropriate PT::PanoCommand to be inserted into GlobalCmdHistory, checks if operation is enabled
      * @returns pointer to valid PT::PanoCommand or NULL if not enabled*/
    virtual PT::PanoCommand* GetCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
protected:
    /** main working function, overwrite it in derived classes */
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)=0;
};

/** PanoOperation which works only with one selected image */
class PanoSingleImageOperation : public PanoOperation
{
public:
    /** return true, if operation is enabled with the given image set */
    virtual bool IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images);
};

/** PanoOperation with works with at least one image */
class PanoMultiImageOperation : public PanoOperation
{
public:
    /** return true, if operation is enabled with the given image set */
    virtual bool IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images);
};

/** PanoOperation to add several user selected images to the panorama */
class AddImageOperation : public PanoOperation
{
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to add all image in a defined timeinterval to the panorama */
class AddImagesSeriesOperation : public PanoOperation
{
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to remove selected images */
class RemoveImageOperation : public PanoMultiImageOperation
{
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to change anchor image */
class ChangeAnchorImageOperation : public PanoSingleImageOperation
{
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to change exposure anchor image */
class ChangeColorAnchorImageOperation : public PanoSingleImageOperation
{
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to create a new stack */
class NewStackOperation : public PanoMultiImageOperation
{
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to change stack number of images */
class ChangeStackOperation : public PanoOperation
{
public:
    /** return true, if operation is enabled with the given image set */
    virtual bool IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images);
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to remove control points */
class RemoveControlPointsOperation : public PanoOperation
{
public:
    /** return true, if operation is enabled with the given image set */
    virtual bool IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images);
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to call control point detector */
class GenerateControlPointsOperation : public RemoveControlPointsOperation
{
public:
    /** constructor, init with given CPDetectorSetting */
    GenerateControlPointsOperation(CPDetectorSetting setting) : RemoveControlPointsOperation(),m_setting(setting) {};
protected:
    /** return true, if operation is enabled with the given image set */
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
private:
    CPDetectorSetting m_setting;
};

/** PanoOperation to clean control points with statistically method */
class CleanControlPointsOperation : public PanoOperation
{
public:
    /** return true, if operation is enabled with the given image set */
    virtual bool IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images);
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to clean control points with Celeste */
class CelesteOperation : public CleanControlPointsOperation
{
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

#ifdef HUGIN_HSI
/** PanoOperation to run a python script with given selected images */
class PythonOperation : public PanoOperation
{
public:
    /** constructor */
    PythonOperation(wxFileName scriptFilename):PanoOperation(),m_filename(scriptFilename){};
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
    wxFileName m_filename;
};
#endif

#endif