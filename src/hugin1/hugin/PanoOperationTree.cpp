// -*- c-basic-offset: 4 -*-

/** @file PanoOperationTree.cpp
 *
 *  @brief Implementation of PanoOperationTreeCtrl and PanoOperationTreeCtrlXmlHandler class
 *
 *  @author T. Modes
 *
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "hugin/PanoOperationTree.h"
#include "hugin/PanoOperation.h"

BEGIN_EVENT_TABLE(PanoOperationTreeCtrl,wxTreeCtrl)
    EVT_LEFT_DCLICK(PanoOperationTreeCtrl::OnLeftDblClick)
END_EVENT_TABLE()

void PanoOperationTreeCtrl::GenerateTree()
{
    wxTreeItemId root=AddRoot(wxT("root"));
    m_images=AppendItem(root,_("Images"));
    PanoOperation* op=new AddImageOperation();
    AppendItem(m_images,_("Add individual images..."),-1,-1,(wxTreeItemData*)op);
    op=new AddImagesSeriesOperation();
    AppendItem(m_images,_("Add time-series of images..."),-1,-1,(wxTreeItemData*)op);
    op=new RemoveImageOperation();
    AppendItem(m_images,_("Remove selected image(s)"),-1,-1,(wxTreeItemData*)op);
    op=new ChangeAnchorImageOperation();
    AppendItem(m_images,_("Anchor this image for position"),-1,-1,(wxTreeItemData*)op);
    op=new ChangeColorAnchorImageOperation();
    AppendItem(m_images,_("Anchor this image for exposure"),-1,-1,(wxTreeItemData*)op);
    Expand(m_images);
    m_stacks=AppendItem(root,_("Stacks"));
    op=new NewStackOperation();
    AppendItem(m_stacks,_("New stack"),-1,-1,(wxTreeItemData*)op);
    op=new ChangeStackOperation();
    AppendItem(m_stacks,_("Change stack..."),-1,-1,(wxTreeItemData*)op);
    m_cp=AppendItem(root,_("Control points"));
    m_cpDetectors=AppendItem(m_cp,_("Control point detectors"));
    op=new RemoveControlPointsOperation();
    AppendItem(m_cp,_("Remove control points"),-1,-1,(wxTreeItemData*)op);
    op=new CelesteOperation();
    AppendItem(m_cp,_("Run Celeste"),-1,-1,(wxTreeItemData*)op);
    op=new CleanControlPointsOperation();
    AppendItem(m_cp,_("Clean control points"),-1,-1,(wxTreeItemData*)op);
};

void PanoOperationTreeCtrl::UpdateCPDetectorItems(CPDetectorConfig& config)
{
    DeleteChildren(m_cpDetectors);
    if(config.GetCount()>0)
    {
        for(size_t i=0;i<config.GetCount();i++)
        {
            CPDetectorSetting setting=config.settings[i];
            PanoOperation* op=new GenerateControlPointsOperation(setting);
            AppendItem(m_cpDetectors,setting.GetCPDetectorDesc(),-1,-1,(wxTreeItemData*)op);
        };
    };
    Expand(m_cpDetectors);
};

#if defined HUGIN_HSI
void PanoOperationTreeCtrl::GeneratePythonTree(PluginItems items)
{
    for(PluginItems::const_iterator it=items.begin();it!=items.end();it++)
    {
        wxTreeItemId id;
        switch((*it).GetPluginType())
        {
            case PluginItem::DefaultPlugin:
                continue;
                break;
            case PluginItem::ImagePlugin:
                id=m_images;
                break;
            case PluginItem::StackPlugin:
                id=m_stacks;
                break;
            case PluginItem::ControlpointPlugin:
                id=m_cp;
                break;
        };
        PanoOperation* op=new PythonOperation((*it).GetFilename());
        AppendItem(id,(*it).GetName(),-1,-1,(wxTreeItemData*)op);
    };
};
#endif

void PanoOperationTreeCtrl::OnLeftDblClick(wxMouseEvent & e)
{
    wxCommandEvent ev(wxEVT_COMMAND_BUTTON_CLICKED,XRCID("images_execute_operation"));
    GetParent()->GetEventHandler()->AddPendingEvent(ev);
};

void PanoOperationTreeCtrl::UpdateState(PT::Panorama & pano,HuginBase::UIntSet images)
{
    wxTreeItemId item=GetRootItem();
    if(item.IsOk())
    {
        Freeze();
        wxColour color1=wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
        wxColour color2=wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
        UpdateStateRecursivly(item,pano,images,color1,color2);
        Thaw();
    };
};

void PanoOperationTreeCtrl::UpdateStateRecursivly(wxTreeItemId startItem,PT::Panorama & pano,HuginBase::UIntSet images,const wxColour colorActive, const wxColour colorInactive)
{
    if(HasChildren(startItem))
    {
        wxTreeItemIdValue cookie;
        wxTreeItemId item=GetFirstChild(startItem,cookie);
        while(item.IsOk())
        {
            PanoOperation* op=(PanoOperation*)(GetItemData(item));
            if(op!=NULL)
            {
                if(op->IsEnabled(pano,images))
                {
                    SetItemTextColour(item,colorActive);
                }
                else
                {
                    SetItemTextColour(item,colorInactive);
                };
            };
            UpdateStateRecursivly(item,pano,images,colorActive,colorInactive);
            item=GetNextChild(startItem,cookie);
        };
    };
};

IMPLEMENT_DYNAMIC_CLASS(PanoOperationTreeCtrl, wxTreeCtrl)

IMPLEMENT_DYNAMIC_CLASS(PanoOperationTreeCtrlXmlHandler, wxTreeCtrlXmlHandler)

wxObject *PanoOperationTreeCtrlXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(tree, PanoOperationTreeCtrl)

    tree->Create(m_parentAsWindow,
                GetID(),
                GetPosition(), GetSize(),
                GetStyle(wxT("style"), wxTR_DEFAULT_STYLE),
                wxDefaultValidator,
                GetName());

#if wxCHECK_VERSION(2,9,0)
    wxImageList *imagelist = GetImageList();
    if ( imagelist )
        tree->AssignImageList(imagelist);
#endif

    SetupWindow(tree);

    return tree;
}

bool PanoOperationTreeCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("PanoOperationTree"));
}
