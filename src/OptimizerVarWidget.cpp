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

#include "CommandHistory.h"
#include "Panorama/PanoCommand.h"
#include "Panorama/Panorama.h"
#include "OptimizerVarWidget.h"

using namespace PT;

OptimizerVarWidget::OptimizerVarWidget(PT::Panorama & p, QWidget* parent, const char* name, WFlags fl )
    : OptimizerVarBaseWidget(parent, name, fl),
      pano(p)
{
    connect(optimizeButton, SIGNAL(clicked()), this, SLOT(optimize()));
    connect(varTable,SIGNAL(valueChanged(int, int)), this, SLOT(setChanges(int,int)));
}

OptimizerVarWidget::~OptimizerVarWidget()
{

}

void OptimizerVarWidget::updateRow(unsigned int imgNr)
{
    assert(varTable);
    assert(optset.size() > imgNr);
    for (int col=0; col<9; col++) {
        QTableItem *aitem = varTable->item(imgNr, col);
        assert(aitem);
        QCheckTableItem * item = (QCheckTableItem*)(aitem);
        if (item == 0) {
            qFatal("wrong table item at %d,%d",imgNr,col);
        } else {
            switch (col) {
            case 0:
                item->setText(QString::number(pano.getVariable(imgNr).yaw.getValue()));
                item->setChecked(true);
                item->setChecked(optset[imgNr].yaw);
                break;
            case 1:
                item->setText(QString::number(pano.getVariable(imgNr).pitch.getValue()));
                item->setChecked(optset[imgNr].pitch);
                break;
            case 2:
                item->setText(QString::number(pano.getVariable(imgNr).roll.getValue()));
                item->setChecked(optset[imgNr].roll);
                break;
            case 3:
                item->setText(QString::number(pano.getVariable(imgNr).HFOV.getValue()));
                item->setChecked(optset[imgNr].HFOV);
                break;
            case 4:
                item->setText(QString::number(pano.getVariable(imgNr).a.getValue()));
                item->setChecked(optset[imgNr].a);
                break;
            case 5:
                item->setText(QString::number(pano.getVariable(imgNr).b.getValue()));
                item->setChecked(optset[imgNr].b);
                break;
            case 6:
                item->setText(QString::number(pano.getVariable(imgNr).c.getValue()));
                item->setChecked(optset[imgNr].c);
                break;
            case 7:
                item->setText(QString::number(pano.getVariable(imgNr).d.getValue()));
                item->setChecked(optset[imgNr].d);
                break;
            case 8:
                item->setText(QString::number(pano.getVariable(imgNr).e.getValue()));
                item->setChecked(optset[imgNr].e);
                break;
            default:
                qFatal("Unknown col");
            }
        }
    }
}


void OptimizerVarWidget::updateView()
{
    qDebug("OptimizerWidget: updateView");
    unsigned int images = pano.getNrOfImages();
    varTable->setNumRows(images);
    optset.resize(images);
    for (unsigned int row=0; row < images; row++) {
        for (unsigned int col=0; col<9; col++) {
            QCheckTableItem * item =  new QCheckTableItem( varTable, "" );
            varTable->setItem( row, col, item);
        }
        updateRow(row);
    }

    for(unsigned int col=0; col < 9; col++) {
        varTable->adjustColumn(col);
    }
}


void OptimizerVarWidget::optimize()
{
    PanoramaOptions opts;
    GlobalCmdHist::getInstance().addCommand(
        new PT::OptimizeCmd(pano, optset, opts)
        );
}

void OptimizerVarWidget::setChanges(int row, int col)
{
    DEBUG_TRACE("setChanges(" << row <<","<< col);
    QTableItem *aitem = varTable->item(row, col);
    assert(aitem);
    QCheckTableItem * item = (QCheckTableItem*)(aitem);
    if (item == 0) {
        qFatal("wrong table item at %d,%d",row,col);
        return;
    }
    bool checked = item->isChecked();
    switch (col) {
    case 0:
        optset[row].yaw = checked;
        break;
    case 1:
        optset[row].pitch = checked;
        break;
    case 2:
        optset[row].roll = checked;
        break;
    case 3:
        optset[row].HFOV = checked;
        break;
    case 4:
        optset[row].a = checked;
        break;
    case 5:
        optset[row].b = checked;
        break;
    case 6:
        optset[row].c = checked;
        break;
    case 7:
        optset[row].d = checked;
        break;
    case 8:
        optset[row].e = checked;
        break;
    default:
        qFatal("Unknown col");
    }
}
