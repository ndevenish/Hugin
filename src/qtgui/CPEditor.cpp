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

#include <algorithm>
#include <functional>

#include <qvbox.h>
#include <qtabbar.h>
#include <qscrollview.h>
#include <qsplitter.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qvalidator.h>
#include <qprogressdialog.h>
#include <qaccel.h>

#include "CommandHistory.h"
#include "Panorama/Panorama.h"
#include "Panorama/PanoCommand.h"
#include "CPImageDisplay.h"
#include "CPListView.h"
#include "utils.h"
#include "stl_utils.h"
#include "QTImageCache.h"
#include "ImageTransforms.h"

#include "CPEditor.h"

using namespace PT;
using namespace std;
using namespace IMG;


CPEditor::CPEditor(PT::Panorama & pano, QProgressBar & mainw, QWidget* parent, const char* name, WFlags fl)
    : QVBox(parent, name, fl),
      pano(pano),
      progBar(mainw),
      cpCreationState(NO_POINT)
{
    QSplitter * mainsplit = new QSplitter(Qt::Vertical, this);
    // ctrl point & image display
    QSplitter * split = new QSplitter(Qt::Horizontal, mainsplit);
    QVBox * rvBox = new QVBox(split);
    firstTab = new QTabBar(rvBox);
    // FIXME.. very ugly hack.. why isnt qtabbar repainted automatically?
    // Am I using an illegal layout here, or is it a bug in QT?
//    firstTab->setMinimumHeight(24);
    lsv = new QScrollView(rvBox,"rightScrollView", WResizeNoErase|WNorthWestGravity|WRepaintNoErase);
    firstImgDisplay = new CPImageDisplay(lsv->viewport(),"firstImageWidget", WRepaintNoErase);
    lsv->addChild(firstImgDisplay);
    lsv->setResizePolicy(QScrollView::AutoOne);



    QVBox * lvBox = new QVBox(split);
    secondTab = new QTabBar(lvBox);
//    secondTab->setMinimumHeight(24);
    lsv = new QScrollView(lvBox,"leftScrollView", WRepaintNoErase);
    secondImgDisplay = new CPImageDisplay(lsv->viewport(),"secondImageWidget", WRepaintNoErase);
    lsv->addChild(secondImgDisplay);
    lsv->setResizePolicy(QScrollView::AutoOne);

    // Control point properties.
    QVBox * propertyBox = new QVBox(mainsplit);

    QValueList<int> tszlist;
    tszlist.append(90);
    tszlist.append(10);
    mainsplit->setSizes(tszlist);
    ctrlPointLV = new CPListView(pano, propertyBox,"ControlPointLV");
    // update  if the pano object is changed

    QHBox * cpPropBox = new QHBox(propertyBox);
    cpPropBox->setMargin(3);
    cpPropBox->setSpacing(4);
    new QLabel(tr("x:"), cpPropBox);
    x1Edit = new QLineEdit(cpPropBox);
    x1Edit->setValidator(new QDoubleValidator(x1Edit));
    new QLabel(tr("y:"),cpPropBox);
    y1Edit = new QLineEdit(cpPropBox);
    y1Edit->setValidator(new QDoubleValidator(y1Edit));
    new QLabel(tr("x:"), cpPropBox);
    x2Edit = new QLineEdit(cpPropBox);
    x2Edit->setValidator(new QDoubleValidator(x2Edit));
    new QLabel(tr("y:"),cpPropBox);
    y2Edit = new QLineEdit(cpPropBox);
    y2Edit->setValidator(new QDoubleValidator(y2Edit));

    new QLabel(tr("Mode"), cpPropBox);
    alignCombo = new QComboBox(cpPropBox);
    alignCombo->insertItem(tr("normal"));
    alignCombo->insertItem(tr("vertical line"));
    alignCombo->insertItem(tr("horizontal line"));

    imageProcCombo = new QComboBox(cpPropBox);
    imageProcCombo->insertItem(tr("Mean Cross Corr Koeff"));
    imageProcCombo->insertItem(tr("Cross Corr Koeff"));
    imageProcCombo->insertItem(tr("Cross Corr"));
    imageProcCombo->insertItem(tr("abs(diff)"));

    normEdit = new QLineEdit("0.7",cpPropBox);
    normEdit->setValidator(new QDoubleValidator(normEdit));

    /*

    // create a window to show the output of a correlation
    resultWindow = new QScrollView(NULL,"output window");
    resultLabel = new QLabel(resultWindow,"label to set text");
    resultWindow->setResizePolicy(QScrollView::AutoOne);
    resultWindow->addChild(resultLabel);
    resultWindow->setCaption("correlation result");

    // create a window to show the src of a correlation
    srcWindow = new QScrollView(NULL,"source window");
    srcLabel = new QLabel(srcWindow);
    srcWindow->setResizePolicy(QScrollView::AutoOne);
    srcWindow->addChild(srcLabel);
    srcWindow->setCaption("gray scale source image");

    // create a window to show the template of a correlation
    templWindow = new QScrollView(NULL,"template");
    templLabel = new QLabel(templWindow);
    templWindow->setResizePolicy(QScrollView::AutoOne);
    templWindow->addChild(templLabel);
    templWindow->setCaption("template image");
    */

    // signals from image display to us
    connect(firstImgDisplay, SIGNAL(pointSelected(unsigned int)),
            this, SLOT(selectLocalPoint(unsigned int)));
    connect(secondImgDisplay, SIGNAL(pointSelected(unsigned int)),
            this, SLOT(selectLocalPoint(unsigned int)));

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
    connect(ctrlPointLV, SIGNAL(selectedPoint(unsigned int)),
            this, SLOT(selectGlobalPoint(unsigned int)));

    // connect TabBar signals
    connect(firstTab, SIGNAL(selected(int)), this, SLOT(setFirstImageFromTab(int)));
    connect(secondTab, SIGNAL(selected(int)), this, SLOT(setSecondImageFromTab(int)));


    // apply changes from line edit
    connect(x1Edit,SIGNAL(returnPressed()), this, SLOT(applyEditedPoint()));
    connect(y1Edit,SIGNAL(returnPressed()), this, SLOT(applyEditedPoint()));
    connect(x2Edit,SIGNAL(returnPressed()), this, SLOT(applyEditedPoint()));
    connect(y2Edit,SIGNAL(returnPressed()), this, SLOT(applyEditedPoint()));

    connect(alignCombo, SIGNAL(activated(int)), this, SLOT(applyEditedPoint()));

    // capture del key to remove point
    QAccel *a = new QAccel( this );
    a->connectItem( a->insertItem(Key_Delete),
                    this,
                    SLOT(removePoint()) );


}

