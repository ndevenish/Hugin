// -*- c-basic-offset: 4 -*-

/** @file CPEditor.cpp
 *
 *  @brief implementation of CPEditor Class
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

#include <qvbox.h>
#include <qtabbar.h>
#include <qscrollview.h>
#include <qsplitter.h>
#include <qlabel.h>


#include "Panorama/Panorama.h"
#include "Panorama/PanoCommand.h"
#include "CPImageDisplay.h"
#include "CPListView.h"
#include "CPEditor.h"

using namespace PT;

CPEditor::CPEditor(PT::Panorama & pano, QWidget* parent, const char* name, WFlags fl)
    : QVBox(parent, name, fl),
      pano(pano),
      cpCreationState(NO_POINT)
{
    QSplitter * mainsplit = new QSplitter(Qt::Vertical, this);
    // ctrl point & image display
    QSplitter * split = new QSplitter(Qt::Horizontal, mainsplit);
    QVBox * rvBox = new QVBox(split);
    firstTab = new QTabBar(rvBox);
    // FIXME.. very ugly hack.. why isnt qtabbar repainted automatically?
    // Am I using an illegal layout here, or is it a bug in QT?
    firstTab->setMinimumHeight(24);
    lsv = new QScrollView(rvBox,"rightScrollView", WResizeNoErase|WNorthWestGravity);
    firstImgDisplay = new CPImageDisplay(lsv->viewport(),"firstImageWidget");
    lsv->addChild(firstImgDisplay);
    lsv->setResizePolicy(QScrollView::AutoOne);


    QVBox * lvBox = new QVBox(split);
    secondTab = new QTabBar(lvBox);
    secondTab->setMinimumHeight(24);
    lsv = new QScrollView(lvBox);
    secondImgDisplay = new CPImageDisplay(lsv->viewport(),"secondImageWidget");
    lsv->addChild(secondImgDisplay);
    lsv->setResizePolicy(QScrollView::AutoOne);

    ctrlPointLV = new CPListView(pano, mainsplit,"ControlPointLV");

    // Control point properties.


    // signals from image display to us
    connect(firstImgDisplay, SIGNAL(pointSelected(unsigned int)),
            this, SLOT(selectPoint(unsigned int)));
    connect(secondImgDisplay, SIGNAL(pointSelected(unsigned int)),
            this, SLOT(selectPoint(unsigned int)));

    connect(firstImgDisplay, SIGNAL(newPointChanged(QPoint)),
            this, SLOT(createNewPointFirst(QPoint)));
    connect(secondImgDisplay, SIGNAL(newPointChanged(QPoint)),
            this, SLOT(createNewPointSecond(QPoint)));

    connect(firstImgDisplay, SIGNAL(pointChanged(unsigned int, QPoint)),
            this, SLOT(moveFirstPoint(unsigned int, QPoint)));
    connect(secondImgDisplay, SIGNAL(pointChanged(unsigned int, QPoint)),
            this, SLOT(moveSecondPoint(unsigned int, QPoint)));

    connect(firstImgDisplay, SIGNAL(regionSelected(QRect)),
            this, SLOT(findRegionFromFirst(QRect)));
    connect(secondImgDisplay, SIGNAL(regionSelected(QRect)),
            this, SLOT(findRegionFromSecond(QRect)));

    // select points in listview..
    connect(firstImgDisplay, SIGNAL(pointSelected(unsigned int)),
            this, SLOT(selectPoint(unsigned int)));

    
    // connect TabBar signals
    connect(firstTab, SIGNAL(selected(int)), this, SLOT(setFirstImageFromTab(int)));
    connect(secondTab, SIGNAL(selected(int)), this, SLOT(setSecondImageFromTab(int)));

    // connect signals from model to us..
    connect(&pano,SIGNAL(imageAdded(unsigned int)), this, SLOT(addImage(unsigned int)));
    connect(&pano,SIGNAL(imageRemoved(unsigned int)), this, SLOT(removeImage(unsigned int)));
    connect(&pano,SIGNAL(ctrlPointAdded(unsigned int)), this, SLOT(addCtrlPoint(unsigned int)));
    connect(&pano,SIGNAL(ctrlPointRemoved(unsigned int)), this, SLOT(removeCtrlPoint(unsigned int)));

}

CPEditor::~CPEditor()
{
}



void CPEditor::addImage(unsigned int img)
{
    firstTab->addTab(new QTab(QString::number(img)));
    firstTab->layoutTabs();
    firstTab->updateGeometry();
    qDebug("first tab size hint: %d, min: %d",
           firstTab->sizeHint().height(),
           firstTab->minimumSizeHint().height());
    secondTab->addTab(new QTab(QString::number(img)));
    secondTab->layoutTabs();
    secondTab->updateGeometry();
}

void CPEditor::removeImage(unsigned int img)
{
    Q_ASSERT((int)img < firstTab->count());
    Q_ASSERT((int)img < secondTab->count());
    firstTab->removeTab(firstTab->tab(img));
    firstTab->layoutTabs();
    firstTab->updateGeometry();
    secondTab->removeTab(secondTab->tab(img));
    secondTab->layoutTabs();
    secondTab->updateGeometry();

    qDebug("first tab size hint: %d, min: %d",
           firstTab->sizeHint().height(),
           firstTab->minimumSizeHint().height());

    qDebug("first image size hint: %d, min: %d, max: %d, actual: %d",
           firstImgDisplay->sizeHint().height(),
           firstImgDisplay->minimumSizeHint().height(),
           firstImgDisplay->maximumSize().height(),
           firstImgDisplay->size().height()
           );
    qDebug("first image scroll viewsize hint: %d, min: %d, max: %d, actual: %d",
           lsv->sizeHint().height(),
           lsv->minimumSizeHint().height(),
           lsv->maximumSize().height(),
           lsv->size().height()
           );
    qDebug("cp list view size hint: %d, min: %d, max: %d, actual:%d",
           ctrlPointLV->sizeHint().height(),
           ctrlPointLV->minimumSizeHint().height(),
           ctrlPointLV->maximumSize().height(),
           ctrlPointLV->size().height()
           );

}

void CPEditor::addCtrlPoint(unsigned int)
{
    updateDialog();
}

void CPEditor::removeCtrlPoint(unsigned int)
{
    updateDialog();
}


void CPEditor::setFirstImageFromTab(int tabId)
{
    qDebug("CPEditor::setFirstImage tabId=%d",tabId);
    if (tabId == -1) {
        firstImage = 0;
        firstImgDisplay->setPixmap(0);
        // no image selected..
    } else {
        QTab * tab = firstTab->tab(tabId);
        Q_ASSERT(tab);
        setFirstImage(tab->text().toUInt());
    }
}


void CPEditor::setSecondImageFromTab(int tabId)
{
    qDebug("CPEditor::setSecondImage tabId=%d",tabId);
    if (tabId == -1) {
        secondImage = 0;
        secondImgDisplay->setPixmap(0);
        // no image selected..
        // clear control points list
    } else {
        QTab * tab = secondTab->tab(tabId);
        Q_ASSERT(tab);
        setSecondImage(tab->text().toUInt());
    }
}


void CPEditor::setFirstImage(unsigned int img)
{
    firstImage = img;
    firstImgDisplay->setPixmap(&pano.getImage(img)->getPixmap());
    updateDialog();
}


void CPEditor::setSecondImage(unsigned int img)
{
    secondImage = img;
    secondImgDisplay->setPixmap(&pano.getImage(img)->getPixmap());
    updateDialog();
}


void CPEditor::updateDialog()
{
    // create a list of all control points and display them.
    const PT::CPVector & controlPoints = pano.getCtrlPoints();
    currentPoints.clear();
    std::vector<QPoint> left;
    std::vector<QPoint> right;

    for (PT::CPVector::const_iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
        if (((*it)->image1->getNr() == firstImage) && ((*it)->image2->getNr() == secondImage)){
            left.push_back(QPoint( (int) (*it)->x1, (int) (*it)->y1));
            right.push_back(QPoint( (int) (*it)->x2, (int) (*it)->y2));
            currentPoints.push_back(*it);
        } else if (((*it)->image2->getNr() == firstImage) && ((*it)->image1->getNr() == secondImage)){
            left.push_back(QPoint( (int) (*it)->x2, (int) (*it)->y2));
            right.push_back(QPoint( (int) (*it)->x1, (int) (*it)->y1));
            currentPoints.push_back(*it);
        }
    }
    ctrlPointLV->setPoints(currentPoints);
    firstImgDisplay->setCtrlPoints(left);
    secondImgDisplay->setCtrlPoints(right);
}


void CPEditor::moveFirstPoint(unsigned int nr, QPoint p)
{
    ControlPoint cp = *pano.getControlPoint(nr);
    cp.x1 = p.x();
    cp.y1 = p.y();
    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangeCtrlPointCmd(pano, nr, cp)
        );
}


void CPEditor::moveSecondPoint(unsigned int nr, QPoint p)
{
    ControlPoint cp = *pano.getControlPoint(nr);
    cp.x2 = p.x();
    cp.y2 = p.y();
    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangeCtrlPointCmd(pano, nr, cp)
        );
}


void CPEditor::createNewPointFirst(QPoint p)
{
    switch (cpCreationState) {
    case NO_POINT:
        cpCreationState = FIRST_POINT;
    case FIRST_POINT:
        newPoint = p;
        break;
    case SECOND_POINT:
        // FIXME: get OptimizeMode from somewhere
        ControlPoint point(pano.getImage(firstImage), p.x(), p.y(),
                           pano.getImage(secondImage), newPoint.x(), newPoint.y(),
                           PT::ControlPoint::X_Y);

        firstImgDisplay->clearNewPoint();
        secondImgDisplay->clearNewPoint();
        cpCreationState = NO_POINT;
        GlobalCmdHist::getInstance().addCommand(
            new PT::AddCtrlPointCmd(pano, point)
            );
    }
}


void CPEditor::createNewPointSecond(QPoint p)
{
    switch (cpCreationState) {
    case NO_POINT:
        cpCreationState = SECOND_POINT;
    case SECOND_POINT:
        newPoint = p;
        break;
    case FIRST_POINT:
        // FIXME: get OptimizeMode from somewhere
        ControlPoint point(pano.getImage(firstImage), newPoint.x(), newPoint.y(),
                           pano.getImage(secondImage), p.x(), p.y(),
                           PT::ControlPoint::X_Y);
        firstImgDisplay->clearNewPoint();
        secondImgDisplay->clearNewPoint();
        cpCreationState = NO_POINT;
        GlobalCmdHist::getInstance().addCommand(
            new PT::AddCtrlPointCmd(pano, point)
            );
    }

}

void CPEditor::findRegionFromFirst(QRect)
{
    // FIXME implement
}


void CPEditor::findRegionFromSecond(QRect)
{
    // FIXME implement
}


void CPEditor::selectPoint(unsigned int pointNr)
{
    qDebug("Point %d selected",pointNr);
    ctrlPointLV->selectPoint(currentPoints[pointNr]);
    // FIXME implement
}
