// -*- c-basic-offset: 4 -*-

/** @file PanoOptionsWidget.cpp
 *
 *  @brief implementation of PanoOptionsWidget Class
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

#include <string>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qfiledialog.h>

#include "CommandHistory.h"
#include "Panorama/PanoCommand.h"
#include "Panorama/Panorama.h"
#include "PanoOptionsWidget.h"


PanoOptionsWidget::PanoOptionsWidget(PT::Panorama & pano, QWidget* parent, const char* name, WFlags fl)
    : PanoOptionsBase(parent, name, fl), pano(pano)
{

    updateView();
}

PanoOptionsWidget::~PanoOptionsWidget()
{
}

void PanoOptionsWidget::changeOptions()
{
    PT::PanoramaOptions opt = getOptions();
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd(pano, opt)
        );
}

void PanoOptionsWidget::updateView()
{
    PT::PanoramaOptions opt = pano.getOptions();
    panoProjectionCombo->setCurrentItem(opt.projectionFormat);
    interpolatorCombo->setCurrentItem(opt.interpolator);
    HFOVSpinBox->setValue((int)opt.HFOV);
    gammaEdit->setText(QString::number(opt.gamma));

    outputFormatCombo->setCurrentItem(0);
    widthSpinBox->setValue(opt.width);
    heightSpinBox->setValue(opt.height);
    jpegQualitySpinBox->setValue(opt.quality);
    progressiveCheckBox->setChecked(opt.progressive);
}


PT::PanoramaOptions PanoOptionsWidget::getOptions()
{
    PT::PanoramaOptions opt;
    opt.projectionFormat = (PT::PanoramaOptions::ProjectionFormat)
                           panoProjectionCombo->currentItem();
    opt.interpolator = (PT::PanoramaOptions::Interpolator)
                       interpolatorCombo->currentItem();
    opt.HFOV = HFOVSpinBox->value();
    opt.gamma = gammaEdit->text().toDouble();

    opt.outputFormat = outputFormatCombo->currentText().ascii();
    opt.width = widthSpinBox->value();
    opt.height = heightSpinBox->value();
    opt.quality = jpegQualitySpinBox->value();
    opt.progressive = progressiveCheckBox->isChecked();

    return opt;
}


void PanoOptionsWidget::createPanorama()
{
    PT::PanoramaOptions opt = getOptions();
    opt.outfile = QFileDialog::getSaveFileName (
        QString::null,
// FIXME set mask for each type..
        "",
        this,
        "Save image",
        "Save final panorama").ascii();
    if (opt.outfile == "") {
        return;
    }
    GlobalCmdHist::getInstance().addCommand(
        new PT::StitchCmd(pano,opt)
        );
}


void PanoOptionsWidget::previewPanorama()
{
    PT::PanoramaOptions opt = getOptions();
    // FIXME update with preview width.

    GlobalCmdHist::getInstance().addCommand(
        new PT::StitchCmd(pano,opt)
        );
}