CPEditor::~CPEditor()
{
}



#if 0
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
#endif

#if 0
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
#endif

void CPEditor::setFirstImageFromTab(int tabId)
{
    DEBUG_TRACE("CPEditor::setFirstImageFromTab(" << tabId << ")");
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
    DEBUG_TRACE("CPEditor::setFirstImageFromTab(" << tabId << ")");
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
    DEBUG_TRACE("CPEditor::setFirstImage(" << img << ")");
    firstImage = img;
    QImage & image = QTImageCache::getInstance().getImage(pano.getImage(img).getFilename());
    // convert to a pixmap.
    QPixmap m(image);
    firstImgDisplay->setPixmap(m);
    updateDialog();
}


void CPEditor::setSecondImage(unsigned int img)
{
    DEBUG_TRACE("CPEditor::setSecondImage(" << img << ")");
    secondImage = img;
    QImage & image = QTImageCache::getInstance().getImage(pano.getImage(img).getFilename());
    // convert to a pixmap.
    QPixmap m(image);
    secondImgDisplay->setPixmap(m);
    updateDialog();
}


void CPEditor::updateDialog()
{
    DEBUG_TRACE("CPEditor::updateDialog()");
    // create a list of all control points and display them.
    const PT::CPVector & controlPoints = pano.getCtrlPoints();
    currentPoints.clear();
    mirroredPoints.clear();
    std::vector<QPoint> left;
    std::vector<QPoint> right;

    unsigned int i = 0;
    for (PT::CPVector::const_iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
        ControlPoint point = *it;
        if ((point.image1Nr == firstImage) && (point.image2Nr == secondImage)){
            left.push_back(QPoint( (int) point.x1, (int) point.y1));
            right.push_back(QPoint( (int) point.x2, (int) point.y2));
            currentPoints.push_back(make_pair(it - controlPoints.begin(), *it));
            i++;
        } else if ((point.image2Nr == firstImage) && (point.image1Nr == secondImage)){
            point.mirror();
            mirroredPoints.insert(i);
            left.push_back(QPoint( (int) point.x1, (int) point.y1));
            right.push_back(QPoint( (int) point.x2, (int) point.y2));
            currentPoints.push_back(std::make_pair(it - controlPoints.begin(), point));
            i++;
        }
    }
    ctrlPointLV->setPoints(currentPoints);
    firstImgDisplay->setCtrlPoints(left);
    secondImgDisplay->setCtrlPoints(right);
}


