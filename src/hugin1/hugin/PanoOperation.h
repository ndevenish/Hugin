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
#include <vector>
#include "icpfind/CPDetectorConfig.h"

namespace PanoOperation
{
/** base class for different PanoOperations 
  * derived classes should overwrite protected PanoOperation::GetInternalCommand to implement the operation
  */
class PanoOperation
{
public:
    /** return true, if operation is enabled with the given image set */
    virtual bool IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images);
    /** returns the appropriate PT::PanoCommand to be inserted into GlobalCmdHistory, checks if operation is enabled
      * @returns pointer to valid PT::PanoCommand or NULL if not enabled*/
    virtual PT::PanoCommand* GetCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
    virtual wxString GetLabel();
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
public:
    virtual wxString GetLabel();
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to add all image in a defined timeinterval to the panorama */
class AddImagesSeriesOperation : public PanoOperation
{
public:
    virtual wxString GetLabel();
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to remove selected images */
class RemoveImageOperation : public PanoMultiImageOperation
{
public:
    virtual wxString GetLabel();
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to change anchor image */
class ChangeAnchorImageOperation : public PanoSingleImageOperation
{
public:
    virtual wxString GetLabel();
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to change exposure anchor image */
class ChangeColorAnchorImageOperation : public PanoSingleImageOperation
{
public:
    virtual wxString GetLabel();
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to assign new lens */
class NewLensOperation : public PanoOperation
{
public:
    /** return true, if operation is enabled with the given image set */
    virtual bool IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images);
    virtual wxString GetLabel();
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to change lens number */
class ChangeLensOperation : public PanoOperation
{
public:
    /** return true, if operation is enabled with the given image set */
    virtual bool IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images);
    virtual wxString GetLabel();
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to load lens from ini file or lensfun database*/
class LoadLensOperation : public PanoMultiImageOperation
{
public:
    LoadLensOperation(bool fromLensfunDB);
    virtual wxString GetLabel();
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
private:
    bool m_fromLensfunDB;
};

/** PanoOperation to save lens to ini file or lensfun database */
class SaveLensOperation : public PanoSingleImageOperation
{
public:
    SaveLensOperation(int lensInfo);
    virtual wxString GetLabel();
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
private:
    int m_lensInfo;
};

/** PanoOperation to remove control points */
class RemoveControlPointsOperation : public PanoOperation
{
public:
    /** return true, if operation is enabled with the given image set */
    virtual bool IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images);
    virtual wxString GetLabel();
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to clean control points with statistically method */
class CleanControlPointsOperation : public PanoOperation
{
public:
    /** return true, if operation is enabled with the given image set */
    virtual bool IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images);
    virtual wxString GetLabel();
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to reset image variables */
class ResetOperation : public PanoOperation
{
public:
    enum ResetMode
    {
        RESET_DIALOG=0,
        RESET_POSITION,
        RESET_LENS,
        RESET_PHOTOMETRICS,
        RESET_DIALOG_LENS,
        RESET_DIALOG_PHOTOMETRICS,
    };
    ResetOperation(ResetMode newResetMode);
    /** return true, if operation is enabled with the given image set */
    virtual bool IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images);
    virtual wxString GetLabel();
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
private:
    bool ShowDialog(wxWindow* parent);
    ResetMode m_resetMode;
    bool m_resetPos;
    bool m_resetHFOV;
    bool m_resetLens;
    int m_resetExposure;
    bool m_resetVignetting;
    bool m_resetColor;
    bool m_resetCameraResponse;
};

/** PanoOperation to clean control points with Celeste */
class CelesteOperation : public CleanControlPointsOperation
{
public:
    virtual wxString GetLabel();
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to assign new stack */
class NewStackOperation : public PanoOperation
{
public:
    /** return true, if operation is enabled with the given image set */
    virtual bool IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images);
    virtual wxString GetLabel();
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to change lens number */
class ChangeStackOperation : public PanoOperation
{
public:
    /** return true, if operation is enabled with the given image set */
    virtual bool IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images);
    virtual wxString GetLabel();
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

/** PanoOperation to assigns stacks */
class AssignStacksOperation : public PanoOperation
{
public:
    /** return true, if operation is enabled with the given image set */
    virtual bool IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images);
    virtual wxString GetLabel();
protected:
    virtual PT::PanoCommand* GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images);
};

typedef std::vector<PanoOperation*> PanoOperationVector;

/** generates the PanoOperationVector for context menu */
void GeneratePanoOperationVector();
/** clears the PanoOperationVector */
void CleanPanoOperationVector();

/** returns list of PanoOperation for work with images */
PanoOperationVector* GetImagesOperationVector();
/** returns list of PanoOperation for work with lenses */
PanoOperationVector* GetLensesOperationVector();
/** returns list of PanoOperation for stacks */
PanoOperationVector* GetStacksOperationVector();
/** returns list of PanoOperation for work with control points */
PanoOperationVector* GetControlPointsOperationVector();
/** returns list of PanoOperation for resetting */
PanoOperationVector* GetResetOperationVector();

} //namespace
#endif