// -*- c-basic-offset: 4 -*-
/** @file OptimizeOptions.h
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

#ifndef _OPTIMIZEOPTIONS_H
#define _OPTIMIZEOPTIONS_H

struct OptimizeVariable
{
    OptimizeVariable()
        : optimize(false),
          linked(false),
          linkImage(0);
    bool optimize;
    bool linked;
    PT::PanoImage * linkImage;
}    
/** brief description.
 *
 *  What this does
 */
class OptimizeOptions
{
public:
    /** ctor.
     */
    OptimizeOptions();

    /** dtor.
     */
    virtual ~OptimizeOptions();

    void addImage(PanoImage * img);
    void removeImage(PanoImage * img);
    
    void setOptimize(unsigned int img, unsigned int param, bool optimize);
    void setLink(unsigned int img, unsigned int param, bool link, PanoImage * img);
    
private:
    typedef std::vector<OptimizeVariable> OptVector;
    typedef std::vector<OptimizeVariable>::iterator OptVectorIterator;
    typedef std::vector<OptVector> OptTable;
    typedef std::vector<OptVector>::iterator OptTableIterator;
    OptTable optVars;
};



#endif // _OPTIMIZEOPTIONS_H
