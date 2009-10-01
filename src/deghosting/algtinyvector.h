
/**
 * Implementation of fast vector type with support of linear algebra operations
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

#include <vigra/tinyvector.hxx>
#include <vigra/numerictraits.hxx>

namespace vigra {

/** Fixed size vector with scalar multiplication and element-wise substraction and addition
 */
template <class T, int SIZE>
class AlgTinyVector
{
    public:
        AlgTinyVector(const AlgTinyVector<T,SIZE> & t) {
            for (unsigned int i = 0; i < SIZE; ++i) {
                content[i] = t[i];
            }
        }
        
        AlgTinyVector(T t) {
            for (unsigned int i = 0; i < SIZE; ++i) {
                content[i] = t;
            }
        }
        
        AlgTinyVector() {
            AlgTinyVector(0);
        }
        
        const T operator[](int i) const {
            return content[i];
        }
        
        T& operator[](int i) {
            return content[i];
        }
        
        const T operator*(const AlgTinyVector<T,SIZE> & t) const {
            T retVal = 0;
            for (unsigned int i = 0; i < SIZE; ++i) {
                retVal += t[i] * content[i];
            }
            return retVal;
        }
        
        const AlgTinyVector operator*(const int t) const {
            AlgTinyVector<T,SIZE> retVal;
            for (unsigned int i = 0; i < SIZE; ++i) {
                retVal[i] = t * content[i];
            }
            return retVal;
        }
        
        const AlgTinyVector operator/(const int t) const {
            AlgTinyVector<T,SIZE> retVal;
            for (unsigned int i = 0; i < SIZE; ++i) {
                retVal[i] = content[i] / t;
            }
            return retVal;
        }
        
        const AlgTinyVector operator-(const AlgTinyVector<T,SIZE> & t) const {
            AlgTinyVector<T,SIZE> retVal;
            for (unsigned int i = 0; i < SIZE; ++i) {
                retVal[i] = t[i] - content[i];
            }
            return retVal;
        }
        
        const AlgTinyVector operator+(const AlgTinyVector<T,SIZE> & t) const {
            AlgTinyVector<T,SIZE> retVal;
            for (unsigned int i = 0; i < SIZE; ++i) {
                retVal[i] = t[i] + content[i];
            }
            return retVal;
        }        
        
        AlgTinyVector & operator=(const AlgTinyVector<T,SIZE> & t) {
            if (this == &t)
                return *this;
            for (unsigned int i = 0; i < SIZE; ++i) {
                content[i] = t[i];
            }
            return *this;
        }
        
        AlgTinyVector & operator=(const TinyVector<T,SIZE> & t) {
            for (unsigned int i = 0; i < SIZE; ++i) {
                content[i] = t[i];
            }
            return *this;
        }
        
    private:
        T content[SIZE];
};

template <class T, int SIZE>
struct NumericTraits<AlgTinyVector<T, SIZE> >
{
    typedef AlgTinyVector<T, SIZE> Type;
    typedef AlgTinyVector<typename NumericTraits<T>::Promote, SIZE> Promote;
    typedef AlgTinyVector<typename NumericTraits<T>::RealPromote, SIZE> RealPromote;
    typedef AlgTinyVector<typename NumericTraits<T>::ComplexPromote, SIZE> ComplexPromote;
    typedef T ValueType;

    typedef typename NumericTraits<T>::isIntegral isIntegral;
    typedef VigraFalseType isScalar;
    typedef typename NumericTraits<T>::isSigned isSigned;
    typedef VigraFalseType isOrdered;
    typedef VigraFalseType isComplex;

    static AlgTinyVector<T, SIZE> zero() {
        return AlgTinyVector<T, SIZE>(NumericTraits<T>::zero());
    }
    static AlgTinyVector<T, SIZE> one() {
        return AlgTinyVector<T, SIZE>(NumericTraits<T>::one());
    }
    static AlgTinyVector<T, SIZE> nonZero() {
        return AlgTinyVector<T, SIZE>(NumericTraits<T>::nonZero());
    }

    static Promote toPromote(const AlgTinyVector<T,SIZE> & v)
    {
        return Promote(v);
    }

    static RealPromote toRealPromote(const AlgTinyVector<T,SIZE> & v)
    {
        return RealPromote(v);
    }

    /*static AlgTinyVector<T, SIZE>
    fromPromote(AlgTinyVector<typename NumericTraits<T>::Promote> const & v)
    {
        AlgTinyVector<T, SIZE> res;
        return res;
    }

    static TinyVector<T, SIZE>
    fromRealPromote(AlgTinyVector<typename NumericTraits<T>::RealPromote> const & v)
    {
        TinyVector<T, SIZE> res(detail::dontInit());
        typedef typename detail::LoopType<SIZE>::type ltype;
        ltype::fromRealPromote(res.begin(), v.begin());
        return res;
    }*/
};

}