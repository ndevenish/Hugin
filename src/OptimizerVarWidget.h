// -*- c-basic-offset: 4 -*-
/** @file OptimizerVarWidget.h
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

#ifndef _OPTIMIZERVARWIDGET_H
#define _OPTIMIZERVARWIDGET_H

#include <qtable.h>
#include <Panorama/Panorama.h>
#include "optimizervarwidgetbase.h"

/** brief description.
 *
 *  What this does
 */
class OptimizerVarWidget : public OptimizerVarBaseWidget
{
  Q_OBJECT
public:

    /** ctor.
     */
    OptimizerVarWidget(PT::Panorama & p, QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    /** dtor.
     */
    virtual ~OptimizerVarWidget();


public slots:
    /** changes have been commited
     */
    void updateView();

    void setChanges(int row, int col);

    // call the optimizer
    void optimize();

private:
    void updateRow(unsigned int row);

private:
    PT::Panorama & pano;
    PT::OptimizeVector optset;
};



#endif // _OPTIMIZERVARWIDGET_H
