#ifndef __OVERVIEW_CAMERA_TOOL_H__
#define __OVERVIEW_CAMERA_TOOL_H__

#include "Tool.h"
#include "ToolHelper.h"

class OverviewCameraTool : public OverviewTool
{
    public:
        OverviewCameraTool(OverviewToolHelper* helper) : OverviewTool (helper) {}
        virtual ~OverviewCameraTool() {}

        void Activate();

        void MouseMoveEvent(double x, double y, wxMouseEvent & e);
        void MouseButtonEvent(wxMouseEvent &e);

        

    private:

        bool down;
        double start_x, start_y;
        double start_angx, start_angy;
};

#endif /* __OVERVIEW_DRAG_TOOL_H__ */

