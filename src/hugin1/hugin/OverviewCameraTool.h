#ifndef __OVERVIEW_CAMERA_TOOL_H__
#define __OVERVIEW_CAMERA_TOOL_H__

#include "Tool.h"
#include "ToolHelper.h"

class PanosphereOverviewCameraTool : public PanosphereOverviewTool
{
    public:
        PanosphereOverviewCameraTool(PanosphereOverviewToolHelper* helper) : PanosphereOverviewTool (helper) {}
        virtual ~PanosphereOverviewCameraTool() {}

        void Activate();

        void MouseMoveEvent(double x, double y, wxMouseEvent & e);
        void MouseButtonEvent(wxMouseEvent &e);
        void MouseWheelEvent(wxMouseEvent &);
        

    private:

        static const double limit_low = 1.2;
        static const double limit_high = 5.0;

        bool down;
        double start_x, start_y;
        double start_angx, start_angy;
};


class PlaneOverviewCameraTool : public PlaneOverviewTool
{
    public:
        PlaneOverviewCameraTool(PlaneOverviewToolHelper * helper) : PlaneOverviewTool (helper) {}
        virtual ~PlaneOverviewCameraTool() {}

        void Activate();

        void MouseMoveEvent(double x, double y, wxMouseEvent & e);
        void MouseButtonEvent(wxMouseEvent &e);
        void MouseWheelEvent(wxMouseEvent &);

    private:


        int counter;
        bool down;
        double start_x, start_y;
        double start_pos_x, start_pos_y;
};

#endif /* __OVERVIEW_DRAG_TOOL_H__ */

