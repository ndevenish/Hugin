// -*- c-basic-offset: 4 -*-
/** @file lut.h
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

#ifndef VIGRA_EXT_EMOR_H
#define VIGRA_EXT_EMOR_H

#include <assert.h>

#include <vector>

#include <vigra/numerictraits.hxx>
#include <vigra_ext/utils.h>

//#define DEBUG_WRITE_FILES

namespace vigra_ext{

namespace EMoR
{
    extern IMPEX double f0[1024];
    extern IMPEX double h[25][1024];

    template <class VECTOR>
    inline void createEMoRLUT(const std::vector<float> & params, VECTOR & lut)
    {
        typedef typename VECTOR::value_type VT;

        VT s = (VT) vigra_ext::LUTTraits<VT>::max();

// lookup tables
        size_t nDim = params.size();
        assert(nDim < 26);
        lut.resize(1024);
        for (int i=0; i<1024; ++i) {
            double t = vigra_ext::EMoR::f0[i];
            for (size_t j=0; j < nDim; j++) {
                t += params[j] * vigra_ext::EMoR::h[j][i];
            }
            lut[i] = vigra::NumericTraits<VT>::fromRealPromote(t*s);
        }
    }
}

} // namespace

#endif // VIGRA_EXT_VIGNETTING_CORRECTION_H
