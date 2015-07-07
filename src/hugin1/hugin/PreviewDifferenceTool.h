// -*- c-basic-offset: 4 -*-
/** @file PreviewDifferenceTool.h
 *
 *  @author James Legg
 *
 *  This program is free software; you can redistribute it and/or
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

#ifndef _PREVIEWDIFFERENCETOOL_H
#define _PREVIEWDIFFERENCETOOL_H

#include "Tool.h"

/** Finds the topmost image underneath the mouse pontier, cancel it's normal
 * drawing, and then subtract it from the rest of the images.
 * It is meant to be the fast preview's equivalent of the accurate preview's
 * difference mode.
 * @warning Since the fast preview is approximate, this will show errors that
 * don't really exist.
 */
class PreviewDifferenceTool : public Tool
{
public:
    explicit PreviewDifferenceTool(ToolHelper *helper);
    /** check, if graphic card supports the necessary modes for difference tool
     * call this procedure first, when there is a OpenGL context
     * @return true, if graphic card supports difference mode
    */
    static bool CheckOpenGLCanDifference();
    void Activate();
    void ImagesUnderMouseChangedEvent();
    void AfterDrawImagesEvent();
    void BeforeDrawImagesEvent();
    bool BeforeDrawImageEvent(unsigned int image);
private:
    unsigned int image_number;
    bool over_image;
};

#endif

