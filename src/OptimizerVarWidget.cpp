// -*- c-basic-offset: 4 -*-

/** @file OptimizerVarWidget.cpp
 *
 *  @brief implementation of OptimizerVarWidget Class
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

#include <qpushbutton.h>

#include "OptimizerVarWidget.h"

#include "Panorama/Panorama.h"

using namespace PT;

OptimizerVarWidget::OptimizerVarWidget(PT::Panorama & p, QWidget* parent, const char* name, WFlags fl )
    : OptimizerVarBaseWidget(parent, name, fl),
      pano(p)
{
    connect(&pano, SIGNAL(stateChanged()), this, SLOT(updateVariables()));
    connect(optimizeButton, SIGNAL(clicked()), this, SLOT(optimize()));
    connect(varTable,SIGNAL(valueChanged(int, int)), this, SLOT(setChanges(int,int)));
}

OptimizerVarWidget::~OptimizerVarWidget()
{

}

void OptimizerVarWidget::updateRow(unsigned int row, PanoImage * img)
{
    for (int col=0; col<9; col++) {
        QCheckTableItem * item = dynamic_cast<QCheckTableItem*>( varTable->item(row,col));
        if (item == 0) {
            qFatal("wrong table item at %d,%d",row,col);
        } else {
            switch (col) {
            case 0:
                item->setText(QString::number(img->getPosition().yaw));
                item->setChecked(img->getOptions().optimizeYaw);
                break;
            case 1:
                item->setText(QString::number(img->getPosition().pitch));
                item->setChecked(img->getOptions().optimizePitch);
                break;
            case 2:
                item->setText(QString::number(img->getPosition().roll));
                item->setChecked(img->getOptions().optimizeRoll);
                break;
            case 3:
                item->setText(QString::number(img->getLens().HFOV));
                item->setChecked(img->getOptions().optimizeFOV);
                break;
            case 4:
                item->setText(QString::number(img->getLens().a));
                item->setChecked(img->getOptions().optimizeA);
                break;
            case 5:
                item->setText(QString::number(img->getLens().b));
                item->setChecked(img->getOptions().optimizeB);
                break;
            case 6:
                item->setText(QString::number(img->getLens().c));
                item->setChecked(img->getOptions().optimizeC);
                break;
            case 7:
                item->setText(QString::number(img->getLens().d));
                item->setChecked(img->getOptions().optimizeD);
                break;
            case 8:
                item->setText(QString::number(img->getLens().e));
                item->setChecked(img->getOptions().optimizeE);
                break;
            default:
                qFatal("Unknown col");
            }
        }
    }
}


void OptimizerVarWidget::updateVariables()
{
    unsigned int images = pano.getNrImages();
    unsigned int rows = varTable->numRows();
    if (rows < images ) {
        varTable->setNumRows(images);
        for (unsigned int row=rows; row < images; row++) {
            for (unsigned int col=0; col<9; col++) {
                QCheckTableItem * item =  new QCheckTableItem( varTable, "" );
                varTable->setItem( row, col, item);
            }
        }
    } else if (rows > images) {
        varTable->setNumRows(images);
    }

    for (unsigned int i=0; i<images; i++) {
        PanoImage * img = pano.getImage(i);
        updateRow(i,img);
    }

    for(unsigned int col=0; col < 9; col++) {
        varTable->adjustColumn(col);
    }
}


void OptimizerVarWidget::optimize()
{
    pano.optimize();
}

void OptimizerVarWidget::setChanges(int row, int col)
{
    qDebug("%d, %d changed", row, col);
    PanoImage * img = pano.getImage(row);
    ImageOptions opt = img->getOptions();

    QCheckTableItem * item = dynamic_cast<QCheckTableItem*>( varTable->item(row,col));
    if (item == 0) {
        qFatal("wrong table item at %d,%d",row,col);
        return;
    }
    bool checked = item->isChecked();
    switch (col) {
    case 0:
        opt.optimizeYaw = checked;
        break;
    case 1:
        opt.optimizePitch = checked;
        break;
    case 2:
        opt.optimizeRoll = checked;
        break;
    case 3:
        opt.optimizeFOV = checked;
        break;
    case 4:
        opt.optimizeA = checked;
        break;
    case 5:
        opt.optimizeB = checked;
        break;
    case 6:
        opt.optimizeC = checked;
        break;
    case 7:
        opt.optimizeD = checked;
        break;
    case 8:
        opt.optimizeE = checked;
        break;
    default:
        qFatal("Unknown col");
    }
    img->setOptions(opt);
}