void CPEditor::moveFirstPoint(unsigned int nr, QPoint p)
{
    DEBUG_TRACE("moveFirstPoint("<< nr << ")");
    Q_ASSERT(nr < currentPoints.size());
    ControlPoint cp = currentPoints[nr].second;

    cp.x1 = p.x();
    cp.y1 = p.y();

    if (set_contains(mirroredPoints, nr)) {
        cp.mirror();
    }
    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangeCtrlPointCmd(pano, currentPoints[nr].first, cp)
        );
}


void CPEditor::moveSecondPoint(unsigned int nr, QPoint p)
{
    DEBUG_TRACE("moveSecondPoint("<< nr << ")");
    Q_ASSERT(nr < currentPoints.size());
    ControlPoint cp = currentPoints[nr].second;

    cp.x2 = p.x();
    cp.y2 = p.y();

    if (set_contains(mirroredPoints, nr)) {
        cp.mirror();
    }
    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangeCtrlPointCmd(pano, currentPoints[nr].first, cp)
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
        ControlPoint point(firstImage, p.x(), p.y(),
                           secondImage, newPoint.x(), newPoint.y(),
                           PT::ControlPoint::X_Y);

        firstImgDisplay->clearNewPoint();
        secondImgDisplay->clearNewPoint();
        cpCreationState = NO_POINT;
        GlobalCmdHist::getInstance().addCommand(
            new PT::AddCtrlPointCmd(pano, point)
            );
        // select new control Point
        unsigned int lPoint = pano.getNrOfCtrlPoints() -1;
        selectGlobalPoint(lPoint);
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
        ControlPoint point(firstImage, newPoint.x(), newPoint.y(),
                           secondImage, p.x(), p.y(),
                           PT::ControlPoint::X_Y);
        firstImgDisplay->clearNewPoint();
        secondImgDisplay->clearNewPoint();
        cpCreationState = NO_POINT;
        GlobalCmdHist::getInstance().addCommand(
            new PT::AddCtrlPointCmd(pano, point)
            );
        // select new control Point
        unsigned int lPoint = pano.getNrOfCtrlPoints() -1;
        selectGlobalPoint(lPoint);
    }
}


