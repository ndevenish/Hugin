// -*- c-basic-offset: 4 -*-

/** @file LensDialog.cpp
 *
 *  @brief implementation of LensDialog Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <math.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qlabel.h>

#include "LensDialog.h"

using namespace PT;

LensDialog::LensDialog(const Lens & lens, QWidget * parent,
                       const char* name, bool modal, WFlags fl)
  : LensDialogBase(parent, name, modal, fl),
    lens(lens)
{
  if (lens.exifHFOV == 0.0) {
    exifButton->setDisabled(true);
  }
  updateHFOV();
  aEdit->setText(QString::number(lens.a));
  bEdit->setText(QString::number(lens.b));
  cEdit->setText(QString::number(lens.c));
  lensTypeCombo->setCurrentItem(lens.projectionFormat);
  buttonOk->setDefault(false);
}

LensDialog::~LensDialog()
{
}


void LensDialog::updateHFOV()
{
  HFOVEdit->setText(QString::number(lens.HFOV));
  focalLengthEdit->setText(QString::number(lens.focalLength));
  focalLengthMultiplierEdit->setText(QString::number(lens.focalLengthConversionFactor));
  focalLengthLabel->setText(QString::number(lens.focalLength * lens.focalLengthConversionFactor));
}

void LensDialog::useEXIF()
{
  lens.HFOV = lens.exifHFOV;
  lens.focalLength = lens.exifFocalLength;
  lens.focalLengthConversionFactor = lens.exifFocalLengthConversionFactor;
  updateHFOV();
}


void LensDialog::calculateFocalLength()
{
    lens.HFOV = HFOVEdit->text().toDouble();
    lens.focalLength = 18.0 / tan( lens.HFOV * M_PI / 360);
    lens.focalLength = lens.focalLength / lens.focalLengthConversionFactor;
    updateHFOV();
}


void LensDialog::calculateHFOV()
{
    lens.focalLength = focalLengthEdit->text().toDouble();
    lens.focalLengthConversionFactor = focalLengthMultiplierEdit->text().toDouble();
    lens.HFOV = 2.0 * atan((36/2)/(lens.focalLength * lens.focalLengthConversionFactor))  * 180/M_PI;
    updateHFOV();
}

const PT::Lens & LensDialog::getLens()
{
    lens.HFOV = HFOVEdit->text().toDouble();
    lens.focalLength = focalLengthEdit->text().toDouble();
    lens.focalLengthConversionFactor = focalLengthMultiplierEdit->text().toDouble();
    lens.a = aEdit->text().toDouble();
    lens.b = bEdit->text().toDouble();
    lens.c = cEdit->text().toDouble();
    lens.projectionFormat = (Lens::ProjectionFormat) lensTypeCombo->currentItem();
    return lens;
}
