// -*- c-basic-offset: 4 -*-
/** @file LayerImage.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _LAYERIMAGE_H
#define _LAYERIMAGE_H

#include <vigra_ext/ROI.h>

namespace vigra_ext
{

/** struct to hold a layer (Image with ROI).
 *
 *  It is used as a building block for multilayer images.
 *
 */
template <class Image, class AlphaImage>
class LayerImage
{
public:

    /** create an empty layer */
    LayerImage()
    {
    }

    /** create an image layer that covers a given ROI
     */
    LayerImage(ROI<vigra::Diff2D> & roi)
      : m_ROI(roi)
    {
        m_image.resize(m_ROI.size());
	m_alpha.resize(m_ROI.size());

    }

    virtual ~LayerImage()
    {
    }

    /** resize the image
     *
     *  erases the current image content.
     */
    void resize(ROI<vigra::Diff2D> & roi)
    {
        m_image.resize(m_ROI.size());
	m_alpha.resize(m_ROI.size());
    }

    /** save the image & alpha channel to disk and remove it from memory
     *
     *  @TODO implement!
     *
     */
    virtual void save()
        {
            //
        }

    /** load the image & alpha channel to disk and remove it from memory
     *
     *  the image must have been saved before!
     *
     *  @TODO implement!
     *
     */
    virtual void load()
        {
            //
        }

    /** get the image.
     *
     * the iterators point to the upper left and lower right (behind the end)
     * of the image.
     *
     * use getROIOffset() to get the panorama coordinates of the upper left
     * image pixel
     */
    vigra::triple<typename Image::Iterator,
		  typename Image::Iterator,
		  typename Image::Accessor>
    image()
    {
	DEBUG_DEBUG(m_ROI << " size(img): " << m_image.size());
	return vigra::make_triple(m_image.upperLeft(), m_image.lowerRight(), m_image.accessor());
    }

    /** get the alpha channel
     *
     * the iterators point to the upper left and lower right (behind the end)
     * of the image.
     *
     * use getROIOffset() to get the panorama coordinates of the upper left
     * alpha pixel
     */
    vigra::triple<typename AlphaImage::Iterator,
		  typename AlphaImage::Iterator,
		  typename AlphaImage::Accessor>
    alpha()
    {
	DEBUG_DEBUG(m_ROI << " size(alpha): " << m_alpha.size());
	return vigra::make_triple(m_alpha.upperLeft(), m_alpha.lowerRight(), m_alpha.accessor());
    }

    vigra_ext::ROI<vigra::Diff2D> & roi()
    {
	return m_ROI;
    }



protected:
    Image m_image;    ///< remapped image
    AlphaImage m_alpha;    ///< corrosponding alpha channel

    // roi of the remapped image
    vigra_ext::ROI<vigra::Diff2D> m_ROI;

};


} // namespace

#endif // _LAYERIMAGE_H
