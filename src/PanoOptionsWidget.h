// -*- c-basic-offset: 4 -*-
/** @file PanoOptionsWidget.h
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

#ifndef _PANOOPTIONSWIDGET_H
#define _PANOOPTIONSWIDGET_H

#include "panooptionsbase.h"

namespace PT {
    class Panorama;
    class PanoramaOptions;
};

/** Widget to edit the (output) panorama properties
 *
 *  What this does
 */
class PanoOptionsWidget : public PanoOptionsBase
{
    Q_OBJECT
public:

    /** ctor.
     */
    PanoOptionsWidget(PT::Panorama & pano, QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );

    /** dtor.
     */
    virtual ~PanoOptionsWidget();

    PT::PanoramaOptions PanoOptionsWidget::getOptions();

public slots:
    void updateDisplay();
    void createPanorama();
    void previewPanorama();

private:
    PT::Panorama & pano;

};



#endif // _PANOOPTIONSWIDGET_H
