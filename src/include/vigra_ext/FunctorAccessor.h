// -*- c-basic-offset: 4 -*-
/** @file FunctorAccessor.h
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

#ifndef _FUNCTORACCESSOR_H
#define _FUNCTORACCESSOR_H

#include <vigra/numerictraits.hxx>

namespace vigra_ext {

    
template <class Functor>
class FunctorAccessor
{
  public:
    FunctorAccessor(Functor f)
        : m_f(f)
    {
    }
    
    /** Get functor result
     */
    template <class ITERATOR>
    typename Functor::result_type operator()(ITERATOR const & i) const {
                return m_f(*i); }

    /** Get functor result
     */
    template <class ITERATOR, class DIFFERENCE>
    typename Functor::result_type operator()(ITERATOR const & i, DIFFERENCE d) const
    {
        return m_f(i[d]);
    }
    
    Functor m_f;
};

/** an accessor that executes a functor on every access.
 *
 *  useful to apply point operations temporarily
 */
template <class VALUE, class Functor>
class FunctorAccessor2
{
  public:
    typedef VALUE value_type;

    FunctorAccessor2()
        { }

    /** Create a functor. the parameter \param d is
     *  given to specify the return value
     */
    FunctorAccessor2(Functor & f, VALUE d)
        : m_func(f)
        { }
    /** Get value of the luminance
     */
    template <class ITERATOR>
    typename ITERATOR::value_type operator()(ITERATOR const & i) const
    {
        return m_func(*i);
    }

    /** Get value of the luminance at an offset
     */
    template <class ITERATOR, class DIFFERENCE>
    typename ITERATOR::value_type operator()(ITERATOR const & i, DIFFERENCE d) const
    {
        return m_func(i[d]);
    }
private:
    Functor & m_func;
};


/** a sample functor that can be used to multiply pixel values with a constant*/
template <class T>
struct Multiply
{
    typedef T result_type;
    
    Multiply(T factor)
        : m_factor(factor)
    {}

    template <class PixelType>
    PixelType operator()(PixelType const& v) const
    {
        return vigra::NumericTraits<result_type>::fromRealPromote(v * m_factor);
    }

    T m_factor;
};

}  // namespace



#endif // _FUNCTORACCESSOR_H
