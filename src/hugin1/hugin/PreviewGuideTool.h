// -*- c-basic-offset: 4 -*-

/** @file PreviewGuideTool.h
 *
 *  @author T. Modes
 *
 *  @brief interface to ToolHelper for drawing guide lines over pano
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _PREVIEWGUIDETOOL_H
#define _PREVIEWGUIDETOOL_H

#include "Tool.h"

/** Draws guide lines over the panorama in fast preview window
 */
class PreviewGuideTool : public PreviewTool
{
public:
    enum Guides
    {
        NONE=0,
        THIRDS=1,
        GOLDENRATIO=2,
        DIAGONAL=3,
        DIAGONAL_METHOD=4,
        TRIANGLE_DOWN=5,
        TRIANGLE_UP=6
    };
    /** constructor */
    explicit PreviewGuideTool(PreviewToolHelper *helper);
    /** activate the tool */
    void Activate();
    /** draws the lines */
    void ReallyAfterDrawImagesEvent();
    /** sets the guide style to the given style */
    void SetGuideStyle(const Guides newGuideStyle);
    /** returns the current guide style */
    const Guides GetGuideStyle() const;
private:
    Guides m_guide;
};

#endif
