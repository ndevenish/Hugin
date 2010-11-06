

#include "OverviewDragTool.h"

void OverviewDragTool::Activate()
{
    helper->NotifyMe(ToolHelper::MOUSE_MOVE, this);
    helper->NotifyMe(ToolHelper::MOUSE_PRESS, this);
    down = false;
}

void OverviewDragTool::MouseMoveEvent(double x, double y, wxMouseEvent & e)
{
    if (down) {
        hugin_utils::FDiff2D pos = helper->GetMousePosition();
        PanosphereOverviewVisualizationState*  state = (PanosphereOverviewVisualizationState*) helper->GetVisualizationStatePtr();
        double b_angx = state->getAngX();
        double b_angy = state->getAngY();
        //FIXME: include a scale factor for the panosphere
        state->setAngX((pos.x - start_x) / 100.0 + start_angx);
        state->setAngY((pos.y - start_y) / 100.0 + start_angy);
        state->Redraw();
    }
}

void OverviewDragTool::MouseButtonEvent(wxMouseEvent &e)
{
    if (e.ButtonDown()) {
        down = true;
        hugin_utils::FDiff2D pos = helper->GetMousePosition();
        start_x = pos.x;
        start_y = pos.y;
        PanosphereOverviewVisualizationState*  state = (PanosphereOverviewVisualizationState*) helper->GetVisualizationStatePtr();
        start_angx = state->getAngX();
        start_angy = state->getAngY();
    }
    if (e.ButtonUp()) {
        if (down) {
            down = false;
        }
    }
}