void CPEditor::findRegionFromFirst(QRect rect)
{
    DEBUG_TRACE("findRegionFromFirst");
    QImage & templsrc = QTImageCache::getInstance().getImage(pano.getImage(firstImage).getFilename());
    QImage & src = QTImageCache::getInstance().getImage(pano.getImage(secondImage).getFilename());
    if (rect.width() %2 == 0) {
        rect.setWidth( rect.width() +1);
    }
    if (rect.height() %2 == 0) {
        rect.setHeight( rect.height() +1);
    }
    QImage srcgrey(src.size(),8,256);
    IMG::ImageToGrey(src, srcgrey);
//    IMG::ImageToHue(src,srcgrey);


    QImage templ = templsrc.copy(rect);
    QImage templgrey(templ.size(),8,256);
    IMG::ImageToGrey(templ, templgrey);
//    IMG::ImageToHue(templ, templgrey);

    QImage dest(src.size(),8, 256);
    for(int i=0; i < 256; i++) {
        srcgrey.setColor(i, qRgb(i,i,i));
        templgrey.setColor(i, qRgb(i,i,i));
    }

    for(int i=0; i < 128; i++) {
        dest.setColor(i,qRgb(i*2,i*2,255));
    }
    for(int i=128; i < 256; i++) {
        dest.setColor(i,qRgb(255,255 - (i-128)*2, 255 - (i-128)*2));
    }
    dest.fill(255);
    srcgrey.save("source.png","PNG");
    templgrey.save("template.png", "PNG");

    QPoint point;
    switch (imageProcCombo->currentItem()) {
    case 0:
    {
        uchar cutoff = (uchar) ((normEdit->text().toDouble() + 1) * 127);
        IMG::MeanCorrCoeff transf(templgrey);
        IMG::Transformer<IMG::MeanCorrCoeff> t;
        connect(&t, SIGNAL(progress(int)), &progBar, SLOT(setProgress(int)));
        DEBUG_DEBUG("calculating corrcoefficient with cutoff " << (int)cutoff);
        point = t.pyramidSearchMax(srcgrey, srcgrey.rect(), dest, transf, 4, cutoff);
        point = t.pyramidSearchMax(srcgrey, srcgrey.rect(), dest, transf, 2, cutoff);
        point = t.pyramidSearchMax(srcgrey, srcgrey.rect(), dest, transf, 1, cutoff);
        break;
    }
    case 3:
    {
        IMG::AbsDifference transf(templgrey);
        IMG::Transformer<IMG::AbsDifference> t;
        connect(&t, SIGNAL(progress(int)), &progBar, SLOT(setProgress(int)));
        point = t.pyramidSearchMin(srcgrey, srcgrey.rect(), dest, transf);
    }
    default:
        DEBUG_ERROR("unknown correlation algorithm " << imageProcCombo->currentItem());
    }
    progBar.setProgress(0);
/*
    case 1:
        transf = new IMG::CorrCoeff(templgrey);
        break;
    case 2:
        transf = new IMG::CrossCorr(templgrey,normEdit->text().toDouble());
        break;
    case 3:
        transf = new IMG::AbsDifference(templgrey,(long int)normEdit->text().toDouble());
        break;
    default:
    }
    if (transf) {
//        t.transform(srcgrey, dest, *transf);
        point = t.pyramidSearchMax(srcgrey, srcgrey.rect(), dest, *transf);
        delete transf;
    }
    //IMG::LensCorr lcor(0,0.9,0, srcgrey.width()/2, srcgrey.height()/2);
    //t.transform(srcgrey, dest, lcor);
    progress.close();
*/
    ControlPoint cp(firstImage, rect.x() + rect.width()/2, rect.y() + rect.height()/2,
                       secondImage, point.x(), point.y(),
                       PT::ControlPoint::X_Y);

    firstImgDisplay->clearNewPoint();
    secondImgDisplay->clearNewPoint();
    cpCreationState = NO_POINT;
    GlobalCmdHist::getInstance().addCommand(
        new PT::AddCtrlPointCmd(pano, cp)
        );

    /*
    resultLabel->setPixmap(QPixmap(dest));
    resultWindow->resize(dest.size()+QSize(2,2));
    resultWindow->show();

    srcLabel->setPixmap(QPixmap(srcgrey));
    srcWindow->resize(srcgrey.size()+QSize(2,2));
    srcWindow->show();

    templLabel->setPixmap(QPixmap(templgrey));
    templWindow->resize(templgrey.size()+QSize(2,2));
    templWindow->show();
    */

}


void CPEditor::findRegionFromSecond(QRect)
{
    DEBUG_ERROR("findRegionFromSecond");
    // FIXME implement
}

