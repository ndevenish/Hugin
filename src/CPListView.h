// -*- c-basic-offset: 4 -*-
/** @file ControlPointLister.h
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

#ifndef _CONTROLPOINTLISTER_H
#define _CONTROLPOINTLISTER_H

#include <vector>
#include <qlistview.h>

#include "Panorama/Panorama.h"

namespace PT {
    class ControlPoint;
    class Panorama;
}

class CtrlPointLVItem : public QListViewItem
{
public:
    CtrlPointLVItem(const PT::ControlPoint & point, unsigned int nr,
                    QListView * parent)
        : QListViewItem(parent),
          cp(point), pNr(nr)
        { };
    // to enable rtti on this class
    virtual ~CtrlPointLVItem();
    // return right text
    QString text(int column) const;
    const PT::ControlPoint & getPoint()
        { return cp; };
    unsigned int getPointNr()
        { return pNr; }

private:
    PT::ControlPoint  cp;
    // global point nr.
    unsigned int pNr;
};


/** display a list of control points.
 *
 *  Usage:
 *    1. set points to display with setPoints()
 *    2. connect addPoint() and removePoint() to the signals from PT::Panorama.
 */
class CPListView : public QListView
{
  Q_OBJECT
public:

    /** ctor.
     */
    CPListView(PT::Panorama & pano, QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );

    /** dtor.
     */
    virtual ~CPListView();

//    void setPoints(const std::vector<PT::ControlPoint> & points);
    void setPoints(const std::vector<std::pair<unsigned int, PT::ControlPoint> > & points);
    
    // returns the selected point
    bool getSelectedPoint(unsigned int & pNr);

public slots:
//    void removePoint(unsigned int point);
    void selectPoint(unsigned int point);

signals:
    /// returns the (global) point nr of the selected point
    void selectedPoint(unsigned int point);

protected slots:

    void forwardSelection(QListViewItem * item);


private:
    PT::Panorama & pano;
};



#endif // _CONTROLPOINTLISTER_H
