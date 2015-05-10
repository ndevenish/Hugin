// -*- c-basic-offset: 4 -*-
/** @file PanoToolsOptimizerWrapper.h
 *
 *  @brief wraps around PTOptimizer
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _PANOTOOLS_PTOPTIMISE_H
#define _PANOTOOLS_PTOPTIMISE_H

#include <panodata/PanoramaData.h>


namespace HuginBase
{
namespace PTools
{
    
    /*
     unsigned int optimize_PT(const HuginBase::Panorama & pano,
                      const PanoCommand::UIntVector &imgs,
                      const PanoCommand::OptimizeVector & optvec,
                      PanoCommand::VariableMapVector & vars,
                      PanoCommand::CPVector & cps,
                      int maxIter=1000);
     */


    /** optimize the images \p imgs, for variables \p optvec, using \p vars
     *  as start. saves the control point distances in \p cps.
     *
     * \param panorama description
     * \param imgs vector with all image numbers that should be used.
     * \param optvect vector of vector of variable names
     * \param cps control points
     * \param progDisplay progress display
     * @return 0:good, 1:parser error, 2: parameter error
     *
     */
    IMPEX unsigned int optimize(PanoramaData & pano,
                  const char * script = 0);

} // namespace
} // namespace

#endif // _H
