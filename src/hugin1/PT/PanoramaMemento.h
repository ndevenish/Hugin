// -*- c-basic-offset: 4 -*-
/** @file PanoramaMemento.h
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

#ifndef _Hgn1_PANORAMAMEMENTO_H
#define _Hgn1_PANORAMAMEMENTO_H

#include <panodata/PanoramaVariable.h>
#include <panodata/Lens.h>
#include <panodata/ControlPoint.h>
#include <panodata/PanoramaOptions.h>
#include <panodata/Panorama.h>

#include "PT/PanoImage.h"


namespace PT {


using HuginBase::Variable;
using HuginBase::LinkedVariable;
using HuginBase::LensVariable;

using HuginBase::PrintVar;

using HuginBase::VariableMap;
using HuginBase::VariableMapVector;
using HuginBase::LensVarMap;

using HuginBase::fillVariableMap;
using HuginBase::fillLensVarMap;
using HuginBase::printVariableMap;

using HuginBase::Lens;
using HuginBase::ControlPoint;
using HuginBase::PanoramaOptions;

using HuginBase::CPVector;
using HuginBase::ImageVector;
using HuginBase::LensVector;
using HuginBase::OptimizeVector;


class PanoramaMemento : public HuginBase::PanoramaMemento
{
public:
    PanoramaMemento()
      : HuginBase::PanoramaMemento()
    {}
    
    PanoramaMemento(const HuginBase::PanoramaMemento& mem)
      : HuginBase::PanoramaMemento(mem)
    {}
    
    virtual ~PanoramaMemento() {};
    
public:
    /** enum for supported PTScript syntax bastards */
    enum PTFileFormat { PTFILE_HUGIN, PTFILE_PTGUI, PTFILE_PTA };

    /** load a PTScript file
     *
     *  initializes the PanoramaMemento from a PTScript file
     */
    bool loadPTScript(std::istream & i, int & ptoVersion, const std::string & prefix = "")
    {
        return HuginBase::PanoramaMemento::loadPTScript(i, ptoVersion, prefix);
    }

};


} // namespace
#endif // _PANORAMAMEMENTO_H
