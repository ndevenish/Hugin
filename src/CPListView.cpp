// -*- c-basic-offset: 4 -*-

/** @file ControlPointLister.cpp
 *
 *  @brief implementation of ControlPointLister Class
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

#include "Panorama/Panorama.h"

#include "CPListView.h"

using namespace PT;

QString CtrlPointLVItem::text(int column) const
{
    switch (column) {
    case 0:
        return QString::number(cp->image1->getNr());
        break;
    case 1:
        return QString::number(cp->x1);
        break;
    case 2:
        return QString::number(cp->y1);
        break;
    case 3:
        return QString::number(cp->image2->getNr());
        break;
    case 4:
        return QString::number(cp->x2);
        break;
    case 5:
        return QString::number(cp->y2);
        break;
    case 6:
        switch (cp->mode) {
        case PT::ControlPoint::X:
            return QObject::tr("H");
            break;
        case PT::ControlPoint::Y:
            return QObject::tr("V");
        case PT::ControlPoint::X_Y:
            return QObject::tr("H+V");
            break;
        }
        break;
    case 7:
        return QString::number(cp->error);
        break;
    default:
        Q_ASSERT(0);
    }
    return ("CtrlPointLVItem:unknown column");
}


CPListView::CPListView(PT::Panorama & pano, QWidget* parent,
                                           const char* name,
                                           WFlags fl)
    : QListView(parent, name, fl),
      pano(pano)
{
    addColumn(tr("Image"));
    addColumn(tr("x"));
    addColumn(tr("y"));
    addColumn(tr("Image"));
    addColumn(tr("x"));
    addColumn(tr("y"));
    addColumn(tr("Alignment"));
    addColumn(tr("Distance"));

    // left to the user.
//      connect(&pano, SIGNAL(ctrlPointAdded(unsigned int)),
//              this, SLOT(addPoint(unsigned int)));
//      connect(&pano, SIGNAL(ctrlPointRemoved(unsigned int)),
//              this, SLOT(removePoint(unsigned int)));
    
    // update  if the pano object is changed
    connect( &pano, SIGNAL(stateChanged()),
             this, SLOT(triggerUpdate()));
}


CPListView::~CPListView()
{

}

void CPListView::addPoint(unsigned int point)
{
    new CtrlPointLVItem(pano.getControlPoint(point),this);
//    triggerUpdate();
}

void CPListView::removePoint(unsigned int point)
{
    qDebug("ctrlPointLV::removePoint %d", point);
    QListViewItemIterator it( this );
    QListViewItem * item;
    PT::ControlPoint * target = pano.getControlPoint(point);
    while ((item = it.current())) {
        CtrlPointLVItem * pitem = static_cast<CtrlPointLVItem*>(item);
        if (pitem->getPoint() == target)
            delete pitem;
        ++it;
    }
//    triggerUpdate();
}



void CPListView::setPoints(std::vector<PT::ControlPoint *> & points)
{
    // remove all points
    clear();
    for (std::vector<ControlPoint*>::const_iterator it = points.begin(); it != points.end(); ++it) {
        new CtrlPointLVItem(*it, this);
    }
}


void CPListView::selectPoint(PT::ControlPoint * point)
{
    qDebug("CPListView::selectPoint(%d)", point);
    QListViewItemIterator it( this );
    QListViewItem * item;
    while ((item = it.current())) {
        CtrlPointLVItem * pitem = static_cast<CtrlPointLVItem*>(item);
        if (pitem->getPoint() == point)
            setSelected(pitem, true);
        ++it;
    }
}
