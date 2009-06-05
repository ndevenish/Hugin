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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* The PreviewDifferenceTool finds the topmost image underneath the mouse
 * pontier, which it cancels the normal draw for, and then subtracts it from the
 * rest of the images.
 * It is meant to be the fast preview's equivalent of the accurate preview's
 * difference mode.
 * WARNING: Since the fast preview is approximate, this will show errors that
 * don't really exist.
 */

#ifndef _PREVIEWDIFFERENCETOOL_H
#define _PREVIEWDIFFERENCETOOL_H

#include "PreviewTool.h"

class PreviewDifferenceTool : public PreviewTool
{
public:
    PreviewDifferenceTool(PreviewToolHelper *helper);
    // ~PreviewIdentifyTool();
    /** check, if graphic card supports the necessary modes for difference tool
     * call this procedure first, when there is a OpenGL context
     * @return true, if graphic card supports difference mode
    */
    static bool CheckOpenGLCanDifference();
    void Activate();
    void ImagesUnderMouseChangedEvent();
    void AfterDrawImagesEvent();
    bool BeforeDrawImageEvent(unsigned int image);
    // these are called when the user moves the mouse over the image buttons.
private:
    unsigned int image_number;
    bool over_image;
};

#endif

