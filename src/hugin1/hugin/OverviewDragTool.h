#ifndef __OVERVIEW_DRAG_TOOL_H__
#define __OVERVIEW_DRAG_TOOL_H__

#include "Tool.h"
#include "ToolHelper.h"

class OverviewDragTool : public OverviewTool
{
    public:
        OverviewDragTool(OverviewToolHelper* helper) : OverviewTool (helper) {}
        virtual ~OverviewDragTool() {}

        void Activate();

        void MouseMoveEvent(double x, double y, wxMouseEvent & e);
        void MouseButtonEvent(wxMouseEvent &e);

        

    private:

        bool down;
        double start_x, start_y;
        double start_angx, start_angy;
};

#endif /* __OVERVIEW_DRAG_TOOL_H__ */

