// -*- c-basic-offset: 4 -*-

/** @file ChoosyRemapper.cpp
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

#ifdef __WXMAC__
#include "panoinc_WX.h"
#include "panoinc.h"
#endif

#include "ChoosyRemapper.h"
#include "ViewState.h"
#include "TexCoordRemapper.h"
#include "VertexCoordRemapper.h"

ChoosyRemapper::ChoosyRemapper(HuginBase::Panorama *m_pano,
                               HuginBase::SrcPanoImage * image, VisualizationState *visualization_state)
  : MeshRemapper(m_pano, image, visualization_state)
{
    selected_remapper = 0;
    selection = REMAP_NONE;
}

ChoosyRemapper::~ChoosyRemapper()
{
    if (selected_remapper) delete selected_remapper;
}

void ChoosyRemapper::UpdateAndResetIndex()
{
    MeshRemapper::UpdateAndResetIndex();
    // have a look at the output mode, find those where the poles cause problems
    // with the vetex remapper.
    HuginBase::PanoramaOptions *opts = visualization_state->GetOptions();
    bool pole = false;
    switch (opts->getProjection())
    {
        // the 'stretchy poles' cases.
        case HuginBase::PanoramaOptions::EQUIRECTANGULAR:
        case HuginBase::PanoramaOptions::MERCATOR:
        case HuginBase::PanoramaOptions::TRANSVERSE_MERCATOR:
        case HuginBase::PanoramaOptions::CYLINDRICAL:
        case HuginBase::PanoramaOptions::LAMBERT:
        case HuginBase::PanoramaOptions::MILLER_CYLINDRICAL:
// FIXME ARCHITECTURAL is top MILLER and bottom LAMBERT. Will need different remapper for top and bottom half
// currently the least distorted result is by treating it like a 'stretchy pole'
        case HuginBase::PanoramaOptions::ARCHITECTURAL:
        // the circular ones are especially important, they tend to stretch the
        // area over the pole covers over the entire image, it is difficult to
        // correct those.
        case HuginBase::PanoramaOptions::STEREOGRAPHIC:
        case HuginBase::PanoramaOptions::FULL_FRAME_FISHEYE:
        case HuginBase::PanoramaOptions::LAMBERT_AZIMUTHAL:
        case HuginBase::PanoramaOptions::ALBERS_EQUAL_AREA_CONIC:
        case HuginBase::PanoramaOptions::ORTHOGRAPHIC:
        case HuginBase::PanoramaOptions::EQUISOLID:
        case HuginBase::PanoramaOptions::THOBY_PROJECTION:
        // Add any projections where the poles maps to a big set of points here.
        case HuginBase::PanoramaOptions::PANINI:
        case HuginBase::PanoramaOptions::EQUI_PANINI:
		case HuginBase::PanoramaOptions::BIPLANE:
		case HuginBase::PanoramaOptions::TRIPLANE:
        case HuginBase::PanoramaOptions::GENERAL_PANINI:
            // check for pole crossing
        {
            OutputProjectionInfo *info = visualization_state->GetProjectionInfo();
            // get the pole in image coordinates
            transform.createTransform(*image,
                                      *(visualization_state->GetOptions()));
            double img_x, img_y;
            transform.transformImgCoord(img_x, img_y,
                                        info->GetNorthPoleX(),
                                        info->GetNorthPoleY());
            if (   img_y > 0.0 && img_y < height
                && img_x > 0.0 && img_x < width)
            {
                pole = true; // it covers this pole
            }
            if (opts->getProjection() ==
                            HuginBase::PanoramaOptions::ALBERS_EQUAL_AREA_CONIC)
            {
                // always use a TexCoordRemapper for Alber's equal area conic.
                pole = true;
                // FIXME This is not sutible for panoramas containing many small
                // images. To detect the poles in Alber's Equal area conic
                // projetion, we need to account for their movement with
                // changing parameters. Also the 180 degree seam goes at a 
                // funny angle, and we need to account for this.
            }
            if (!pole)
            {
                // check the other pole
                transform.transformImgCoord(img_x, img_y,
                                           info->GetSouthPoleX(),
                                           info->GetSouthPoleY());
                if (   img_y > 0.0 && img_y < height
                    && img_x > 0.0 && img_x < width)
                {
                    pole = true; // it covers this pole.
                }
            }
            if (pole)
            {
                // the VertexCoordRemapper doesn't fair well with images where
                // a single point on an image covers a range on the output.
                // Use the TexCoordRemmaper instead.
                if (selection != REMAP_TEX)
                {
                    selection = REMAP_TEX;
                    if (selected_remapper)
                    {
                        delete selected_remapper;
                        selected_remapper = 0;
                    }
                    selected_remapper = new TexCoordRemapper(m_pano, 
                                                             image,
                                                             visualization_state);
                }
                break;            
            }
            // not breking to get a VertexCoordRemapper.
        }
        default:
            // A VertexCoordRemapper is generally the best. Create a new one
            // if we do not already have one.
            if (selection != REMAP_VERTEX)
            {
                selection = REMAP_VERTEX;
                if (selected_remapper)
                {
                    delete selected_remapper;
                    selected_remapper = 0;
                }
                selected_remapper = new VertexCoordRemapper(m_pano, 
                                                            image,
                                                            visualization_state);
            }
            break;
    }
    // now we get the selected remapper to actually do the work.
    selected_remapper->UpdateAndResetIndex();
}

bool ChoosyRemapper::GetNextFaceCoordinates(Coords *result)
{
    // just use the remapper created already:
    return selected_remapper->GetNextFaceCoordinates(result);
}

