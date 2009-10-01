
/**
 * Support code for use in deghosting implementation
 * Copyright (C) 2009  Lukáš Jirkovský <l.jirkovsky@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef SUPPORT_H_
#define SUPPORT_H_

#include "deghosting.h"

#include <vigra/functorexpression.hxx>
// used in hugin_hdrmerge
// FIXME: move it to the hugin_hdrmerge
#include <vigra/combineimages.hxx>

namespace deghosting {

using namespace vigra;
using namespace vigra::functor;

/** Logarithm functor
 */
template <class PixelType>
class LogarithmFunctor {
    public:
        LogarithmFunctor(PixelType off=0) : offset(off)  {}
        
        PixelType operator()(PixelType const& v) const {
            return std::log(v + offset);
        }
    protected:
        PixelType offset;
};

/** Logarithm functor
 * specialization for RGBValue
 */
template <class ComponentType>
class LogarithmFunctor<RGBValue<ComponentType> > {
    public:
        LogarithmFunctor(ComponentType off=0) : offset(off) {}
        
        RGBValue<ComponentType> operator()(RGBValue<ComponentType> const& v) const {
            RGBValue<ComponentType> retVal;
            retVal[0] = log(v[0] + offset);
            retVal[1] = log(v[1] + offset);
            retVal[2] = log(v[2] + offset);
            //cout << retVal[0] << "," << retVal[1] << "," << retVal[2] << endl;
            return retVal;
        }
    protected:
        ComponentType offset;
};

/** Functor to apply mexican hat function
 * returns very small values for input near to 0 or 255
 */
template <class PixelType>
class HatFunctor {
    public:
        HatFunctor() {}
        
        PixelType operator()(PixelType v) const {
            PixelType t = (v/127.5 -1);
            t *= t; // ^2
            t *= t; // ^4
            t *= t; // ^8
            t *= t; // ^16
            return 1.0 - t; 
        }
};

/** Functor to apply mexican hat function
 * returns very small values for input near to 0 or 255
 * specialization for RGBValue
 */
template <class ComponentType>
class HatFunctor<RGBValue<ComponentType> > {
    public:
        HatFunctor() {}
        
        ComponentType operator()(RGBValue<ComponentType> v) const {
            ComponentType t = (v.luminance()/127.5 -1);
            t *= t; // ^2
            t *= t; // ^4
            t *= t; // ^8
            t *= t; // ^16
            return 1.0 - t; 
        }
};

/** Fuctor to normalize values
 */
template <class PixelType>
class NormalizeFunctor {
    public:
        NormalizeFunctor(PixelType f) : factor(f) {}
        NormalizeFunctor(PixelType oldMaxValue, PixelType newMaxValue) : factor(newMaxValue/oldMaxValue) {}
        
        PixelType operator()(PixelType const &v) const {
            return v*factor;
        }
    protected:
        PixelType factor;
};

/** Fuctor to normalize values
 * specialization for RGBValue
 */
template <class ComponentType>
class NormalizeFunctor<RGBValue<ComponentType> > {
    public:
        NormalizeFunctor(RGBValue<ComponentType> oldMaxValue, RGBValue<ComponentType> newMaxValue) {
            // TODO
        }
        
        RGBValue<ComponentType> operator()(RGBValue<ComponentType> const &v) {
            // TODO
        }
    protected:
        RGBValue<ComponentType> foo;
};

}  // namespace deghosting

#endif /* SUPPORT_H_ */
