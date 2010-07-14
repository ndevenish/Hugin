

#include "OverviewCameraTool.h"

void OverviewCameraTool::Activate()
{
    helper->NotifyMe(ToolHelper::MOUSE_MOVE, this);
    helper->NotifyMe(ToolHelper::MOUSE_PRESS, this);
    down = false;
}

void OverviewCameraTool::MouseMoveEvent(double x, double y, wxMouseEvent & e)
{
    if (down) {
        hugin_utils::FDiff2D pos = helper->GetMouseScreenPosition();
        PanosphereOverviewVisualizationState*  state = (PanosphereOverviewVisualizationState*) helper->GetVisualizationStatePtr();
        double b_angx = state->getAngX();
        double b_angy = state->getAngY();
        //FIXME: include a scale factor for the panosphere
        state->setAngX((pos.x - start_x) / 100.0 + start_angx);
        double ey = (pos.y - start_y) / 100.0 + start_angy;
        if (ey >= M_PI / 2.0) {ey = M_PI / 2.0 - 0.0001;}
        if (ey <= -M_PI / 2.0) {ey = -M_PI / 2.0 + 0.0001;}
        state->setAngY(ey);
        state->Redraw();
    }
}

void OverviewCameraTool::MouseButtonEvent(wxMouseEvent &e)
{
    DEBUG_DEBUG("mouse ov drag button");
    if (e.ButtonDown()) {
        if (!helper->IsMouseOverPano()) {
            down = true;
            hugin_utils::FDiff2D pos = helper->GetMouseScreenPosition();
            start_x = pos.x;
            start_y = pos.y;
            PanosphereOverviewVisualizationState*  state = (PanosphereOverviewVisualizationState*) helper->GetVisualizationStatePtr();
            start_angx = state->getAngX();
            start_angy = state->getAngY();
        }
    }
    if (e.ButtonUp()) {
        if (down) {
            down = false;
        }
    }
}

