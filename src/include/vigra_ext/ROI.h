// -*- c-basic-offset: 4 -*-
/** @file ROI.h
 *
 *  functions to manage ROI's
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

#ifndef VIGRA_EXT_ROI_H
#define VIGRA_EXT_ROI_H

#include <common/utils.h>

namespace vigra_ext {

/** represents a rectangular 2D ROI.
 *
 *  useful for the intersection of layers, and for applying the
 *  roi to image iterators.
 */
template <class Coord>
class ROI
{
public:

    ROI()
    {
    }
    /** construct a region, with a upper left point, and a size */
    ROI(Coord & ul, Coord & size)
	: m_ul(ul), m_size(size)
    {
    }

    void set(Coord & ul, Coord & size)
    {
	m_ul = ul;
	m_size = size;
    }

    void setCorners(const Coord & ul, const Coord & lr)
    {
        setUpperLeft(ul);
	setLowerRight(lr);
    }

    void setUpperLeft(const Coord & ul)
    {
        m_ul = ul;
    }

    void setLowerRight(const Coord & lr)
    {
        m_size = lr - m_ul;
    }

    const Coord & getUL() const
    { return m_ul; }

    const Coord & size() const
    { return m_size; }

    /** returns the "past the end" point of the ROI */
    Coord getLR() const
    { return m_ul + m_size; }

    /** calculates the intersection (common region).
     *
     * @param other      ROI that should be intersected agains this ROI
     * @param intersect  if an intersection was found, it is contained here,
     *                   otherwise it is left unchanged
     *
     * @return  true the ROI's overlap, false otherwise
     */
    bool intersect(const ROI & o, ROI & result) const
    {
	Coord lr = getLR();
        Coord olr = o.getLR();
	
	Coord ulMax(std::max(m_ul.x, o.m_ul.x), std::max(m_ul.y, o.m_ul.y));
	Coord lrMin(std::min(lr.x, olr.x), std::min(lr.y, olr.y));

	// check for if the resulting ROI is positive
	if (ulMax.x < lrMin.x && ulMax.y < lrMin.y) {
	    result.m_ul = ulMax;
	    result.m_size = lrMin - ulMax;
	    return true;
	}
	return false;
    }

    /** calculates the union ROI (ROI that contains both ROI's)
     *
     * @param other      ROI that should be intersected agains this ROI
     * @param result     the union of this and other
     *
     * other and result can be the same object
     *
     */
    void unite(const ROI & o, ROI & result) const
    {
	Coord lr = getLR();
        Coord olr = o.getLR();
	
	Coord ulMin(std::min(m_ul.x, o.m_ul.x), std::min(m_ul.y, o.m_ul.y));
	Coord lrMax(std::max(lr.x, olr.x), std::max(lr.y, olr.y));

	result.m_ul = ulMin;
	result.m_size = lrMax - ulMin;
    }


    /** create iterators to iterate over this ROI
     *
     *  @param image     iterators to source image
     *  @param imageROI  the ROI of the source image
     *
     *  @return iterator set, to iterate over this ROI
     */
    template <typename ImgIter, typename ImgAccessor>
    vigra::triple<ImgIter, ImgIter, ImgAccessor>
    apply(vigra::triple<ImgIter, ImgIter, ImgAccessor> image,
	  const ROI & imageROI)
    {
        vigra::Diff2D ulDiff = getUL() - imageROI.getUL();
	vigra::Diff2D lrDiff = getLR() - imageROI.getLR();
#ifdef DEBUG
	{
	    vigra::Diff2D imgSize = image.second - image.first;
//	    DEBUG_DEBUG("target " << *this << " img:" << imageROI << " img size: " << imgSize);
	    DEBUG_ASSERT(ulDiff.x >= 0 && ulDiff.y >= 0);
	    DEBUG_ASSERT(ulDiff.x < imgSize.x && ulDiff.y < imgSize.y);
	    DEBUG_ASSERT(lrDiff.x <= 0 && lrDiff.y <= 0);
	    DEBUG_ASSERT(lrDiff.x > -imgSize.x && lrDiff.y > -imgSize.y);
	}
#endif
	return vigra::make_triple(image.first + ulDiff, image.second + lrDiff, image.third);
    }

    /** create iterators to iterate on the ROI
     *
     *  @param image     iterators to source image
     *  @param imageROI  the ROI of the source image
     *
     *  @return iterator set, to iterate over this ROI
     */
    template <typename ImgIter, typename ImgAccessor>
    std::pair<ImgIter, ImgAccessor>
    apply(std::pair<ImgIter, ImgAccessor> image,
	  const ROI & imageROI)
    {
        vigra::Diff2D ulDiff = getUL() - imageROI.getUL();
	return std::make_pair(image.first + ulDiff, image.second);
    }

    /** create iterators to iterate over this ROI
     *
     *  @param image     iterators to source image
     *
     *  @return iterator set, to iterate over this ROI
     */
    template <typename ImgIter, typename ImgAccessor>
    vigra::triple<ImgIter, ImgIter, ImgAccessor>
    apply(vigra::triple<ImgIter, ImgIter, ImgAccessor> image)
    {
	return vigra::make_triple(image.first + getUL(), image.first + getLR(), image.third);
    }

    /** create iterators to iterate over the ROI
     *
     *  @param image     iterators to source image (the source image should
     *                   cover the whole ROI)
     *
     *  @return iterator set, to iterate over this ROI
     */
    template <typename ImgIter, typename ImgAccessor>
    std::pair<ImgIter, ImgAccessor>
    apply(std::pair<ImgIter, ImgAccessor> image)
    {
	return std::make_pair(image.first + getUL(), image.second);
    }

protected:
    Coord m_ul;
    Coord m_size;
};


} // namespace

/** print a ROI */
template<class T>
std::ostream & operator<<(std::ostream & o, const vigra_ext::ROI<T> & r)
{
    o << " ROI: " << r.getUL() << " - " << r.getLR() << ", size: " << r.size();
    return o;
}

#endif // VIGRA_EXT_ROI_H
