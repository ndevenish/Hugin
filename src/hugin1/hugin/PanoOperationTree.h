// -*- c-basic-offset: 4 -*-
/**  @file PanoOperationTree.h
 *
 *  @brief Definition of PanoOperationTreeCtrl and PanoOperationTreeCtrlXmlHandler class
 *
 *  @author T. Modes
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

#ifndef PANOOPERATIONTREE_H
#define PANOOPERATIONTREE_H

// standard wx include
//#include <config.h>
#include "panoinc.h"
#include "panoinc_WX.h"
#include <wx/treectrl.h>
#include <wx/xrc/xh_tree.h>
#include "icpfind/CPDetectorConfig.h"
#ifdef HUGIN_HSI
#include "hugin/PluginItems.h"
#endif

using namespace std;
using namespace PT;

/** tree control for PanoOperations */
class PanoOperationTreeCtrl : public wxTreeCtrl
{
public:
    void GenerateTree();
#ifdef HUGIN_HSI
    void GeneratePythonTree(PluginItems items);
#endif
    void UpdateCPDetectorItems(CPDetectorConfig& configs);
    void UpdateState(PT::Panorama & pano,HuginBase::UIntSet images);
protected:
    void OnLeftDblClick(wxMouseEvent & e);
private:
    void UpdateStateRecursivly(wxTreeItemId startItem,PT::Panorama & pano,HuginBase::UIntSet images, const wxColour colorActive, const wxColour colorInactive);
    wxTreeItemId m_images;
    wxTreeItemId m_stacks;
    wxTreeItemId m_cp;
    wxTreeItemId m_cpDetectors;
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(PanoOperationTreeCtrl)
};

/** xrc handler for PanoOperationTreeCtrl */
class PanoOperationTreeCtrlXmlHandler : public wxTreeCtrlXmlHandler
{
    DECLARE_DYNAMIC_CLASS(PanoOperationTreeCtrlXmlHandler)

public:
    /** Create PanoOperationTreeCtrl from resource */
    virtual wxObject *DoCreateResource();
    /** Internal use to identify right xml handler */
    virtual bool CanHandle(wxXmlNode *node);
};

#endif //PANOOPERATIONTREE_H
