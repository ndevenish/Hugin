// -*- c-basic-offset: 4 -*-
/** @file LensDialog.h
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

#ifndef _LENSDIALOG_H
#define _LENSDIALOG_H

#include <lensdialogbase.h>

#include "Panorama/PanoImage.h"

#include <qdialog.h>

/** brief description.
 *
 *  What this does
 */
class LensDialog : public LensDialogBase
{
  Q_OBJECT
public:

    /** ctor.
     */
    LensDialog(const PT::LensSettings & lens, QWidget * parent = 0,
               const char* name = 0, bool modal = FALSE, WFlags fl = 0);

    /** dtor.
     */
    virtual ~LensDialog();

    /// get the changes.
    const PT::LensSettings & getLens();

    /// update Edit fields after calc.
    void updateHFOV();

public slots:
    virtual void useEXIF();
    virtual void calculateFocalLength();
    virtual void calculateHFOV();


private:
    PT::LensSettings lens;
};

#endif // _LENSDIALOG_H
