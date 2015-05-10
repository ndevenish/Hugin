// -*- c-basic-offset: 4 -*-

/** @file ChoosyRemapper.h
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
 
#ifndef _CHOOSYREMAPPER_H
#define _CHOOSYREMAPPER_H

#include "MeshRemapper.h"

/** A ChoosyRemapper combines the other MeshRemappers and picks which one it
 * deems is best suited for each image.
 */
class ChoosyRemapper : public MeshRemapper
{
public:
    ChoosyRemapper(HuginBase::Panorama *m_pano, HuginBase::SrcPanoImage * image,
                   VisualizationState *visualization_state);
    ~ChoosyRemapper();
    void UpdateAndResetIndex();
    bool GetNextFaceCoordinates(Coords *result);
private:
    enum RemapperSelection {REMAP_NONE, REMAP_VERTEX, REMAP_TEX};
    RemapperSelection selection;
    MeshRemapper *selected_remapper;
};


#endif
