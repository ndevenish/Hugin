// -*- c-basic-offset: 4 -*-
/** @file MultiLayerImage.h
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

#ifndef _MULTILAYERIMAGE_H
#define _MULTILAYERIMAGE_H

#include <vector>

namespace vigra_ext {

/** A multilayer image is simply a collection consisting
 *  of multiple ROIImages.
 *  
 *  It contains some simple methods to iterate over the image.
 */
template<class Image, class Mask>
struct MultiLayerImage
{

	
    
public:

    typedef typename Image::value_type      image_value_type;
    typedef typename Image::traverser       image_traverser;
    typedef typename Image::const_traverser const_image_traverser;
    typedef typename Image::Accessor        ImageAccessor;
    typedef typename Image::ConstAccessor   ConstImageAccessor;
    
    typedef typename Mask::value_type       mask_value_type;
    typedef typename Mask::traverser        mask_traverser;
    typedef typename Mask::const_traverser  const_mask_traverser;
    typedef typename Mask::Accessor         MaskAccessor;
    typedef typename Mask::ConstAccessor    ConstMaskAccessor;

private:
    typedef typename std::vector<ROIImage<Image, Mask>*> LayerVector;
    typedef typename LayerVector::iterator LayerIterator;
    typedef typename LayerVector::const_iterator ConstLayerIterator;

    typedef typename std::vector<image_value_type> value_type;

public:    
    MultiLayerImage()
    {

    }

    ~MultiLayerImage()
    {
	for (LayerIterator it = m_layers.begin(); it != m_layers.end();
	     ++it)
	{
	    delete (*it);
	}
    }

    unsigned int layers()
    {
	return m_layers.size();
    }

    ROIImage<Image, Mask> & appendLayer()
    {
	ROIImage<Image, Mask> * l =new ROIImage<Image, Mask>;
	m_layers.push_back(l);
	return *l;
    }

    void removeLayer(unsigned int i)
    {
	ROIImage<Image, Mask> * old = m_layers[i];
	m_layers.erase(m_layers.begin() + i);
	delete old;
    }

    /// calculate the bounding box of all images.
    vigra::Rect2D boundingBox()
    {
	vigra::Rect2D res;
	LayerIterator it = m_layers.begin();
	if (it == m_layers.end()) {
	    return res;
	}
	res = (*it)->boundingBox();
	it++;
	while(it != m_layers.end()) {
	   res |= (*it)->boundingBox();
	   ++it;
	}
	return res;
    }
    

    ROIImage<Image, Mask> & GetLayer(unsigned int i);

    /** get all image and mask data at a given point.
     *
     *  They are copied into the containers given by @p image_iter
     *  and @p mask_iter.
     *
     *  This is SLOW.
     *  
     *  */
    template <class ImgIter, class MaskIter>
    void getCut(int x, int y, 
	        ImgIter image_iter, MaskIter mask_iter) const
    {
	mask_value_type m;
	for (ConstLayerIterator it = m_layers.begin(); it != m_layers.end();
	     ++it)
	{
	    m = (*it)->getMask(x,y);
	    (*mask_iter) = m;
	    (*image_iter) = (*it)->operator()(x,y);
	    ++mask_iter;
	    ++image_iter;
	}
    }

protected:
    /** the data storage. */
    LayerVector m_layers;
};


/** a simple blending functor for multilayer images. */


} //namespace

#endif // _MULTILAYERIMAGE_H
