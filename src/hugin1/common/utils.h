// -*- c-basic-offset: 4 -*-
/** @file utils.h
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

#ifndef _Hgn1_UTILS_H
#define _Hgn1_UTILS_H

#include <hugin_utils/utils.h>
#include <appbase/ProgressDisplayOld.h>
#include <appbase/ProgressReporterOld.h>

#include "platform.h"


namespace utils
{
    
    //using hugin_utils::CurrentTime;
    using hugin_utils::doubleToString;
    using hugin_utils::stringToDouble;
    using hugin_utils::stripExtension;
    using hugin_utils::getExtension;
    using hugin_utils::stripPath;
    using hugin_utils::getExtension;
    using hugin_utils::lexical_cast;
    using hugin_utils::QuoteStringInternal;

    using AppBase::ProgressReporter;
    using AppBase::StreamProgressReporter;
    using AppBase::ProgressTask;
    using AppBase::MultiProgressDisplay;
    using AppBase::StreamMultiProgressDisplay;

    
    /** Try to escape special chars on windows and linux.
    *
    * @BUG: I'm quite sure that this routine doesn't replace
    *       some important shell chars I don't know of.
    *       This could lead to nasty behaviour and maybe
    *       even security holes.
    */
    template <class str>
    str wxQuoteStringUnix(const str & arg)
    {
        return QuoteStringInternal(arg, str("\\"), str("\\ ~$\"!@#%^&|'`{}[](),.-+="));
    }
    

} // namespace


#endif // _UTILS_H
