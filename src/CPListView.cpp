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


CtrlPointLVItem::~CtrlPointLVItem()
{

}

QString CtrlPointLVItem::text(int column) const
{
    switch (column) {
    case 0:
        return QString::number(pNr);
        break;
    case 1:
        return QString::number(cp.image1Nr);
        break;
    case 2:
        return QString::number(cp.x1);
        break;
    case 3:
        return QString::number(cp.y1);
        break;
    case 4:
        return QString::number(cp.image2Nr);
        break;
    case 5:
        return QString::number(cp.x2);
        break;
    case 6:
        return QString::number(cp.y2);
        break;
    case 7:
        switch (cp.mode) {
        case PT::ControlPoint::X:
            return QObject::tr("V");
            break;
        case PT::ControlPoint::Y:
            return QObject::tr("H");
        case PT::ControlPoint::X_Y:
            return QObject::tr("H+V");
            break;
        }
        break;
    case 8:
        return QString::number(cp.error);
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
    addColumn(tr("Nr"));
    addColumn(tr("Image"));
    addColumn(tr("x"));
    addColumn(tr("y"));
    addColumn(tr("Image"));
    addColumn(tr("x"));
    addColumn(tr("y"));
    addColumn(tr("Alignment"));
    addColumn(tr("Distance"));

    setSorting(0);
    connect(this, SIGNAL(selectionChanged(QListViewItem *)),
            this, SLOT(forwardSelection(QListViewItem *)));

}


CPListView::~CPListView()
{

}

//void CPListView::setPoints(const std::vector<PT::ControlPoint> & points)
void CPListView::setPoints(const std::vector<std::pair<unsigned int, PT::ControlPoint> > & points)
{
    DEBUG_TRACE("setPoints()");
    QListViewItem * selItem = QListView::selectedItem();
    unsigned int oldSelPoint = 0;
    if (selItem) {
        CtrlPointLVItem * item = static_cast<CtrlPointLVItem *>(selItem);
        oldSelPoint = item->getPointNr();
    }
    // remove all points
    clear();
    for (std::vector<std::pair<unsigned int, ControlPoint> >::const_iterator it = points.begin(); it != points.end(); ++it) {
        CtrlPointLVItem * item = new CtrlPointLVItem((*it).second, (*it).first, this);

        if (selItem && (*it).first == oldSelPoint) {
            DEBUG_DEBUG("restoring old selection");
            setSelected(item,true);
        }
    }
}

void CPListView::selectPoint(unsigned int pNr)
{
    DEBUG_TRACE("selectPoint(" << pNr <<")");
    QListViewItemIterator it( this );
    QListViewItem * item;
    while ((item = it.current())) {
        CtrlPointLVItem * pitem = static_cast<CtrlPointLVItem*>(item);
        if (pitem->getPointNr() == pNr)
            setSelected(pitem, true);
        ++it;
    }
}



void CPListView::forwardSelection(QListViewItem * qitem)
{
    assert(qitem);
    // dynamic_cast crashes.. why?
    CtrlPointLVItem * item = static_cast<CtrlPointLVItem *>(qitem);
    assert(item);
    DEBUG_TRACE("forwardSelection(), pointNr: " << item->getPointNr());
    emit(selectedPoint(item->getPointNr()));
}


bool CPListView::getSelectedPoint(unsigned int & pNr)
{
    QListViewItem * selItem = QListView::selectedItem();
    if (selItem) {
        CtrlPointLVItem * item = static_cast<CtrlPointLVItem *>(selItem);
        pNr = item->getPointNr();
        return true;
    } else {
        return false;
    }
}
