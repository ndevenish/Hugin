// -*- c-basic-offset: 4 -*-
/** @file PreviewPanoMaskTool.h
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

#ifndef _PREVIEWPANOMASKTOOL_h
#define _PREVIEWPANOMASKTOOL_h

#include "Tool.h"

/** For projections where the output range is limited, but the approximatly
 * remaped images can extend this, we mask out the off-panorama bits with a
 * stencil.
 */
class PreviewPanoMaskTool : public PreviewTool
{
public:
    explicit PreviewPanoMaskTool(PreviewToolHelper *helper);
    void Activate();
    void BeforeDrawImagesEvent();
    void ReallyAfterDrawImagesEvent();
};

#endif