bool CPEditor::globalPNr2LocalPNr(unsigned int & localNr, unsigned int globalNr) const
{
    vector<CPoint>::const_iterator it;
    // just wanted to try the advanced stl stuff here.  this searches
    // the currentPoints list for a CPoint (std::pair), whose first
    // element (global point nr) matches pointNr
    it = find_if(currentPoints.begin(),
                 currentPoints.end(),
                 compose1(std::bind2nd(std::equal_to<unsigned int>(), globalNr),
                               select1st<CPoint>()));
    if (it != currentPoints.end()) {
        localNr = it - currentPoints.begin();
        return true;
    } else {
        return false;
    }
}

void CPEditor::selectGlobalPoint(unsigned int globalNr)
{
    unsigned int localNr;
    if (globalPNr2LocalPNr(localNr,globalNr)) {
        DEBUG_DEBUG("CPEditor::setGlobalPoint(" << globalNr << ") found local point " << localNr);
        selectLocalPoint(localNr);
    } else {
        DEBUG_ERROR("CPEditor::setGlobalPoint: point " << globalNr << " not found in currentPoints");
    }
}

void CPEditor::selectLocalPoint(unsigned int LVpointNr)
{
    DEBUG_TRACE("selectLocalPoint(" << LVpointNr << ")");
    ctrlPointLV->selectPoint(currentPoints[LVpointNr].first);

    // update point display
    x1Edit->setText(QString::number(currentPoints[LVpointNr].second.x1));
    y1Edit->setText(QString::number(currentPoints[LVpointNr].second.y1));
    x2Edit->setText(QString::number(currentPoints[LVpointNr].second.x2));
    y2Edit->setText(QString::number(currentPoints[LVpointNr].second.y2));
    alignCombo->setCurrentItem(currentPoints[LVpointNr].second.mode);
}


void CPEditor::updateView()
{
    DEBUG_TRACE("updateView()");
    // create tabs.
    unsigned int nrImages = pano.getNrOfImages();
    unsigned int nrTabs = firstTab->count();
    if (nrTabs < nrImages) {
        for (unsigned int img = nrTabs; img <nrImages; ++img) {

            firstTab->addTab(new QTab(QString::number(img)));
            firstTab->updateGeometry();
            secondTab->addTab(new QTab(QString::number(img)));
            secondTab->layoutTabs();
        }
        firstTab->layoutTabs();
        firstTab->updateGeometry();
        secondTab->layoutTabs();
        secondTab->updateGeometry();
    } else if (nrTabs > nrImages) {
        for (unsigned int img = nrImages; img > nrTabs; img--) {
            firstTab->removeTab(firstTab->tab(img));
            secondTab->removeTab(secondTab->tab(img));
        }
        firstTab->layoutTabs();
        firstTab->updateGeometry();
        secondTab->layoutTabs();
        secondTab->updateGeometry();
    }
    updateDialog();
}


void CPEditor::applyEditedPoint()
{
    unsigned int globalNr;
    unsigned int localNr;
    if (ctrlPointLV->getSelectedPoint(globalNr)) {
        if (globalPNr2LocalPNr(localNr, globalNr)) {
            ControlPoint cp = currentPoints[localNr].second;

            cp.x1 = x1Edit->text().toDouble();
            cp.y1 = y1Edit->text().toDouble();
            cp.x2 = x2Edit->text().toDouble();
            cp.y2 = y2Edit->text().toDouble();

            switch(alignCombo->currentItem()) {
            case 0:
                cp.mode = ControlPoint::X_Y;
                break;
            case 1:
                cp.mode = ControlPoint::X;
                break;
            case 2:
                cp.mode = ControlPoint::Y;
                break;
            default:
                DEBUG_ERROR("Unknown CtrlPoint mode: " << alignCombo->currentItem());
            }
            if (set_contains(mirroredPoints, localNr)) {
                cp.mirror();
            }
            GlobalCmdHist::getInstance().addCommand(
                new PT::ChangeCtrlPointCmd(pano, globalNr, cp)
                );
            return;
        }
    }
    DEBUG_ERROR("applyPoint without a selection in listview");
}


void CPEditor::removePoint()
{
    unsigned int globalNr;
    if (ctrlPointLV->getSelectedPoint(globalNr)) {
        GlobalCmdHist::getInstance().addCommand(
            new PT::RemoveCtrlPointCmd(pano, globalNr)
            );
    }
}
