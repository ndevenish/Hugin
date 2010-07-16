#ifndef __OVERVIEW_OUTLINES_TOOL_H__
#define __OVERVIEW_OUTLINES_TOOL_H__

#include "Tool.h"

class GLViewer;

class OverviewOutlinesTool : public OverviewTool, public HuginBase::PanoramaObserver
{
    public:
        OverviewOutlinesTool(OverviewToolHelper* helper, GLViewer * preview);
        virtual ~OverviewOutlinesTool();

        void Activate();

    void panoramaChanged(HuginBase::PanoramaData &pano);
    void panoramaImagesChanged(HuginBase::PanoramaData&, const HuginBase::UIntSet&) {}

        void MouseMoveEvent(double x, double y, wxMouseEvent & e);

        void AfterDrawImagesEvent();

        class Rect {
        public:
            Rect(double left, double top, double right, double bottom) {
                val[0][0] = left;   val[0][1] = top;
                val[1][0] = left;   val[1][1] = bottom;
                val[2][0] = right;  val[2][1] = bottom;
                val[3][0] = right;  val[3][1] = top;
            }
            Rect transformImgCoord(HuginBase::PTools::Transform *trans) {
                Rect res(0,0,0,0);
                for (int s = 0 ; s < 4 ; s++) {
                    double x,y;
                    trans->transformImgCoord(x,y, val[s][0], val[s][1]);
                    res.val[s][0] = x;
                    res.val[s][1] = y;
                }
                return res;
            }
            void center(double &x, double &y) {
                x = (val[0][0] + val[1][0] + val[2][0] + val[3][0]) / 4.0;
                y = (val[0][1] + val[1][1] + val[2][1] + val[3][1]) / 4.0;
            }
            double val[4][2];
        };

        void DrawRect(double left, double top, double right, double bottom, bool outline);

    private:

        bool dirty_meshes;

        //TODO take into consideration scale of gl canvas
        //resolution of the mesh
        const static double res = 10;
        const static double mindist = 2;
    
        GLViewer * preview;

        unsigned int display_list_number_canvas;
        unsigned int display_list_number_crop;
        unsigned int display_list_number_canvas_outline;
        unsigned int display_list_number_crop_outline;
        /* data */
};

#endif /* __OVERVIEW_OUTLINES_TOOL_H__ */

